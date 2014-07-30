//
//  InboundAudioStream.cpp
//  libraries/audio/src
//
//  Created by Yixin Wang on 7/17/2014
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InboundAudioStream.h"
#include "PacketHeaders.h"

InboundAudioStream::InboundAudioStream(int numFrameSamples, int numFramesCapacity,
    bool dynamicJitterBuffers, int maxFramesOverDesired, bool useStDevForJitterCalc) :
    _ringBuffer(numFrameSamples, false, numFramesCapacity),
    _lastPopSucceeded(false),
    _lastPopOutput(),
    _dynamicJitterBuffers(dynamicJitterBuffers),
    _dynamicJitterBuffersOverride(false),
    _useStDevForJitterCalc(useStDevForJitterCalc),
    _calculatedJitterBufferFramesUsingMaxGap(0),
    _calculatedJitterBufferFramesUsingStDev(0),
    _desiredJitterBufferFrames(1),
    _maxFramesOverDesired(maxFramesOverDesired),
    _isStarved(true),
    _hasStarted(false),
    _consecutiveNotMixedCount(0),
    _starveCount(0),
    _silentFramesDropped(0),
    _oldFramesDropped(0),
    _incomingSequenceNumberStats(INCOMING_SEQ_STATS_HISTORY_LENGTH_SECONDS),
    _lastFrameReceivedTime(0),
    _interframeTimeGapStatsForJitterCalc(TIME_GAPS_FOR_JITTER_CALC_INTERVAL_SAMPLES, TIME_GAPS_FOR_JITTER_CALC_WINDOW_INTERVALS),
    _interframeTimeGapStatsForStatsPacket(TIME_GAPS_FOR_STATS_PACKET_INTERVAL_SAMPLES, TIME_GAPS_FOR_STATS_PACKET_WINDOW_INTERVALS),
    _framesAvailableStat(),
    _currentJitterBufferFrames(0)
{
}

void InboundAudioStream::reset() {
    _ringBuffer.reset();
    _lastPopSucceeded = false;
    _lastPopOutput = AudioRingBuffer::ConstIterator();
    _isStarved = true;
    _hasStarted = false;
    resetStats();
}

void InboundAudioStream::resetStats() {
    _desiredJitterBufferFrames = 1;
    _consecutiveNotMixedCount = 0;
    _starveCount = 0;
    _silentFramesDropped = 0;
    _oldFramesDropped = 0;
    _incomingSequenceNumberStats.reset();
    _lastFrameReceivedTime = 0;
    _interframeTimeGapStatsForJitterCalc.reset();
    _interframeTimeGapStatsForStatsPacket.reset();
    _framesAvailableStat.reset();
    _currentJitterBufferFrames = 0;
}

void InboundAudioStream::clearBuffer() {
    _ringBuffer.clear();
    _framesAvailableStat.reset();
    _currentJitterBufferFrames = 0;
}

int InboundAudioStream::parseData(const QByteArray& packet) {
    PacketType packetType = packetTypeForPacket(packet);
    QUuid senderUUID = uuidFromPacketHeader(packet);

    // parse header 
    int numBytesHeader = numBytesForPacketHeader(packet);
    const char* sequenceAt = packet.constData() + numBytesHeader;
    int readBytes = numBytesHeader;

    // parse sequence number and track it
    quint16 sequence = *(reinterpret_cast<const quint16*>(sequenceAt));
    readBytes += sizeof(quint16);
    SequenceNumberStats::ArrivalInfo arrivalInfo = frameReceivedUpdateNetworkStats(sequence, senderUUID);

    // TODO: handle generalized silent packet here?????

    // parse the info after the seq number and before the audio data.(the stream properties)
    int numAudioSamples;
    readBytes += parseStreamProperties(packetType, packet.mid(readBytes), numAudioSamples);

    // handle this packet based on its arrival status.
    // For now, late packets are ignored.  It may be good in the future to insert the late audio frame
    // into the ring buffer to fill in the missing frame if it hasn't been mixed yet.
    switch (arrivalInfo._status) {
        case SequenceNumberStats::Early: {
            int packetsDropped = arrivalInfo._seqDiffFromExpected;
            writeSamplesForDroppedPackets(packetsDropped * numAudioSamples);
            // fall through to OnTime case
        }
        case SequenceNumberStats::OnTime: {
            readBytes += parseAudioData(packetType, packet.mid(readBytes), numAudioSamples);
            break;
        }
        default: {
            break;
        }
    }

    int framesAvailable = _ringBuffer.framesAvailable();
    // if this stream was starved, check if we're still starved.
    if (_isStarved && framesAvailable >= _desiredJitterBufferFrames) {
        _isStarved = false;
    }
    // if the ringbuffer exceeds the desired size by more than the threshold specified,
    // drop the oldest frames so the ringbuffer is down to the desired size.
    if (framesAvailable > _desiredJitterBufferFrames + _maxFramesOverDesired) {
        int framesToDrop = framesAvailable - (_desiredJitterBufferFrames + DESIRED_JITTER_BUFFER_FRAMES_PADDING);
        _ringBuffer.shiftReadPosition(framesToDrop * _ringBuffer.getNumFrameSamples());
        
        _framesAvailableStat.reset();
        _currentJitterBufferFrames = 0;

        _oldFramesDropped += framesToDrop;
    }

    framesAvailableChanged();

    return readBytes;
}

bool InboundAudioStream::popFrames(int numFrames, bool starveOnFail) {
    int numSamplesRequested = numFrames * _ringBuffer.getNumFrameSamples();
    if (_isStarved) {
        // we're still refilling; don't pop
        _consecutiveNotMixedCount++;
        _lastPopSucceeded = false;
    } else {
        if (_ringBuffer.samplesAvailable() >= numSamplesRequested) {
            // we have enough samples to pop, so we're good to mix
            _lastPopOutput = _ringBuffer.nextOutput();
            _ringBuffer.shiftReadPosition(numSamplesRequested);
            framesAvailableChanged();

            _hasStarted = true;
            _lastPopSucceeded = true;
        } else {
            // we don't have enough samples, so set this stream to starve
            // if starveOnFail is true
            if (starveOnFail) {
                starved();
                _consecutiveNotMixedCount++;
            }
            _lastPopSucceeded = false;
        }
    }
    return _lastPopSucceeded;
}

void InboundAudioStream::framesAvailableChanged() {
    _framesAvailableStat.updateWithSample(_ringBuffer.framesAvailable());

    if (_framesAvailableStat.getElapsedUsecs() >= FRAMES_AVAILABLE_STAT_WINDOW_USECS) {
        _currentJitterBufferFrames = (int)ceil(_framesAvailableStat.getAverage());
        _framesAvailableStat.reset();
    }
}

void InboundAudioStream::setToStarved() {
    if (!_isStarved && _ringBuffer.framesAvailable() < _desiredJitterBufferFrames) {
        starved();
    }
}

void InboundAudioStream::starved() {
    _isStarved = true;
    _consecutiveNotMixedCount = 0;
    _starveCount++;
}

void InboundAudioStream::overrideDesiredJitterBufferFramesTo(int desired) {
    _dynamicJitterBuffersOverride = true;
    _desiredJitterBufferFrames = clampDesiredJitterBufferFramesValue(desired);
}

void InboundAudioStream::unoverrideDesiredJitterBufferFrames() {
    _dynamicJitterBuffersOverride = false;
    if (!_dynamicJitterBuffers) {
        _desiredJitterBufferFrames = 1;
    }
}

int InboundAudioStream::clampDesiredJitterBufferFramesValue(int desired) const {
    const int MIN_FRAMES_DESIRED = 0;
    const int MAX_FRAMES_DESIRED = _ringBuffer.getFrameCapacity();
    return glm::clamp(desired, MIN_FRAMES_DESIRED, MAX_FRAMES_DESIRED);
}

SequenceNumberStats::ArrivalInfo InboundAudioStream::frameReceivedUpdateNetworkStats(quint16 sequenceNumber, const QUuid& senderUUID) {
    // track the sequence number we received
    SequenceNumberStats::ArrivalInfo arrivalInfo = _incomingSequenceNumberStats.sequenceNumberReceived(sequenceNumber, senderUUID);

    // update our timegap stats and desired jitter buffer frames if necessary
    // discard the first few packets we receive since they usually have gaps that aren't represensative of normal jitter
    const int NUM_INITIAL_PACKETS_DISCARD = 3;
    quint64 now = usecTimestampNow();
    if (_incomingSequenceNumberStats.getNumReceived() > NUM_INITIAL_PACKETS_DISCARD) {
        quint64 gap = now - _lastFrameReceivedTime;
        _interframeTimeGapStatsForStatsPacket.update(gap);

        const float USECS_PER_FRAME = NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL * USECS_PER_SECOND / (float)SAMPLE_RATE;

        // update stats for Freddy's method of jitter calc
        _interframeTimeGapStatsForJitterCalc.update(gap);
        if (_interframeTimeGapStatsForJitterCalc.getNewStatsAvailableFlag()) {
            _calculatedJitterBufferFramesUsingMaxGap = ceilf((float)_interframeTimeGapStatsForJitterCalc.getWindowMax() / USECS_PER_FRAME);
            _interframeTimeGapStatsForJitterCalc.clearNewStatsAvailableFlag();

            if (_dynamicJitterBuffers && !_dynamicJitterBuffersOverride && !_useStDevForJitterCalc) {
                _desiredJitterBufferFrames = clampDesiredJitterBufferFramesValue(_calculatedJitterBufferFramesUsingMaxGap);
            }
        }

        // update stats for Philip's method of jitter calc
        _stdev.addValue(gap);
        const int STANDARD_DEVIATION_SAMPLE_COUNT = 500;
        if (_stdev.getSamples() > STANDARD_DEVIATION_SAMPLE_COUNT) {
            const float NUM_STANDARD_DEVIATIONS = 3.0f;
            _calculatedJitterBufferFramesUsingStDev = (int)ceilf(NUM_STANDARD_DEVIATIONS * _stdev.getStDev() / USECS_PER_FRAME);
            _stdev.reset();

            if (_dynamicJitterBuffers && !_dynamicJitterBuffersOverride && _useStDevForJitterCalc) {
                _desiredJitterBufferFrames = clampDesiredJitterBufferFramesValue(_calculatedJitterBufferFramesUsingStDev);
            }
        }
    }
    _lastFrameReceivedTime = now;

    return arrivalInfo;
}

int InboundAudioStream::writeDroppableSilentSamples(int numSilentSamples) {
    
    // calculate how many silent frames we should drop.
    int samplesPerFrame = _ringBuffer.getNumFrameSamples();
    int desiredJitterBufferFramesPlusPadding = _desiredJitterBufferFrames + DESIRED_JITTER_BUFFER_FRAMES_PADDING;
    int numSilentFramesToDrop = 0;

    if (numSilentSamples >= samplesPerFrame && _currentJitterBufferFrames > desiredJitterBufferFramesPlusPadding) {

        // our avg jitter buffer size exceeds its desired value, so ignore some silent
        // frames to get that size as close to desired as possible
        int numSilentFramesToDropDesired = _currentJitterBufferFrames - desiredJitterBufferFramesPlusPadding;
        int numSilentFramesReceived = numSilentSamples / samplesPerFrame;
        numSilentFramesToDrop = std::min(numSilentFramesToDropDesired, numSilentFramesReceived);

        // dont reset _currentJitterBufferFrames here; we want to be able to drop further silent frames
        // without waiting for _framesAvailableStat to fill up to 10s of samples.
        _currentJitterBufferFrames -= numSilentFramesToDrop;
        _silentFramesDropped += numSilentFramesToDrop;
        
        _framesAvailableStat.reset();
    }

    return _ringBuffer.addSilentFrame(numSilentSamples - numSilentFramesToDrop * samplesPerFrame);
}

int InboundAudioStream::writeSamplesForDroppedPackets(int numSamples) {
    return writeDroppableSilentSamples(numSamples);
}

AudioStreamStats InboundAudioStream::getAudioStreamStats() const {
    AudioStreamStats streamStats;

    streamStats._timeGapMin = _interframeTimeGapStatsForStatsPacket.getMin();
    streamStats._timeGapMax = _interframeTimeGapStatsForStatsPacket.getMax();
    streamStats._timeGapAverage = _interframeTimeGapStatsForStatsPacket.getAverage();
    streamStats._timeGapWindowMin = _interframeTimeGapStatsForStatsPacket.getWindowMin();
    streamStats._timeGapWindowMax = _interframeTimeGapStatsForStatsPacket.getWindowMax();
    streamStats._timeGapWindowAverage = _interframeTimeGapStatsForStatsPacket.getWindowAverage();

    streamStats._framesAvailable = _ringBuffer.framesAvailable();
    streamStats._framesAvailableAverage = _framesAvailableStat.getAverage();
    streamStats._desiredJitterBufferFrames = _desiredJitterBufferFrames;
    streamStats._starveCount = _starveCount;
    streamStats._consecutiveNotMixedCount = _consecutiveNotMixedCount;
    streamStats._overflowCount = _ringBuffer.getOverflowCount();
    streamStats._framesDropped = _silentFramesDropped + _oldFramesDropped;    // TODO: add separate stat for old frames dropped

    streamStats._packetStreamStats = _incomingSequenceNumberStats.getStats();
    streamStats._packetStreamWindowStats = _incomingSequenceNumberStats.getStatsForHistoryWindow();

    return streamStats;
}

AudioStreamStats InboundAudioStream::updateSeqHistoryAndGetAudioStreamStats() {
    _incomingSequenceNumberStats.pushStatsToHistory();
    return getAudioStreamStats();
}
