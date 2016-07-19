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

#include <glm/glm.hpp>

#include <NLPacket.h>
#include <Node.h>
#include <NodeList.h>

#include "InboundAudioStream.h"

const int STARVE_HISTORY_CAPACITY = 50;

InboundAudioStream::InboundAudioStream(int numFrameSamples, int numFramesCapacity, const Settings& settings) :
    _ringBuffer(numFrameSamples, false, numFramesCapacity),
    _lastPopSucceeded(false),
    _lastPopOutput(),
    _dynamicJitterBuffers(settings._dynamicJitterBuffers),
    _staticDesiredJitterBufferFrames(settings._staticDesiredJitterBufferFrames),
    _useStDevForJitterCalc(settings._useStDevForJitterCalc),
    _desiredJitterBufferFrames(settings._dynamicJitterBuffers ? 1 : settings._staticDesiredJitterBufferFrames),
    _maxFramesOverDesired(settings._maxFramesOverDesired),
    _isStarved(true),
    _hasStarted(false),
    _consecutiveNotMixedCount(0),
    _starveCount(0),
    _silentFramesDropped(0),
    _oldFramesDropped(0),
    _incomingSequenceNumberStats(STATS_FOR_STATS_PACKET_WINDOW_SECONDS),
    _lastPacketReceivedTime(0),
    _timeGapStatsForDesiredCalcOnTooManyStarves(0, settings._windowSecondsForDesiredCalcOnTooManyStarves),
    _calculatedJitterBufferFramesUsingMaxGap(0),
    _stdevStatsForDesiredCalcOnTooManyStarves(),
    _calculatedJitterBufferFramesUsingStDev(0),
    _timeGapStatsForDesiredReduction(0, settings._windowSecondsForDesiredReduction),
    _starveHistoryWindowSeconds(settings._windowSecondsForDesiredCalcOnTooManyStarves),
    _starveHistory(STARVE_HISTORY_CAPACITY),
    _starveThreshold(settings._windowStarveThreshold),
    _framesAvailableStat(),
    _currentJitterBufferFrames(0),
    _timeGapStatsForStatsPacket(0, STATS_FOR_STATS_PACKET_WINDOW_SECONDS),
    _repetitionWithFade(settings._repetitionWithFade),
    _hasReverb(false)
{
}

void InboundAudioStream::reset() {
    _ringBuffer.reset();
    _lastPopSucceeded = false;
    _lastPopOutput = AudioRingBuffer::ConstIterator();
    _isStarved = true;
    _hasStarted = false;
    resetStats();
    cleanupCodec();
}

void InboundAudioStream::resetStats() {
    if (_dynamicJitterBuffers) {
        _desiredJitterBufferFrames = 1;
    }
    _consecutiveNotMixedCount = 0;
    _starveCount = 0;
    _silentFramesDropped = 0;
    _oldFramesDropped = 0;
    _incomingSequenceNumberStats.reset();
    _lastPacketReceivedTime = 0;
    _timeGapStatsForDesiredCalcOnTooManyStarves.reset();
    _stdevStatsForDesiredCalcOnTooManyStarves = StDev();
    _timeGapStatsForDesiredReduction.reset();
    _starveHistory.clear();
    _framesAvailableStat.reset();
    _currentJitterBufferFrames = 0;
    _timeGapStatsForStatsPacket.reset();
}

void InboundAudioStream::clearBuffer() {
    _ringBuffer.clear();
    _framesAvailableStat.reset();
    _currentJitterBufferFrames = 0;
}

void InboundAudioStream::setReverb(float reverbTime, float wetLevel) {
    _hasReverb = true;
    _reverbTime = reverbTime;
    _wetLevel = wetLevel;
}

void InboundAudioStream::perSecondCallbackForUpdatingStats() {
    _incomingSequenceNumberStats.pushStatsToHistory();
    _timeGapStatsForDesiredCalcOnTooManyStarves.currentIntervalComplete();
    _timeGapStatsForDesiredReduction.currentIntervalComplete();
    _timeGapStatsForStatsPacket.currentIntervalComplete();
}

int InboundAudioStream::parseData(ReceivedMessage& message) {
    PacketType packetType = message.getType();

    // parse sequence number and track it
    quint16 sequence;
    message.readPrimitive(&sequence);
    SequenceNumberStats::ArrivalInfo arrivalInfo = _incomingSequenceNumberStats.sequenceNumberReceived(sequence,
                                                                                                       message.getSourceID());
    QString codecInPacket = message.readString();

    packetReceivedUpdateTimingStats();

    int networkSamples;
    
    // parse the info after the seq number and before the audio data (the stream properties)
    int prePropertyPosition = message.getPosition();
    int propertyBytes = parseStreamProperties(message.getType(), message.readWithoutCopy(message.getBytesLeftToRead()), networkSamples);
    message.seek(prePropertyPosition + propertyBytes);

    // handle this packet based on its arrival status.
    switch (arrivalInfo._status) {
        case SequenceNumberStats::Early: {
            // Packet is early; write droppable silent samples for each of the skipped packets.
            // NOTE: we assume that each dropped packet contains the same number of samples
            // as the packet we just received.
            int packetsDropped = arrivalInfo._seqDiffFromExpected;
            writeSamplesForDroppedPackets(packetsDropped * networkSamples);

            // fall through to OnTime case
        }
        case SequenceNumberStats::OnTime: {
            // Packet is on time; parse its data to the ringbuffer
            if (message.getType() == PacketType::SilentAudioFrame) {
                // FIXME - Some codecs need to know about these silent frames... and can produce better output
                writeDroppableSilentSamples(networkSamples);
            } else {
                // note: PCM and no codec are identical
                bool selectedPCM = _selectedCodecName == "pcm" || _selectedCodecName == "";
                bool packetPCM = codecInPacket == "pcm" || codecInPacket == "";
                if (codecInPacket == _selectedCodecName || (packetPCM && selectedPCM)) {
                    auto afterProperties = message.readWithoutCopy(message.getBytesLeftToRead());
                    parseAudioData(message.getType(), afterProperties);
                } else {
                    qDebug() << "Codec mismatch: expected" << _selectedCodecName << "got" << codecInPacket << "writing silence";
                    writeDroppableSilentSamples(networkSamples);
                    // inform others of the mismatch
                    auto sendingNode = DependencyManager::get<NodeList>()->nodeWithUUID(message.getSourceID());
                    emit mismatchedAudioCodec(sendingNode, _selectedCodecName);
                }
            }
            break;
        }
        default: {
            // For now, late packets are ignored.  It may be good in the future to insert the late audio packet data
            // into the ring buffer to fill in the missing frame if it hasn't been mixed yet.
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

    return message.getPosition();
}

int InboundAudioStream::parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples) {
    if (type == PacketType::SilentAudioFrame) {
        quint16 numSilentSamples = 0;
        memcpy(&numSilentSamples, packetAfterSeqNum.constData(), sizeof(quint16));
        numAudioSamples = numSilentSamples;
        return sizeof(quint16);
    } else {
        // mixed audio packets do not have any info between the seq num and the audio data.
        numAudioSamples = AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL;
        return 0;
    }
}

int InboundAudioStream::parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties) {
    QByteArray decodedBuffer;
    if (_decoder) {
        _decoder->decode(packetAfterStreamProperties, decodedBuffer);
    } else {
        decodedBuffer = packetAfterStreamProperties;
    }
    auto actualSize = decodedBuffer.size();
    return _ringBuffer.writeData(decodedBuffer.data(), actualSize);
}

int InboundAudioStream::writeDroppableSilentSamples(int silentSamples) {
    if (_decoder) {
        _decoder->trackLostFrames(silentSamples);
    }

    // calculate how many silent frames we should drop.
    int samplesPerFrame = _ringBuffer.getNumFrameSamples();
    int desiredJitterBufferFramesPlusPadding = _desiredJitterBufferFrames + DESIRED_JITTER_BUFFER_FRAMES_PADDING;
    int numSilentFramesToDrop = 0;

    if (silentSamples >= samplesPerFrame && _currentJitterBufferFrames > desiredJitterBufferFramesPlusPadding) {

        // our avg jitter buffer size exceeds its desired value, so ignore some silent
        // frames to get that size as close to desired as possible
        int numSilentFramesToDropDesired = _currentJitterBufferFrames - desiredJitterBufferFramesPlusPadding;
        int numSilentFramesReceived = silentSamples / samplesPerFrame;
        numSilentFramesToDrop = std::min(numSilentFramesToDropDesired, numSilentFramesReceived);

        // dont reset _currentJitterBufferFrames here; we want to be able to drop further silent frames
        // without waiting for _framesAvailableStat to fill up to 10s of samples.
        _currentJitterBufferFrames -= numSilentFramesToDrop;
        _silentFramesDropped += numSilentFramesToDrop;

        _framesAvailableStat.reset();
    }

    int ret = _ringBuffer.addSilentSamples(silentSamples - numSilentFramesToDrop * samplesPerFrame);
    
    return ret;
}

int InboundAudioStream::popSamples(int maxSamples, bool allOrNothing, bool starveIfNoSamplesPopped) {
    int samplesPopped = 0;
    int samplesAvailable = _ringBuffer.samplesAvailable();
    if (_isStarved) {
        // we're still refilling; don't pop
        _consecutiveNotMixedCount++;
        _lastPopSucceeded = false;
    } else {
        if (samplesAvailable >= maxSamples) {
            // we have enough samples to pop, so we're good to pop
            popSamplesNoCheck(maxSamples);
            samplesPopped = maxSamples;
        } else if (!allOrNothing && samplesAvailable > 0) {
            // we don't have the requested number of samples, but we do have some
            // samples available, so pop all those (except in all-or-nothing mode)
            popSamplesNoCheck(samplesAvailable);
            samplesPopped = samplesAvailable;
        } else {
            // we can't pop any samples. set this stream to starved if needed
            if (starveIfNoSamplesPopped) {
                setToStarved();
                _consecutiveNotMixedCount++;
            }
            _lastPopSucceeded = false;
        }
    }
    return samplesPopped;
}

int InboundAudioStream::popFrames(int maxFrames, bool allOrNothing, bool starveIfNoFramesPopped) {
    int framesPopped = 0;
    int framesAvailable = _ringBuffer.framesAvailable();
    if (_isStarved) {
        // we're still refilling; don't pop
        _consecutiveNotMixedCount++;
        _lastPopSucceeded = false;
    } else {
        if (framesAvailable >= maxFrames) {
            // we have enough frames to pop, so we're good to pop
            popSamplesNoCheck(maxFrames * _ringBuffer.getNumFrameSamples());
            framesPopped = maxFrames;
        } else if (!allOrNothing && framesAvailable > 0) {
            // we don't have the requested number of frames, but we do have some
            // frames available, so pop all those (except in all-or-nothing mode)
            popSamplesNoCheck(framesAvailable * _ringBuffer.getNumFrameSamples());
            framesPopped = framesAvailable;
        } else {
            // we can't pop any frames. set this stream to starved if needed
            if (starveIfNoFramesPopped) {
                setToStarved();
                _consecutiveNotMixedCount = 1;
            }
            _lastPopSucceeded = false;
        }
    }
    return framesPopped;
}

void InboundAudioStream::popSamplesNoCheck(int samples) {
    _lastPopOutput = _ringBuffer.nextOutput();
    _ringBuffer.shiftReadPosition(samples);
    framesAvailableChanged();

    _hasStarted = true;
    _lastPopSucceeded = true;
}

void InboundAudioStream::framesAvailableChanged() {
    _framesAvailableStat.updateWithSample(_ringBuffer.framesAvailable());

    if (_framesAvailableStat.getElapsedUsecs() >= FRAMES_AVAILABLE_STAT_WINDOW_USECS) {
        _currentJitterBufferFrames = (int)ceil(_framesAvailableStat.getAverage());
        _framesAvailableStat.reset();
    }
}

void InboundAudioStream::setToStarved() {
    _consecutiveNotMixedCount = 0;
    _starveCount++;
    // if we have more than the desired frames when setToStarved() is called, then we'll immediately
    // be considered refilled. in that case, there's no need to set _isStarved to true.
    _isStarved = (_ringBuffer.framesAvailable() < _desiredJitterBufferFrames);

    // record the time of this starve in the starve history
    quint64 now = usecTimestampNow();
    _starveHistory.insert(now);

    if (_dynamicJitterBuffers) {
        // dynamic jitter buffers are enabled. check if this starve put us over the window
        // starve threshold
        quint64 windowEnd = now - _starveHistoryWindowSeconds * USECS_PER_SECOND;
        RingBufferHistory<quint64>::Iterator starvesIterator = _starveHistory.begin();
        RingBufferHistory<quint64>::Iterator end = _starveHistory.end();
        int starvesInWindow = 1;
        do {
            ++starvesIterator;
            if (*starvesIterator < windowEnd) {
                break;
            }
            starvesInWindow++;
        } while (starvesIterator != end);

        // this starve put us over the starve threshold. update _desiredJitterBufferFrames to
        // value determined by window A.
        if (starvesInWindow >= _starveThreshold) {
            int calculatedJitterBufferFrames;
            if (_useStDevForJitterCalc) {
                calculatedJitterBufferFrames = _calculatedJitterBufferFramesUsingStDev;
            } else {
                // we don't know when the next packet will arrive, so it's possible the gap between the last packet and the
                // next packet will exceed the max time gap in the window.  If the time since the last packet has already exceeded
                // the window max gap, then we should use that value to calculate desired frames.
                int framesSinceLastPacket = ceilf((float)(now - _lastPacketReceivedTime)
                                                  / (float)AudioConstants::NETWORK_FRAME_USECS);
                calculatedJitterBufferFrames = std::max(_calculatedJitterBufferFramesUsingMaxGap, framesSinceLastPacket);
            }
            // make sure _desiredJitterBufferFrames does not become lower here
            if (calculatedJitterBufferFrames >= _desiredJitterBufferFrames) {
                _desiredJitterBufferFrames = calculatedJitterBufferFrames;
            }
        }
    }
}

void InboundAudioStream::setSettings(const Settings& settings) {
    setMaxFramesOverDesired(settings._maxFramesOverDesired);
    setDynamicJitterBuffers(settings._dynamicJitterBuffers);
    setStaticDesiredJitterBufferFrames(settings._staticDesiredJitterBufferFrames);
    setUseStDevForJitterCalc(settings._useStDevForJitterCalc);
    setWindowStarveThreshold(settings._windowStarveThreshold);
    setWindowSecondsForDesiredCalcOnTooManyStarves(settings._windowSecondsForDesiredCalcOnTooManyStarves);
    setWindowSecondsForDesiredReduction(settings._windowSecondsForDesiredReduction);
    setRepetitionWithFade(settings._repetitionWithFade);
}

void InboundAudioStream::setDynamicJitterBuffers(bool dynamicJitterBuffers) {
    if (!dynamicJitterBuffers) {
        _desiredJitterBufferFrames = _staticDesiredJitterBufferFrames;
    } else {
        if (!_dynamicJitterBuffers) {
            // if we're enabling dynamic jitter buffer frames, start desired frames at 1
            _desiredJitterBufferFrames = 1;
        }
    }
    _dynamicJitterBuffers = dynamicJitterBuffers;
}

void InboundAudioStream::setStaticDesiredJitterBufferFrames(int staticDesiredJitterBufferFrames) {
    _staticDesiredJitterBufferFrames = staticDesiredJitterBufferFrames;
    if (!_dynamicJitterBuffers) {
        _desiredJitterBufferFrames = _staticDesiredJitterBufferFrames;
    }
}

void InboundAudioStream::setWindowSecondsForDesiredCalcOnTooManyStarves(int windowSecondsForDesiredCalcOnTooManyStarves) {
    _timeGapStatsForDesiredCalcOnTooManyStarves.setWindowIntervals(windowSecondsForDesiredCalcOnTooManyStarves);
    _starveHistoryWindowSeconds = windowSecondsForDesiredCalcOnTooManyStarves;
}

void InboundAudioStream::setWindowSecondsForDesiredReduction(int windowSecondsForDesiredReduction) {
    _timeGapStatsForDesiredReduction.setWindowIntervals(windowSecondsForDesiredReduction);
}


int InboundAudioStream::clampDesiredJitterBufferFramesValue(int desired) const {
    const int MIN_FRAMES_DESIRED = 0;
    const int MAX_FRAMES_DESIRED = _ringBuffer.getFrameCapacity();
    return glm::clamp(desired, MIN_FRAMES_DESIRED, MAX_FRAMES_DESIRED);
}

void InboundAudioStream::packetReceivedUpdateTimingStats() {
    
    // update our timegap stats and desired jitter buffer frames if necessary
    // discard the first few packets we receive since they usually have gaps that aren't represensative of normal jitter
    const quint32 NUM_INITIAL_PACKETS_DISCARD = 3;
    quint64 now = usecTimestampNow();
    if (_incomingSequenceNumberStats.getReceived() > NUM_INITIAL_PACKETS_DISCARD) {
        quint64 gap = now - _lastPacketReceivedTime;
        _timeGapStatsForStatsPacket.update(gap);

        // update all stats used for desired frames calculations under dynamic jitter buffer mode
        _timeGapStatsForDesiredCalcOnTooManyStarves.update(gap);
        _stdevStatsForDesiredCalcOnTooManyStarves.addValue(gap);
        _timeGapStatsForDesiredReduction.update(gap);

        if (_timeGapStatsForDesiredCalcOnTooManyStarves.getNewStatsAvailableFlag()) {
            _calculatedJitterBufferFramesUsingMaxGap = ceilf((float)_timeGapStatsForDesiredCalcOnTooManyStarves.getWindowMax()
                                                             / (float) AudioConstants::NETWORK_FRAME_USECS);
            _timeGapStatsForDesiredCalcOnTooManyStarves.clearNewStatsAvailableFlag();
        }

        const int STANDARD_DEVIATION_SAMPLE_COUNT = 500;
        if (_stdevStatsForDesiredCalcOnTooManyStarves.getSamples() > STANDARD_DEVIATION_SAMPLE_COUNT) {
            const float NUM_STANDARD_DEVIATIONS = 3.0f;
            _calculatedJitterBufferFramesUsingStDev = ceilf(NUM_STANDARD_DEVIATIONS
                                                            * _stdevStatsForDesiredCalcOnTooManyStarves.getStDev()
                                                            / (float) AudioConstants::NETWORK_FRAME_USECS);
            _stdevStatsForDesiredCalcOnTooManyStarves.reset();
        }

        if (_dynamicJitterBuffers) {
            // if the max gap in window B (_timeGapStatsForDesiredReduction) corresponds to a smaller number of frames than _desiredJitterBufferFrames,
            // then reduce _desiredJitterBufferFrames to that number of frames.
            if (_timeGapStatsForDesiredReduction.getNewStatsAvailableFlag() && _timeGapStatsForDesiredReduction.isWindowFilled()) {
                int calculatedJitterBufferFrames = ceilf((float)_timeGapStatsForDesiredReduction.getWindowMax()
                                                         / (float)AudioConstants::NETWORK_FRAME_USECS);
                if (calculatedJitterBufferFrames < _desiredJitterBufferFrames) {
                    _desiredJitterBufferFrames = calculatedJitterBufferFrames;
                }
                _timeGapStatsForDesiredReduction.clearNewStatsAvailableFlag();
            }
        }
    }

    _lastPacketReceivedTime = now;
}

int InboundAudioStream::writeSamplesForDroppedPackets(int networkSamples) {
    if (_repetitionWithFade) {
        return writeLastFrameRepeatedWithFade(networkSamples);
    }
    return writeDroppableSilentSamples(networkSamples);
}

int InboundAudioStream::writeLastFrameRepeatedWithFade(int samples) {
    AudioRingBuffer::ConstIterator frameToRepeat = _ringBuffer.lastFrameWritten();
    int frameSize = _ringBuffer.getNumFrameSamples();
    int samplesToWrite = samples;
    int indexOfRepeat = 0;
    do {
        int samplesToWriteThisIteration = std::min(samplesToWrite, frameSize);
        float fade = calculateRepeatedFrameFadeFactor(indexOfRepeat);
        if (fade == 1.0f) {
            samplesToWrite -= _ringBuffer.writeSamples(frameToRepeat, samplesToWriteThisIteration);
        } else {
            samplesToWrite -= _ringBuffer.writeSamplesWithFade(frameToRepeat, samplesToWriteThisIteration, fade);
        }
        indexOfRepeat++;
    } while (samplesToWrite > 0);

    return samples;
}

AudioStreamStats InboundAudioStream::getAudioStreamStats() const {
    AudioStreamStats streamStats;

    streamStats._timeGapMin = _timeGapStatsForStatsPacket.getMin();
    streamStats._timeGapMax = _timeGapStatsForStatsPacket.getMax();
    streamStats._timeGapAverage = _timeGapStatsForStatsPacket.getAverage();
    streamStats._timeGapWindowMin = _timeGapStatsForStatsPacket.getWindowMin();
    streamStats._timeGapWindowMax = _timeGapStatsForStatsPacket.getWindowMax();
    streamStats._timeGapWindowAverage = _timeGapStatsForStatsPacket.getWindowAverage();

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

float calculateRepeatedFrameFadeFactor(int indexOfRepeat) {
    // fade factor scheme is from this paper:
    // http://inst.eecs.berkeley.edu/~ee290t/sp04/lectures/packet_loss_recov_paper11.pdf

    const float INITIAL_MSECS_NO_FADE = 20.0f;
    const float MSECS_FADE_TO_ZERO = 320.0f;

    const float INITIAL_FRAMES_NO_FADE = INITIAL_MSECS_NO_FADE * AudioConstants::NETWORK_FRAME_MSECS;
    const float FRAMES_FADE_TO_ZERO = MSECS_FADE_TO_ZERO * AudioConstants::NETWORK_FRAME_MSECS;

    const float SAMPLE_RANGE = std::numeric_limits<int16_t>::max();

    if (indexOfRepeat <= INITIAL_FRAMES_NO_FADE) {
        return 1.0f;
    } else if (indexOfRepeat <= INITIAL_FRAMES_NO_FADE + FRAMES_FADE_TO_ZERO) {
        return pow(SAMPLE_RANGE, -(indexOfRepeat - INITIAL_FRAMES_NO_FADE) / FRAMES_FADE_TO_ZERO);

        //return 1.0f - ((indexOfRepeat - INITIAL_FRAMES_NO_FADE) / FRAMES_FADE_TO_ZERO);
    }
    return 0.0f;
}

void InboundAudioStream::setupCodec(CodecPluginPointer codec, const QString& codecName, int numChannels) {
    cleanupCodec(); // cleanup any previously allocated coders first
    _codec = codec;
    _selectedCodecName = codecName;
    if (_codec) {
        _decoder = codec->createDecoder(AudioConstants::SAMPLE_RATE, numChannels);
    }
}

void InboundAudioStream::cleanupCodec() {
    // release any old codec encoder/decoder first...
    if (_codec) {
        if (_decoder) {
            _codec->releaseDecoder(_decoder);
            _decoder = nullptr;
        }
    }
}
