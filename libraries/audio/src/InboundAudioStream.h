//
//  InboundAudioStream.h
//  libraries/audio/src
//
//  Created by Yixin Wang on 7/17/2014.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_InboundAudioStream_h
#define hifi_InboundAudioStream_h

#include <Node.h>
#include <NodeData.h>
#include <NumericalConstants.h>
#include <udt/PacketHeaders.h>
#include <ReceivedMessage.h>
#include <StDev.h>

#include <plugins/CodecPlugin.h>

#include "AudioRingBuffer.h"
#include "MovingMinMaxAvg.h"
#include "SequenceNumberStats.h"
#include "AudioStreamStats.h"
#include "TimeWeightedAvg.h"

// Audio Env bitset
const int HAS_REVERB_BIT = 0; // 1st bit

class InboundAudioStream : public NodeData {
    Q_OBJECT

public:
    // settings
    static const bool DEFAULT_DYNAMIC_JITTER_BUFFER_ENABLED;
    static const int DEFAULT_STATIC_JITTER_FRAMES;
    // legacy (now static) settings
    static const int MAX_FRAMES_OVER_DESIRED;
    static const int WINDOW_STARVE_THRESHOLD;
    static const int WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES;
    static const int WINDOW_SECONDS_FOR_DESIRED_REDUCTION;
    // unused (eradicated) settings
    static const bool USE_STDEV_FOR_JITTER;
    static const bool REPETITION_WITH_FADE;

    InboundAudioStream() = delete;
    InboundAudioStream(int numChannels, int numFrames, int numBlocks, int numStaticJitterBlocks);
    ~InboundAudioStream();

    void reset();
    virtual void resetStats();
    void clearBuffer();

    virtual int parseData(ReceivedMessage& packet) override;

    int popFrames(int maxFrames, bool allOrNothing);
    int popSamples(int maxSamples, bool allOrNothing);

    bool lastPopSucceeded() const { return _lastPopSucceeded; };
    const AudioRingBuffer::ConstIterator& getLastPopOutput() const { return _lastPopOutput; }

    quint64 usecsSinceLastPacket() { return usecTimestampNow() - _lastPacketReceivedTime; }

    void setToStarved();

    void setDynamicJitterBufferEnabled(bool enable);
    void setStaticJitterBufferFrames(int staticJitterBufferFrames);

    virtual AudioStreamStats getAudioStreamStats() const;

    /// returns the desired number of jitter buffer frames under the dyanmic jitter buffers scheme
    int getCalculatedJitterBufferFrames() const { return _calculatedJitterBufferFrames; }
    
    bool dynamicJitterBufferEnabled() const { return _dynamicJitterBufferEnabled; }
    int getStaticJitterBufferFrames() { return _staticJitterBufferFrames; }
    int getDesiredJitterBufferFrames() { return _desiredJitterBufferFrames; }

    int getNumFrameSamples() const { return _ringBuffer.getNumFrameSamples(); }
    int getFrameCapacity() const { return _ringBuffer.getFrameCapacity(); }
    int getFramesAvailable() const { return _ringBuffer.framesAvailable(); }
    double getFramesAvailableAverage() const { return _framesAvailableStat.getAverage(); }
    int getSamplesAvailable() const { return _ringBuffer.samplesAvailable(); }

    bool isStarved() const { return _isStarved; }
    bool hasStarted() const { return _hasStarted; }

    int getConsecutiveNotMixedCount() const { return _consecutiveNotMixedCount; }
    int getStarveCount() const { return _starveCount; }
    int getSilentFramesDropped() const { return _silentFramesDropped; }
    int getOverflowCount() const { return _ringBuffer.getOverflowCount(); }

    int getPacketsReceived() const { return _incomingSequenceNumberStats.getReceived(); }
    
    bool hasReverb() const { return _hasReverb; }
    float getRevebTime() const { return _reverbTime; }
    float getWetLevel() const { return _wetLevel; }
    void setReverb(float reverbTime, float wetLevel);
    void clearReverb() { _hasReverb = false; }

    void setupCodec(CodecPluginPointer codec, const QString& codecName, int numChannels);
    void cleanupCodec();

signals:
    void mismatchedAudioCodec(SharedNodePointer sendingNode, const QString& currentCodec, const QString& recievedCodec);

public slots:
    /// This function should be called every second for all the stats to function properly. If dynamic jitter buffers
    /// is enabled, those stats are used to calculate _desiredJitterBufferFrames.
    /// If the stats are not used and dynamic jitter buffers is disabled, it's not necessary to call this function.
    void perSecondCallbackForUpdatingStats();

private:
    void packetReceivedUpdateTimingStats();

    void popSamplesNoCheck(int samples);
    void framesAvailableChanged();

protected:
    // disallow copying of InboundAudioStream objects
    InboundAudioStream(const InboundAudioStream&);
    InboundAudioStream& operator= (const InboundAudioStream&);

    /// parses the info between the seq num and the audio data in the network packet and calculates
    /// how many audio samples this packet contains (used when filling in samples for dropped packets).
    /// default implementation assumes no stream properties and raw audio samples after stream propertiess
    virtual int parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& networkSamples);

    /// parses the audio data in the network packet.
    /// default implementation assumes packet contains raw audio samples after stream properties
    virtual int parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties);

    /// produces audio data for lost network packets.
    virtual int lostAudioData(int numPackets);

    /// writes silent frames to the buffer that may be dropped to reduce latency caused by the buffer
    virtual int writeDroppableSilentFrames(int silentFrames);
    
protected:

    AudioRingBuffer _ringBuffer;
    int _numChannels;

    bool _lastPopSucceeded { false };
    AudioRingBuffer::ConstIterator _lastPopOutput;
    
    bool _dynamicJitterBufferEnabled { DEFAULT_DYNAMIC_JITTER_BUFFER_ENABLED };
    int _staticJitterBufferFrames { DEFAULT_STATIC_JITTER_FRAMES };
    int _desiredJitterBufferFrames;

    bool _isStarved { true };
    bool _hasStarted { false };

    // stats

    int _consecutiveNotMixedCount { 0 };
    int _starveCount { 0 };
    int _silentFramesDropped { 0 };
    int _oldFramesDropped { 0 };

    SequenceNumberStats _incomingSequenceNumberStats;

    quint64 _lastPacketReceivedTime { 0 };
    MovingMinMaxAvg<quint64> _timeGapStatsForDesiredCalcOnTooManyStarves { 0, WINDOW_SECONDS_FOR_DESIRED_CALC_ON_TOO_MANY_STARVES };
    int _calculatedJitterBufferFrames { 0 };
    MovingMinMaxAvg<quint64> _timeGapStatsForDesiredReduction { 0, WINDOW_SECONDS_FOR_DESIRED_REDUCTION };

    RingBufferHistory<quint64> _starveHistory;

    TimeWeightedAvg<int> _framesAvailableStat;
    MovingMinMaxAvg<float> _unplayedMs;

    // this value is periodically updated with the time-weighted avg from _framesAvailableStat. it is only used for
    // dropping silent frames right now.
    int _currentJitterBufferFrames { 0 };

    MovingMinMaxAvg<quint64> _timeGapStatsForStatsPacket;

    // Reverb properties
    bool _hasReverb { false };
    float _reverbTime { 0.0f };
    float _wetLevel { 0.0f };

    CodecPluginPointer _codec;
    QString _selectedCodecName;
    Decoder* _decoder { nullptr };
};

float calculateRepeatedFrameFadeFactor(int indexOfRepeat);

#endif // hifi_InboundAudioStream_h
