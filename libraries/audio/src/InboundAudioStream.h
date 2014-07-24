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

#include "NodeData.h"
#include "AudioRingBuffer.h"
#include "MovingMinMaxAvg.h"
#include "SequenceNumberStats.h"
#include "AudioStreamStats.h"
#include "PacketHeaders.h"

// the time gaps stats for _desiredJitterBufferFrames calculation
// will recalculate the max for the past 5000 samples every 500 samples
const int TIME_GAPS_FOR_JITTER_CALC_INTERVAL_SAMPLES = 500;
const int TIME_GAPS_FOR_JITTER_CALC_WINDOW_INTERVALS = 10;

// the time gap stats for constructing AudioStreamStats will
// recalculate min/max/avg every ~1 second for the past ~30 seconds of time gap data
const int TIME_GAPS_FOR_STATS_PACKET_INTERVAL_SAMPLES = USECS_PER_SECOND / BUFFER_SEND_INTERVAL_USECS;
const int TIME_GAPS_FOR_STATS_PACKET_WINDOW_INTERVALS = 30;

// the stats for calculating the average frames available  will recalculate every ~1 second
// and will include data for the past ~2 seconds 
const int FRAMES_AVAILABLE_STATS_INTERVAL_SAMPLES = USECS_PER_SECOND / BUFFER_SEND_INTERVAL_USECS;
const int FRAMES_AVAILABLE_STATS_WINDOW_INTERVALS = 2;

// the internal history buffer of the incoming seq stats will cover 30s to calculate
// packet loss % over last 30s
const int INCOMING_SEQ_STATS_HISTORY_LENGTH_SECONDS = 30;

const int INBOUND_RING_BUFFER_FRAME_CAPACITY = 100;


class InboundAudioStream : public NodeData {
    Q_OBJECT
public:
    InboundAudioStream(int numFrameSamples, int numFramesCapacity, bool dynamicJitterBuffers);

    void reset();
    void flushBuffer() { _ringBuffer.reset(); }
    void resetSequenceNumberStats() { _incomingSequenceNumberStats.reset(); }


    virtual int parseData(const QByteArray& packet);

    bool popFrames(int numFrames, bool starveOnFail = true);
    bool popFrames(int16_t* dest, int numFrames, bool starveOnFail = true);
    bool popFrames(AudioRingBuffer::ConstIterator* nextOutput, int numFrames, bool starveOnFail = true);

    void setToStarved();


    /// this function should be called once per second to ensure the seq num stats history spans ~30 seconds
    AudioStreamStats updateSeqHistoryAndGetAudioStreamStats();

    virtual AudioStreamStats getAudioStreamStats() const;

    int getCalculatedDesiredJitterBufferFrames() const;

    int getDesiredJitterBufferFrames() const { return _desiredJitterBufferFrames; }
    int getNumFrameSamples() const { return _ringBuffer.getNumFrameSamples(); }
    int getFramesAvailable() const { return _ringBuffer.framesAvailable(); }
    double getFramesAvailableAverage() const { return _framesAvailableStats.getWindowAverage(); }

    bool isStarved() const { return _isStarved; }
    bool hasStarted() const { return _hasStarted; }

    int getConsecutiveNotMixedCount() const { return _consecutiveNotMixedCount; }
    int getStarveCount() const { return _starveCount; }
    int getSilentFramesDropped() const { return _silentFramesDropped; }
    int getOverflowCount() const { return _ringBuffer.getOverflowCount(); }

private:
    bool shouldPop(int numSamples, bool starveOnFail);

protected:
    // disallow copying of InboundAudioStream objects
    InboundAudioStream(const InboundAudioStream&);
    InboundAudioStream& operator= (const InboundAudioStream&);

    /// parses the info between the seq num and the audio data in the network packet and calculates
    /// how many audio samples this packet contains
    virtual int parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples) = 0;

    /// parses the audio data in the network packet
    virtual int parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples) = 0;

    int writeDroppableSilentSamples(int numSilentSamples);
    int writeSamplesForDroppedPackets(int numSamples);
    void frameReceivedUpdateTimingStats();

protected:

    AudioRingBuffer _ringBuffer;

    bool _dynamicJitterBuffers;
    int _desiredJitterBufferFrames;

    bool _isStarved;
    bool _hasStarted;


    // stats

    int _consecutiveNotMixedCount;
    int _starveCount;
    int _silentFramesDropped;

    SequenceNumberStats _incomingSequenceNumberStats;

    quint64 _lastFrameReceivedTime;
    MovingMinMaxAvg<quint64> _interframeTimeGapStatsForJitterCalc;
    MovingMinMaxAvg<quint64> _interframeTimeGapStatsForStatsPacket;
    
    // TODO: change this to time-weighted moving avg
    MovingMinMaxAvg<int> _framesAvailableStats;
};

#endif // hifi_InboundAudioStream_h
