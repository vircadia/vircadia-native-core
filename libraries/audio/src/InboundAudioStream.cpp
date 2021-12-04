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
#include "TryLocker.h"

#include <glm/glm.hpp>

#include <NLPacket.h>
#include <Node.h>
#include <NodeList.h>

#include "AudioLogging.h"

const bool InboundAudioStream::DEFAULT_DYNAMIC_JITTER_BUFFER_ENABLED = true;
const int InboundAudioStream::DEFAULT_STATIC_JITTER_FRAMES = 1;
const int InboundAudioStream::MAX_FRAMES_OVER_DESIRED = 10;
const int InboundAudioStream::WINDOW_STARVE_THRESHOLD = 3;
const int InboundAudioStream::WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES = 50;
const int InboundAudioStream::WINDOW_SECONDS_FOR_DESIRED_REDUCTION = 10;
const bool InboundAudioStream::USE_STDEV_FOR_JITTER = false;
const bool InboundAudioStream::REPETITION_WITH_FADE = true;

static const int STARVE_HISTORY_CAPACITY = 50;

// This is called 1x/s, and we want it to log the last 5s
static const int UNPLAYED_MS_WINDOW_SECS = 5;

// This adds some number of frames to the desired jitter buffer frames target we use when we're dropping frames.
// The larger this value is, the less frames we drop when attempting to reduce the jitter buffer length.
// Setting this to 0 will try to get the jitter buffer to be exactly _desiredJitterBufferFrames when dropping frames,
// which could lead to a starve soon after.
static const int DESIRED_JITTER_BUFFER_FRAMES_PADDING = 1;

// this controls the length of the window for stats used in the stats packet (not the stats used in
// _desiredJitterBufferFrames calculation)
static const int STATS_FOR_STATS_PACKET_WINDOW_SECONDS = 30;

// this controls the window size of the time-weighted avg of frames available.  Every time the window fills up,
// _currentJitterBufferFrames is updated with the time-weighted avg and the running time-weighted avg is reset.
static const quint64 FRAMES_AVAILABLE_STAT_WINDOW_USECS = 10 * USECS_PER_SECOND;

// When the audio codec is switched, temporary codec mismatch is expected due to packets in-flight.
// A SelectedAudioFormat packet is not sent until this threshold is exceeded.
static const int MAX_MISMATCHED_AUDIO_CODEC_COUNT = 10;

InboundAudioStream::InboundAudioStream(int numChannels, int numFrames, int numBlocks, int numStaticJitterBlocks) :
    _ringBuffer(numChannels * numFrames, numBlocks),
    _numChannels(numChannels),
    _dynamicJitterBufferEnabled(numStaticJitterBlocks == -1),
    _staticJitterBufferFrames(std::max(numStaticJitterBlocks, DEFAULT_STATIC_JITTER_FRAMES)),
    _desiredJitterBufferFrames(_dynamicJitterBufferEnabled ? 1 : _staticJitterBufferFrames),
    _incomingSequenceNumberStats(STATS_FOR_STATS_PACKET_WINDOW_SECONDS),
    _starveHistory(STARVE_HISTORY_CAPACITY),
    _unplayedMs(0, UNPLAYED_MS_WINDOW_SECS),
    _timeGapStatsForStatsPacket(0, STATS_FOR_STATS_PACKET_WINDOW_SECONDS) {}

InboundAudioStream::~InboundAudioStream() {
    cleanupCodec();
}

void InboundAudioStream::reset() {
    _ringBuffer.reset();
    _lastPopSucceeded = false;
    _lastPopOutput = AudioRingBuffer::ConstIterator();
    _isStarved = true;
    _hasStarted = false;
    resetStats();
    // FIXME: calling cleanupCodec() seems to be the cause of the buzzsaw -- we get an assert
    // after this is called in AudioClient.  Ponder and fix...
    // cleanupCodec();
}

void InboundAudioStream::resetStats() {
    if (_dynamicJitterBufferEnabled) {
        _desiredJitterBufferFrames = 1;
    }
    _consecutiveNotMixedCount = 0;
    _starveCount = 0;
    _silentFramesDropped = 0;
    _oldFramesDropped = 0;
    _incomingSequenceNumberStats.reset();
    _lastPacketReceivedTime = 0;
    _timeGapStatsForDesiredCalcOnTooManyStarves.reset();
    _timeGapStatsForDesiredReduction.reset();
    _starveHistory.clear();
    _framesAvailableStat.reset();
    _currentJitterBufferFrames = 0;
    _timeGapStatsForStatsPacket.reset();
    _unplayedMs.reset();
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
    _unplayedMs.currentIntervalComplete();
}

int InboundAudioStream::parseData(ReceivedMessage& message) {
    // parse sequence number and track it
    quint16 sequence;
    message.readPrimitive(&sequence);
    SequenceNumberStats::ArrivalInfo arrivalInfo =
        _incomingSequenceNumberStats.sequenceNumberReceived(sequence, message.getSourceID());
    QString codecInPacket = message.readString();

    packetReceivedUpdateTimingStats();

    int networkFrames;

    // parse the info after the seq number and before the audio data (the stream properties)
    int prePropertyPosition = message.getPosition();
    int propertyBytes = parseStreamProperties(message.getType(), message.readWithoutCopy(message.getBytesLeftToRead()), networkFrames);

    message.seek(prePropertyPosition + propertyBytes);

    // handle this packet based on its arrival status.
    switch (arrivalInfo._status) {
        case SequenceNumberStats::Unreasonable: {
            lostAudioData(1);
            break;
        }
        case SequenceNumberStats::Early: {
            // Packet is early. Treat the packets as if all the packets between the last
            // OnTime packet and this packet were lost. If we're using a codec this will 
            // also result in allowing the codec to interpolate lost data. Then
            // fall through to the "on time" logic to actually handle this packet
            int packetsDropped = arrivalInfo._seqDiffFromExpected;
            lostAudioData(packetsDropped);

            // fall through to OnTime case
        }
        // FALLTHRU
        case SequenceNumberStats::OnTime: {
            // Packet is on time; parse its data to the ringbuffer
            if (message.getType() == PacketType::SilentAudioFrame
                || message.getType() == PacketType::ReplicatedSilentAudioFrame) {
                // If we recieved a SilentAudioFrame from our sender, we might want to drop
                // some of the samples in order to catch up to our desired jitter buffer size.
                writeDroppableSilentFrames(networkFrames);

            } else {
                // note: PCM and no codec are identical
                bool selectedPCM = _selectedCodecName == "pcm" || _selectedCodecName == "";
                bool packetPCM = codecInPacket == "pcm" || codecInPacket == "";
                if (codecInPacket == _selectedCodecName || (packetPCM && selectedPCM)) {
                    auto afterProperties = message.readWithoutCopy(message.getBytesLeftToRead());
                    parseAudioData(afterProperties);
                    _mismatchedAudioCodecCount = 0;

                } else {
                    _mismatchedAudioCodecCount++;

                    if (packetPCM) {
                        // If there are PCM packets in-flight after the codec is changed, use them.
                        auto afterProperties = message.readWithoutCopy(message.getBytesLeftToRead());
                        _ringBuffer.writeData(afterProperties.data(), afterProperties.size());
                    } else {
                        // Since the data in the stream is using a codec that we aren't prepared for,
                        // we need to let the codec know that we don't have data for it, this will
                        // allow the codec to interpolate missing data and produce a fade to silence.
                        lostAudioData(1);
                    }

                    if (_mismatchedAudioCodecCount > MAX_MISMATCHED_AUDIO_CODEC_COUNT) {
                        _mismatchedAudioCodecCount = 0;

                        // inform others of the mismatch
                        auto sendingNode = DependencyManager::get<NodeList>()->nodeWithLocalID(message.getSourceID());
                        if (sendingNode) {
                            emit mismatchedAudioCodec(sendingNode, _selectedCodecName, codecInPacket);
                            qDebug(audio) << "Codec mismatch threshold exceeded, sent selected codec"
                                << _selectedCodecName << "to" << message.getSenderSockAddr();
                        }
                    }
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
    if (framesAvailable > _desiredJitterBufferFrames + MAX_FRAMES_OVER_DESIRED) {
        int framesToDrop = framesAvailable - (_desiredJitterBufferFrames + DESIRED_JITTER_BUFFER_FRAMES_PADDING);
        _ringBuffer.shiftReadPosition(framesToDrop * _ringBuffer.getNumFrameSamples());

        _framesAvailableStat.reset();
        _currentJitterBufferFrames = 0;

        _oldFramesDropped += framesToDrop;

        qCInfo(audiostream, "Dropped %d frames", framesToDrop);
        qCInfo(audiostream, "Reset current jitter frames");
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

int InboundAudioStream::lostAudioData(int numPackets) {
    QByteArray decodedBuffer;

    while (numPackets--) {
        MutexTryLocker lock(_decoderMutex);
        if (!lock.isLocked()) {
            // an incoming packet is being processed,
            // and will likely be on the ring buffer shortly,
            // so don't bother generating more data
            qCInfo(audiostream, "Packet currently being unpacked or lost frame already being generated.  Not generating lost frame.");
            return 0;
        }
        if (_decoder) {
            _decoder->lostFrame(decodedBuffer);
        } else {
            decodedBuffer.resize(AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL * _numChannels);
            memset(decodedBuffer.data(), 0, decodedBuffer.size());
        }
        _ringBuffer.writeData(decodedBuffer.data(), decodedBuffer.size());
    }
    return 0;
}

int InboundAudioStream::parseAudioData(const QByteArray& packetAfterStreamProperties) {
    QByteArray decodedBuffer;

    // may block on the real-time thread, which is acceptible as 
    // parseAudioData is only called by the packet processing
    // thread which, while high performance, is not as sensitive to
    // delays as the real-time thread.
    QMutexLocker lock(&_decoderMutex);
    if (_decoder) {
        _decoder->decode(packetAfterStreamProperties, decodedBuffer);
    } else {
        decodedBuffer = packetAfterStreamProperties;
    }
    auto actualSize = decodedBuffer.size();
    return _ringBuffer.writeData(decodedBuffer.data(), actualSize);
}

int InboundAudioStream::writeDroppableSilentFrames(int silentFrames) {

    // We can't guarentee that all clients have faded the stream down
    // to silence and encoded that silence before sending us a 
    // SilentAudioFrame. If the encoder has truncated the stream it will
    // leave the decoder holding some unknown loud state. To handle this 
    // case we will call the decoder's lostFrame() method, which indicates
    // that it should interpolate from its last known state down toward 
    // silence.
    {
        // may block on the real-time thread, which is acceptible as 
        // writeDroppableSilentFrames is only called by the packet processing
        // thread which, while high performance, is not as sensitive to
        // delays as the real-time thread.
        QMutexLocker lock(&_decoderMutex);
        if (_decoder) {
            // FIXME - We could potentially use the output from the codec, in which 
            // case we might get a cleaner fade toward silence. NOTE: The below logic 
            // attempts to catch up in the event that the jitter buffers have grown. 
            // The better long term fix is to use the output from the decode, detect
            // when it actually reaches silence, and then delete the silent portions
            // of the jitter buffers. Or petentially do a cross fade from the decode
            // output to silence.
            QByteArray decodedBuffer;
            _decoder->lostFrame(decodedBuffer);
        }
    }

    // calculate how many silent frames we should drop.
    int silentSamples = silentFrames * _numChannels;
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

        qCInfo(audiostream, "Dropped %d silent frames", numSilentFramesToDrop);
        qCInfo(audiostream, "Set current jitter frames to %d (dropped)", _currentJitterBufferFrames);

        _framesAvailableStat.reset();
    }

    int ret = _ringBuffer.addSilentSamples(silentSamples - numSilentFramesToDrop * samplesPerFrame);
    
    return ret;
}

int InboundAudioStream::popSamples(int maxSamples, bool allOrNothing) {
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
            // we can't pop any samples, set this stream to starved for jitter 
            // buffer calculations.
            setToStarved();
            _consecutiveNotMixedCount++;

            // use PLC to generate extrapolated audio data, to reduce clicking
            if (allOrNothing) {
                int samplesNeeded = maxSamples - samplesAvailable;
                int packetsNeeded = (samplesNeeded + _ringBuffer.getNumFrameSamples() - 1) / _ringBuffer.getNumFrameSamples();
                lostAudioData(packetsNeeded);
            } else {
                lostAudioData(1);
            }
            samplesAvailable = _ringBuffer.samplesAvailable();

            if (samplesAvailable > 0) {
                samplesPopped = std::min(samplesAvailable, maxSamples);
                popSamplesNoCheck(samplesPopped);
            } else {
                // No samples available means a packet is currently being
                // processed, so we don't generate lost audio data, and instead
                // just wait for the packet to come in.  This prevents locking
                // the real-time audio thread at the cost of a potential (but rare)
                // 'click'
                _lastPopSucceeded = false;
            }
        }
    }
    return samplesPopped;
}

int InboundAudioStream::popFrames(int maxFrames, bool allOrNothing) {
    int numFrameSamples = _ringBuffer.getNumFrameSamples();
    int samplesPopped = popSamples(maxFrames * numFrameSamples, allOrNothing);
    return samplesPopped / numFrameSamples;
}

void InboundAudioStream::popSamplesNoCheck(int samples) {
    float unplayedMs = (_ringBuffer.samplesAvailable() / (float)_ringBuffer.getNumFrameSamples()) * AudioConstants::NETWORK_FRAME_MSECS;
    _unplayedMs.update(unplayedMs);

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
        qCInfo(audiostream, "Set current jitter frames to %d (changed)", _currentJitterBufferFrames);

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

    if (_dynamicJitterBufferEnabled) {
        // dynamic jitter buffers are enabled. check if this starve put us over the window
        // starve threshold
        quint64 windowEnd = now - WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES * USECS_PER_SECOND;
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
        if (starvesInWindow >= WINDOW_STARVE_THRESHOLD) {
            int calculatedJitterBufferFrames;
            // we don't know when the next packet will arrive, so it's possible the gap between the last packet and the
            // next packet will exceed the max time gap in the window.  If the time since the last packet has already exceeded
            // the window max gap, then we should use that value to calculate desired frames.
            int framesSinceLastPacket = ceilf((float)(now - _lastPacketReceivedTime)
                                              / (float)AudioConstants::NETWORK_FRAME_USECS);
            calculatedJitterBufferFrames = std::max(_calculatedJitterBufferFrames, framesSinceLastPacket);
            // make sure _desiredJitterBufferFrames does not become lower here
            if (calculatedJitterBufferFrames >= _desiredJitterBufferFrames) {
                _desiredJitterBufferFrames = calculatedJitterBufferFrames;
                qCInfo(audiostream, "Set desired jitter frames to %d (starved)", _desiredJitterBufferFrames);
            }
        }
    }
}

void InboundAudioStream::setDynamicJitterBufferEnabled(bool enable) {
    if (!enable) {
        _desiredJitterBufferFrames = _staticJitterBufferFrames;
    } else {
        if (!_dynamicJitterBufferEnabled) {
            // if we're enabling dynamic jitter buffer frames, start desired frames at 1
            _desiredJitterBufferFrames = 1;
        }
    }
    _dynamicJitterBufferEnabled = enable;
}

void InboundAudioStream::setStaticJitterBufferFrames(int staticJitterBufferFrames) {
    _staticJitterBufferFrames = staticJitterBufferFrames;
    if (!_dynamicJitterBufferEnabled) {
        _desiredJitterBufferFrames = _staticJitterBufferFrames;
    }
}

void InboundAudioStream::packetReceivedUpdateTimingStats() {
    
    // update our timegap stats and desired jitter buffer frames if necessary
    // discard the first few packets we receive since they usually have gaps that aren't represensative of normal jitter
    const quint32 NUM_INITIAL_PACKETS_DISCARD = 1000; // 10s
    quint64 now = usecTimestampNow();
    if (_incomingSequenceNumberStats.getReceived() > NUM_INITIAL_PACKETS_DISCARD) {
        quint64 gap = now - _lastPacketReceivedTime;
        _timeGapStatsForStatsPacket.update(gap);

        // update all stats used for desired frames calculations under dynamic jitter buffer mode
        _timeGapStatsForDesiredCalcOnTooManyStarves.update(gap);
        _timeGapStatsForDesiredReduction.update(gap);

        if (_timeGapStatsForDesiredCalcOnTooManyStarves.getNewStatsAvailableFlag()) {
            _calculatedJitterBufferFrames = ceilf((float)_timeGapStatsForDesiredCalcOnTooManyStarves.getWindowMax()
                                                             / (float) AudioConstants::NETWORK_FRAME_USECS);
            _timeGapStatsForDesiredCalcOnTooManyStarves.clearNewStatsAvailableFlag();
        }

        if (_dynamicJitterBufferEnabled) {
            // if the max gap in window B (_timeGapStatsForDesiredReduction) corresponds to a smaller number of frames than _desiredJitterBufferFrames,
            // then reduce _desiredJitterBufferFrames to that number of frames.
            if (_timeGapStatsForDesiredReduction.getNewStatsAvailableFlag() && _timeGapStatsForDesiredReduction.isWindowFilled()) {
                int calculatedJitterBufferFrames = ceilf((float)_timeGapStatsForDesiredReduction.getWindowMax()
                                                         / (float)AudioConstants::NETWORK_FRAME_USECS);
                if (calculatedJitterBufferFrames < _desiredJitterBufferFrames) {
                    _desiredJitterBufferFrames = calculatedJitterBufferFrames;
                    qCInfo(audiostream, "Set desired jitter frames to %d (reduced)", _desiredJitterBufferFrames);
                }
                _timeGapStatsForDesiredReduction.clearNewStatsAvailableFlag();
            }
        }
    }

    _lastPacketReceivedTime = now;
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
    streamStats._unplayedMs = (quint16)_unplayedMs.getWindowMax();
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

    const float INITIAL_FRAMES_NO_FADE = INITIAL_MSECS_NO_FADE / AudioConstants::NETWORK_FRAME_MSECS;
    const float FRAMES_FADE_TO_ZERO = MSECS_FADE_TO_ZERO / AudioConstants::NETWORK_FRAME_MSECS;

    const float SAMPLE_RANGE = std::numeric_limits<int16_t>::max();

    if (indexOfRepeat <= INITIAL_FRAMES_NO_FADE) {
        return 1.0f;
    } else if (indexOfRepeat <= INITIAL_FRAMES_NO_FADE + FRAMES_FADE_TO_ZERO) {
        return pow(SAMPLE_RANGE, -(indexOfRepeat - INITIAL_FRAMES_NO_FADE) / FRAMES_FADE_TO_ZERO);
    }
    return 0.0f;
}

void InboundAudioStream::setupCodec(CodecPluginPointer codec, const QString& codecName, int numChannels) {
    cleanupCodec(); // cleanup any previously allocated coders first
    _codec = codec;
    _selectedCodecName = codecName;
    if (_codec) {
        QMutexLocker lock(&_decoderMutex);
        _decoder = codec->createDecoder(AudioConstants::SAMPLE_RATE, numChannels);
    }
}

void InboundAudioStream::cleanupCodec() {
    // release any old codec encoder/decoder first...
    if (_codec) {
        QMutexLocker lock(&_decoderMutex);
        if (_decoder) {
            _codec->releaseDecoder(_decoder);
            _decoder = nullptr;
        }
    }
    _selectedCodecName = "";
}
