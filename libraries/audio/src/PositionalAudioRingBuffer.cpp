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
    _nextOutputTrailingLoudness(0.0f),
    _listenerUnattenuatedZone(NULL),
    _lastFrameReceivedTime(0),
    _interframeTimeGapStatsForJitterCalc(TIME_GAPS_FOR_JITTER_CALC_INTERVAL_SAMPLES, TIME_GAPS_FOR_JITTER_CALC_WINDOW_INTERVALS),
    _interframeTimeGapStatsForStatsPacket(TIME_GAPS_FOR_STATS_PACKET_INTERVAL_SAMPLES, TIME_GAPS_FOR_STATS_PACKET_WINDOW_INTERVALS),
    _framesAvailableStats(FRAMES_AVAILABLE_STATS_INTERVAL_SAMPLES, FRAMES_AVAILABLE_STATS_WINDOW_INTERVALS),
    _desiredJitterBufferFrames(1),
    _dynamicJitterBuffers(dynamicJitterBuffers),
    _consecutiveNotMixedCount(0),
    _starveCount(0),
    _silentFramesDropped(0)
{
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
    
    if (samplesAvailable() >= _numFrameSamples) {
        for (int i = 0; i < _numFrameSamples; ++i) {
            nextLoudness += fabsf(_nextOutput[i]);
        }
        nextLoudness /= _numFrameSamples;
        nextLoudness /= MAX_SAMPLE_VALUE;
    }
    
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
    int desiredJitterBufferSamples = _desiredJitterBufferFrames * _numFrameSamples;
    
    if (!isNotStarvedOrHasMinimumSamples(_numFrameSamples + desiredJitterBufferSamples)) {
        // if the buffer was starved, allow it to accrue at least the desired number of
        // jitter buffer frames before we start taking frames from it for mixing

        if (_shouldOutputStarveDebug) {
            _shouldOutputStarveDebug = false;
        }

        _consecutiveNotMixedCount++;
        return  false;
    } else if (samplesAvailable() < _numFrameSamples) {
        // if the buffer doesn't have a full frame of samples to take for mixing, it is starved
        _isStarved = true;
        _starveCount++;

        _framesAvailableStats.reset();
        
        // reset our _shouldOutputStarveDebug to true so the next is printed
        _shouldOutputStarveDebug = true;

        _consecutiveNotMixedCount = 1;
        return false;
    }
    
    // good buffer, add this to the mix

    // if we just finished refilling after a starve, we have a new jitter buffer length.
    // reset the frames available stats.
    
    _isStarved = false;

    _framesAvailableStats.update(framesAvailable());
    
    // since we've read data from ring buffer at least once - we've started
    _hasStarted = true;

    return true;
}

int PositionalAudioRingBuffer::getCalculatedDesiredJitterBufferFrames() const {
    const float USECS_PER_FRAME = NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL * USECS_PER_SECOND / (float)SAMPLE_RATE;
     
    int calculatedDesiredJitterBufferFrames = ceilf((float)_interframeTimeGapStatsForJitterCalc.getWindowMax() / USECS_PER_FRAME);
    if (calculatedDesiredJitterBufferFrames < 1) {
        calculatedDesiredJitterBufferFrames = 1;
    }
    return calculatedDesiredJitterBufferFrames;
}


void PositionalAudioRingBuffer::frameReceivedUpdateTimingStats() {
    // update the two time gap stats we're keeping
    quint64 now = usecTimestampNow();
    if (_lastFrameReceivedTime != 0) {
        quint64 gap = now - _lastFrameReceivedTime;
        _interframeTimeGapStatsForJitterCalc.update(gap);
        _interframeTimeGapStatsForStatsPacket.update(gap);
    }
    _lastFrameReceivedTime = now;

    // recalculate the _desiredJitterBufferFrames if _interframeTimeGapStatsForJitterCalc has updated stats for us
    if (_interframeTimeGapStatsForJitterCalc.getNewStatsAvailableFlag()) {
        if (!_dynamicJitterBuffers) {
            _desiredJitterBufferFrames = 1; // HACK to see if this fixes the audio silence
        } else {
            const float USECS_PER_FRAME = NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL * USECS_PER_SECOND / (float)SAMPLE_RATE;

            _desiredJitterBufferFrames = ceilf((float)_interframeTimeGapStatsForJitterCalc.getWindowMax() / USECS_PER_FRAME);
            if (_desiredJitterBufferFrames < 1) {
                _desiredJitterBufferFrames = 1;
            }
            const int maxDesired = _frameCapacity - 1;
            if (_desiredJitterBufferFrames > maxDesired) {
                _desiredJitterBufferFrames = maxDesired;
            }
        }
        _interframeTimeGapStatsForJitterCalc.clearNewStatsAvailableFlag();
    }
}

void PositionalAudioRingBuffer::addDroppableSilentSamples(int numSilentSamples) {

    // This adds some number of frames to the desired jitter buffer frames target we use.
    // The larger this value is, the less aggressive we are about reducing the jitter buffer length.
    // Setting this to 0 will try to get the jitter buffer to be exactly _desiredJitterBufferFrames long,
    // which could lead immediately to a starve.
    const int DESIRED_JITTER_BUFFER_FRAMES_PADDING = 1;

    // calculate how many silent frames we should drop.  We only drop silent frames if
    // the running avg num frames available has stabilized and it's more than
    // our desired number of frames by the margin defined above.
    int numSilentFramesToDrop = 0;
    if (_framesAvailableStats.getNewStatsAvailableFlag() && _framesAvailableStats.isWindowFilled()
        && numSilentSamples >= _numFrameSamples) {
        _framesAvailableStats.clearNewStatsAvailableFlag();
        int averageJitterBufferFrames = (int)_framesAvailableStats.getWindowAverage();
        int desiredJitterBufferFramesPlusPadding = _desiredJitterBufferFrames + DESIRED_JITTER_BUFFER_FRAMES_PADDING;

        if (averageJitterBufferFrames > desiredJitterBufferFramesPlusPadding) {
            // our avg jitter buffer size exceeds its desired value, so ignore some silent
            // frames to get that size as close to desired as possible
            int numSilentFramesToDropDesired = averageJitterBufferFrames - desiredJitterBufferFramesPlusPadding;
            int numSilentFramesReceived = numSilentSamples / _numFrameSamples;
            numSilentFramesToDrop = std::min(numSilentFramesToDropDesired, numSilentFramesReceived);

            // since we now have a new jitter buffer length, reset the frames available stats.
            _framesAvailableStats.reset();

            _silentFramesDropped += numSilentFramesToDrop;
        }   
    }

    addSilentFrame(numSilentSamples - numSilentFramesToDrop * _numFrameSamples);
}
