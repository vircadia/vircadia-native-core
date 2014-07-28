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
#include "StdDev.h"

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
const int FRAMES_AVAILABLE_STATS_WINDOW_INTERVALS = 10;

// the internal history buffer of the incoming seq stats will cover 30s to calculate
// packet loss % over last 30s
const int INCOMING_SEQ_STATS_HISTORY_LENGTH_SECONDS = 30;

const int INBOUND_RING_BUFFER_FRAME_CAPACITY = 100;


class InboundAudioStream : public NodeData {
    Q_OBJECT
public:
    InboundAudioStream(int numFrameSamples, int numFramesCapacity, bool dynamicJitterBuffers, bool useStDevForJitterCalc = false);

    void reset();
    void resetStats();
    void clearBuffer();

    virtual int parseData(const QByteArray& packet);


    bool popFrames(int numFrames, bool starveOnFail = true);

    bool lastPopSucceeded() const { return _lastPopSucceeded; };
    const AudioRingBuffer::ConstIterator& getLastPopOutput() const { return _lastPopOutput; }


    void setToStarved();

    /// turns off dyanmic jitter buffers and sets the desired jitter buffer frames to specified value
    void overrideDesiredJitterBufferFramesTo(int desired);
    void unoverrideDesiredJitterBufferFrames();

    /// this function should be called once per second to ensure the seq num stats history spans ~30 seconds
    AudioStreamStats updateSeqHistoryAndGetAudioStreamStats();

    virtual AudioStreamStats getAudioStreamStats() const;

    /// returns the desired number of jitter buffer frames under the dyanmic jitter buffers scheme
    int getCalculatedJitterBufferFrames() const { return _useStDevForJitterCalc ? 
        _calculatedJitterBufferFramesUsingStDev : _calculatedJitterBufferFramesUsingMaxGap; };

    /// returns the desired number of jitter buffer frames using Philip's method
    int getCalculatedJitterBufferFramesUsingStDev() const { return _calculatedJitterBufferFramesUsingStDev; }

    /// returns the desired number of jitter buffer frames using Freddy's method
    int getCalculatedJitterBufferFramesUsingMaxGap() const { return _calculatedJitterBufferFramesUsingMaxGap; }

    int getDesiredJitterBufferFrames() const { return _desiredJitterBufferFrames; }
    int getNumFrameSamples() const { return _ringBuffer.getNumFrameSamples(); }
    int getFrameCapacity() const { return _ringBuffer.getFrameCapacity(); }
    int getFramesAvailable() const { return _ringBuffer.framesAvailable(); }
    double getFramesAvailableAverage() const { return _framesAvailableStats.getWindowAverage(); }

    bool isStarved() const { return _isStarved; }
    bool hasStarted() const { return _hasStarted; }

    int getConsecutiveNotMixedCount() const { return _consecutiveNotMixedCount; }
    int getStarveCount() const { return _starveCount; }
    int getSilentFramesDropped() const { return _silentFramesDropped; }
    int getOverflowCount() const { return _ringBuffer.getOverflowCount(); }

private:
    void starved();

    SequenceNumberStats::ArrivalInfo frameReceivedUpdateNetworkStats(quint16 sequenceNumber, const QUuid& senderUUID);
    int clampDesiredJitterBufferFramesValue(int desired) const;

    int writeSamplesForDroppedPackets(int numSamples);

protected:
    // disallow copying of InboundAudioStream objects
    InboundAudioStream(const InboundAudioStream&);
    InboundAudioStream& operator= (const InboundAudioStream&);

    /// parses the info between the seq num and the audio data in the network packet and calculates
    /// how many audio samples this packet contains
    virtual int parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples) = 0;

    /// parses the audio data in the network packet
    virtual int parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples);

    int writeDroppableSilentSamples(int numSilentSamples);
    
protected:

    AudioRingBuffer _ringBuffer;

    bool _lastPopSucceeded;
    AudioRingBuffer::ConstIterator _lastPopOutput;

    bool _dynamicJitterBuffers;
    bool _dynamicJitterBuffersOverride;
    bool _useStDevForJitterCalc;
    
    int _calculatedJitterBufferFramesUsingMaxGap;
    int _calculatedJitterBufferFramesUsingStDev;
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
    StDev _stdev;
    MovingMinMaxAvg<quint64> _interframeTimeGapStatsForStatsPacket;
    
    // TODO: change this to time-weighted moving avg
    MovingMinMaxAvg<int> _framesAvailableStats;
};

#endif // hifi_InboundAudioStream_h
