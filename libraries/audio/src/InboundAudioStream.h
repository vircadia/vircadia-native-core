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
#include "TimeWeightedAvg.h"

// This adds some number of frames to the desired jitter buffer frames target we use when we're dropping frames.
// The larger this value is, the less aggressive we are about reducing the jitter buffer length.
// Setting this to 0 will try to get the jitter buffer to be exactly _desiredJitterBufferFrames long when dropping frames,
// which could lead to a starve soon after.
const int DESIRED_JITTER_BUFFER_FRAMES_PADDING = 1;

// the time gaps stats for _desiredJitterBufferFrames calculation
// will recalculate the max for the past 5000 samples every 500 samples
const int TIME_GAPS_FOR_JITTER_CALC_INTERVAL_SAMPLES = 500;
const int TIME_GAPS_FOR_JITTER_CALC_WINDOW_INTERVALS = 10;

// the time gap stats for constructing AudioStreamStats will
// recalculate min/max/avg every ~1 second for the past ~30 seconds of time gap data
const int TIME_GAPS_FOR_STATS_PACKET_INTERVAL_SAMPLES = USECS_PER_SECOND / BUFFER_SEND_INTERVAL_USECS;
const int TIME_GAPS_FOR_STATS_PACKET_WINDOW_INTERVALS = 30;

// this controls the window size of the time-weighted avg of frames available.  Every time the window fills up,
// _currentJitterBufferFrames is updated with the time-weighted avg and the running time-weighted avg is reset.
const int FRAMES_AVAILABLE_STAT_WINDOW_USECS = 2 * USECS_PER_SECOND;

// the internal history buffer of the incoming seq stats will cover 30s to calculate
// packet loss % over last 30s
const int INCOMING_SEQ_STATS_HISTORY_LENGTH_SECONDS = 30;

const int INBOUND_RING_BUFFER_FRAME_CAPACITY = 100;

const int DEFAULT_MAX_FRAMES_OVER_DESIRED = 10;
const int DEFAULT_DESIRED_JITTER_BUFFER_FRAMES = 1;

class InboundAudioStream : public NodeData {
    Q_OBJECT
public:
    InboundAudioStream(int numFrameSamples, int numFramesCapacity,
        bool dynamicJitterBuffers, int staticDesiredJitterBufferFrames, int maxFramesOverDesired,
        bool useStDevForJitterCalc = false);

    void reset();
    void resetStats();
    void clearBuffer();

    virtual int parseData(const QByteArray& packet);

    int popFrames(int maxFrames, bool allOrNothing, bool starveIfNoFramesPopped = true);
    int popSamples(int maxSamples, bool allOrNothing, bool starveIfNoSamplesPopped = true);

    bool lastPopSucceeded() const { return _lastPopSucceeded; };
    const AudioRingBuffer::ConstIterator& getLastPopOutput() const { return _lastPopOutput; }


    void setToStarved();

    
    void setDynamicJitterBuffers(bool dynamicJitterBuffers);
    void setStaticDesiredJitterBufferFrames(int staticDesiredJitterBufferFrames);

    /// this function should be called once per second to ensure the seq num stats history spans ~30 seconds
    AudioStreamStats updateSeqHistoryAndGetAudioStreamStats();

    void setMaxFramesOverDesired(int maxFramesOverDesired) { _maxFramesOverDesired = maxFramesOverDesired; }

    virtual AudioStreamStats getAudioStreamStats() const;

    /// returns the desired number of jitter buffer frames under the dyanmic jitter buffers scheme
    int getCalculatedJitterBufferFrames() const { return _useStDevForJitterCalc ? 
        _calculatedJitterBufferFramesUsingStDev : _calculatedJitterBufferFramesUsingMaxGap; };

    /// returns the desired number of jitter buffer frames using Philip's method
    int getCalculatedJitterBufferFramesUsingStDev() const { return _calculatedJitterBufferFramesUsingStDev; }

    /// returns the desired number of jitter buffer frames using Freddy's method
    int getCalculatedJitterBufferFramesUsingMaxGap() const { return _calculatedJitterBufferFramesUsingMaxGap; }

    int getDesiredJitterBufferFrames() const { return _desiredJitterBufferFrames; }
    int getMaxFramesOverDesired() const { return _maxFramesOverDesired; }
    int getNumFrameSamples() const { return _ringBuffer.getNumFrameSamples(); }
    int getFrameCapacity() const { return _ringBuffer.getFrameCapacity(); }
    int getFramesAvailable() const { return _ringBuffer.framesAvailable(); }
    double getFramesAvailableAverage() const { return _framesAvailableStat.getAverage(); }

    bool isStarved() const { return _isStarved; }
    bool hasStarted() const { return _hasStarted; }

    int getConsecutiveNotMixedCount() const { return _consecutiveNotMixedCount; }
    int getStarveCount() const { return _starveCount; }
    int getSilentFramesDropped() const { return _silentFramesDropped; }
    int getOverflowCount() const { return _ringBuffer.getOverflowCount(); }

    int getPacketsReceived() const { return _incomingSequenceNumberStats.getReceived(); }

private:
    void frameReceivedUpdateTimingStats();
    int clampDesiredJitterBufferFramesValue(int desired) const;

    int writeSamplesForDroppedPackets(int numSamples);

    void popSamplesNoCheck(int samples);
    void framesAvailableChanged();

protected:
    // disallow copying of InboundAudioStream objects
    InboundAudioStream(const InboundAudioStream&);
    InboundAudioStream& operator= (const InboundAudioStream&);

    /// parses the info between the seq num and the audio data in the network packet and calculates
    /// how many audio samples this packet contains (used when filling in samples for dropped packets).
    virtual int parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples) = 0;

    /// parses the audio data in the network packet.
    /// default implementation assumes packet contains raw audio samples after stream properties
    virtual int parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples);

    int writeDroppableSilentSamples(int numSilentSamples);
    
protected:

    AudioRingBuffer _ringBuffer;

    bool _lastPopSucceeded;
    AudioRingBuffer::ConstIterator _lastPopOutput;

    bool _dynamicJitterBuffers;         // if false, _desiredJitterBufferFrames is locked at 1 (old behavior)
    int _staticDesiredJitterBufferFrames;

    // if jitter buffer is dynamic, this determines what method of calculating _desiredJitterBufferFrames 
    // if true, Philip's timegap std dev calculation is used.  Otherwise, Freddy's max timegap calculation is used
    bool _useStDevForJitterCalc;
    int _calculatedJitterBufferFramesUsingMaxGap;
    int _calculatedJitterBufferFramesUsingStDev;

    int _desiredJitterBufferFrames;

    // if there are more than _desiredJitterBufferFrames + _maxFramesOverDesired frames, old ringbuffer frames
    // will be dropped to keep audio delay from building up
    int _maxFramesOverDesired;

    bool _isStarved;
    bool _hasStarted;

    // stats

    int _consecutiveNotMixedCount;
    int _starveCount;
    int _silentFramesDropped;
    int _oldFramesDropped;

    SequenceNumberStats _incomingSequenceNumberStats;

    quint64 _lastFrameReceivedTime;
    MovingMinMaxAvg<quint64> _interframeTimeGapStatsForJitterCalc;
    StDev _stdev;
    MovingMinMaxAvg<quint64> _interframeTimeGapStatsForStatsPacket;
    
    TimeWeightedAvg<int> _framesAvailableStat;

    // this value is based on the time-weighted avg from _framesAvailableStat. it is only used for
    // dropping silent frames right now.
    int _currentJitterBufferFrames;
};

#endif // hifi_InboundAudioStream_h
