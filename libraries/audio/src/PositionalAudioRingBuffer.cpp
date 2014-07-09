//
//  PositionalAudioRingBuffer.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cstring>

#include <glm/detail/func_common.hpp>
#include <QtCore/QDataStream>

#include <Node.h>
#include <PacketHeaders.h>
#include <UUID.h>

#include "PositionalAudioRingBuffer.h"
#include "SharedUtil.h"

InterframeTimeGapStats::InterframeTimeGapStats()
    : _lastFrameReceivedTime(0),
    _numSamplesInCurrentInterval(0),
    _currentIntervalMaxGap(0),
    _newestIntervalMaxGapAt(0),
    _windowMaxGap(0),
    _newWindowMaxGapAvailable(false)
{
    memset(_intervalMaxGaps, 0, TIME_GAP_NUM_INTERVALS_IN_WINDOW * sizeof(quint64));
}

void InterframeTimeGapStats::frameReceived() {
    quint64 now = usecTimestampNow();

    // make sure this isn't the first time frameReceived() is called so can actually calculate a gap.
    if (_lastFrameReceivedTime != 0) {
        quint64 gap = now - _lastFrameReceivedTime;

        // update the current interval max
        if (gap > _currentIntervalMaxGap) {
            _currentIntervalMaxGap = gap;

            // keep the window max gap at least as large as the current interval max
            // this allows the window max gap to respond immediately to a sudden spike in gap times
            // also, this prevents the window max gap from staying at 0 until the first interval of samples filled up
            if (_currentIntervalMaxGap > _windowMaxGap) {
                _windowMaxGap = _currentIntervalMaxGap;
                _newWindowMaxGapAvailable = true;
            }
        }
        _numSamplesInCurrentInterval++;

        // if the current interval of samples is now full, record it in our interval maxes
        if (_numSamplesInCurrentInterval == TIME_GAP_NUM_SAMPLES_IN_INTERVAL) {

            // find location to insert this interval's max (increment index cyclically)
            _newestIntervalMaxGapAt = _newestIntervalMaxGapAt == TIME_GAP_NUM_INTERVALS_IN_WINDOW - 1 ? 0 : _newestIntervalMaxGapAt + 1;

            // record the current interval's max gap as the newest
            _intervalMaxGaps[_newestIntervalMaxGapAt] = _currentIntervalMaxGap;

            // update the window max gap, which is the max out of all the past intervals' max gaps
            _windowMaxGap = 0;
            for (int i = 0; i < TIME_GAP_NUM_INTERVALS_IN_WINDOW; i++) {
                if (_intervalMaxGaps[i] > _windowMaxGap) {
                    _windowMaxGap = _intervalMaxGaps[i];
                }
            }
            _newWindowMaxGapAvailable = true;

            // reset the current interval
            _numSamplesInCurrentInterval = 0;
            _currentIntervalMaxGap = 0;
        }
    }
    _lastFrameReceivedTime = now;
}

quint64 InterframeTimeGapStats::getWindowMaxGap() {
    _newWindowMaxGapAvailable = false;
    return _windowMaxGap;
}


PositionalAudioRingBuffer::PositionalAudioRingBuffer(PositionalAudioRingBuffer::Type type, bool isStereo, bool dynamicJitterBuffers) :
        
    AudioRingBuffer(isStereo ? NETWORK_BUFFER_LENGTH_SAMPLES_STEREO : NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL,
                    false, AUDIOMIXER_INBOUND_RING_BUFFER_FRAME_CAPACITY),
    _type(type),
    _position(0.0f, 0.0f, 0.0f),
    _orientation(0.0f, 0.0f, 0.0f, 0.0f),
    _willBeAddedToMix(false),
    _shouldLoopbackForNode(false),
    _shouldOutputStarveDebug(true),
    _isStereo(isStereo),
    _listenerUnattenuatedZone(NULL),
    _desiredJitterBufferFrames(1),
    _currentJitterBufferFrames(-1),
    _dynamicJitterBuffers(dynamicJitterBuffers),
    _consecutiveNotMixedCount(0)
{
}

int PositionalAudioRingBuffer::parseData(const QByteArray& packet) {
    
    // skip the packet header (includes the source UUID)
    int readBytes = numBytesForPacketHeader(packet);

    // skip the sequence number
    readBytes += sizeof(quint16);
    
    // hop over the channel flag that has already been read in AudioMixerClientData
    readBytes += sizeof(quint8);
    // read the positional data
    readBytes += parsePositionalData(packet.mid(readBytes));
   
    if (packetTypeForPacket(packet) == PacketTypeSilentAudioFrame) {
        // this source had no audio to send us, but this counts as a packet
        // write silence equivalent to the number of silent samples they just sent us
        int16_t numSilentSamples;
        
        memcpy(&numSilentSamples, packet.data() + readBytes, sizeof(int16_t));
        
        readBytes += sizeof(int16_t);
        
        // NOTE: fixes a bug in old clients that would send garbage for their number of silentSamples
        numSilentSamples = getSamplesPerFrame();
        
        if (numSilentSamples > 0) {
            if (_dynamicJitterBuffers && _currentJitterBufferFrames > _desiredJitterBufferFrames) {
                // our current jitter buffer size exceeds its desired value, so ignore some silent
                // frames to get that size as close to desired as possible
                int samplesPerFrame = getSamplesPerFrame();
                int numSilentFrames = numSilentSamples / samplesPerFrame;
                int numFramesToDropDesired = _currentJitterBufferFrames - _desiredJitterBufferFrames;

                if (numSilentFrames > numFramesToDropDesired) {
                    // we have more than enough frames to drop to get the jitter buffer to its desired length
                    int numSilentFramesToAdd = numSilentFrames - numFramesToDropDesired;
                    addSilentFrame(numSilentFramesToAdd * samplesPerFrame);
                    _currentJitterBufferFrames = _desiredJitterBufferFrames;

                } else {
                    // we need to drop all frames to get the jitter buffer close as possible to its desired length
                    _currentJitterBufferFrames -= numSilentFrames;
                }
            } else {
                addSilentFrame(numSilentSamples);
            }
        }
    } else {
        // there is audio data to read
        readBytes += writeData(packet.data() + readBytes, packet.size() - readBytes);
    }
    return readBytes;
}

int PositionalAudioRingBuffer::parsePositionalData(const QByteArray& positionalByteArray) {
    QDataStream packetStream(positionalByteArray);
    
    packetStream.readRawData(reinterpret_cast<char*>(&_position), sizeof(_position));
    packetStream.readRawData(reinterpret_cast<char*>(&_orientation), sizeof(_orientation));

    // if this node sent us a NaN for first float in orientation then don't consider this good audio and bail
    if (glm::isnan(_orientation.x)) {
        reset();
        return 0;
    }

    return packetStream.device()->pos();
}

void PositionalAudioRingBuffer::updateNextOutputTrailingLoudness() {
    // ForBoundarySamples means that we expect the number of samples not to roll of the end of the ring buffer
    float nextLoudness = 0;
    
    for (int i = 0; i < _numFrameSamples; ++i) {
        nextLoudness += fabsf(_nextOutput[i]);
    }
    
    nextLoudness /= _numFrameSamples;
    nextLoudness /= MAX_SAMPLE_VALUE;
    
    const int TRAILING_AVERAGE_FRAMES = 100;
    const float CURRENT_FRAME_RATIO = 1.0f / TRAILING_AVERAGE_FRAMES;
    const float PREVIOUS_FRAMES_RATIO = 1.0f - CURRENT_FRAME_RATIO;
    const float LOUDNESS_EPSILON = 0.000001f;
    
    if (nextLoudness >= _nextOutputTrailingLoudness) {
        _nextOutputTrailingLoudness = nextLoudness;
    } else {
        _nextOutputTrailingLoudness = (_nextOutputTrailingLoudness * PREVIOUS_FRAMES_RATIO) + (CURRENT_FRAME_RATIO * nextLoudness);
        
        if (_nextOutputTrailingLoudness < LOUDNESS_EPSILON) {
            _nextOutputTrailingLoudness = 0;
        }
    }
}

bool PositionalAudioRingBuffer::shouldBeAddedToMix() {
    int samplesPerFrame = getSamplesPerFrame();
    int desiredJitterBufferSamples = _desiredJitterBufferFrames * samplesPerFrame;
    
    if (!isNotStarvedOrHasMinimumSamples(samplesPerFrame + desiredJitterBufferSamples)) {
        // if the buffer was starved, allow it to accrue at least the desired number of
        // jitter buffer frames before we start taking frames from it for mixing

        if (_shouldOutputStarveDebug) {
            _shouldOutputStarveDebug = false;
        }

        _consecutiveNotMixedCount++;
        return  false;
    } else if (samplesAvailable() < samplesPerFrame) { 
        // if the buffer doesn't have a full frame of samples to take for mixing, it is starved
        _isStarved = true;
        
        // set to -1 to indicate the jitter buffer is starved
        _currentJitterBufferFrames = -1;
        
        // reset our _shouldOutputStarveDebug to true so the next is printed
        _shouldOutputStarveDebug = true;

        _consecutiveNotMixedCount++;
        return false;
    }
    
    // good buffer, add this to the mix
    if (_isStarved) {
        // if this buffer has just finished replenishing after being starved, the number of frames in it now
        // minus one (since a frame will be read immediately after this) is the length of the jitter buffer
        _currentJitterBufferFrames = samplesAvailable() / samplesPerFrame - 1;
        _isStarved = false;
        _consecutiveNotMixedCount = 0;
    }

    // since we've read data from ring buffer at least once - we've started
    _hasStarted = true;

    return true;
}

int PositionalAudioRingBuffer::getCalculatedDesiredJitterBufferFrames() const {
    int calculatedDesiredJitterBufferFrames = 1;
    const float USECS_PER_FRAME = NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL * USECS_PER_SECOND / (float)SAMPLE_RATE;
     
    calculatedDesiredJitterBufferFrames = ceilf((float)_interframeTimeGapStats.peekWindowMaxGap() / USECS_PER_FRAME);
    if (calculatedDesiredJitterBufferFrames < 1) {
        calculatedDesiredJitterBufferFrames = 1;
    }
    return calculatedDesiredJitterBufferFrames;
}

void PositionalAudioRingBuffer::updateDesiredJitterBufferFrames() {
    if (_interframeTimeGapStats.hasNewWindowMaxGapAvailable()) {
        if (!_dynamicJitterBuffers) {
            _desiredJitterBufferFrames = 1; // HACK to see if this fixes the audio silence
        } else {
            const float USECS_PER_FRAME = NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL * USECS_PER_SECOND / (float)SAMPLE_RATE;
            
            _desiredJitterBufferFrames = ceilf((float)_interframeTimeGapStats.getWindowMaxGap() / USECS_PER_FRAME);
            if (_desiredJitterBufferFrames < 1) {
                _desiredJitterBufferFrames = 1;
            }
            const int maxDesired = _frameCapacity - 1;
            if (_desiredJitterBufferFrames > maxDesired) {
                _desiredJitterBufferFrames = maxDesired;
            }
        }
    }
}
