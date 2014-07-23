//
//  PositionalAudioRingBuffer.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PositionalAudioRingBuffer_h
#define hifi_PositionalAudioRingBuffer_h

#include <glm/gtx/quaternion.hpp>

#include <AABox.h>

#include "AudioRingBuffer.h"
#include "MovingMinMaxAvg.h"

// the time gaps stats for _desiredJitterBufferFrames calculation
// will recalculate the max for the past 5000 samples every 500 samples
const int TIME_GAPS_FOR_JITTER_CALC_INTERVAL_SAMPLES = 500;
const int TIME_GAPS_FOR_JITTER_CALC_WINDOW_INTERVALS = 10;

// the time gap stats for constructing AudioStreamStats will
// recalculate min/max/avg every ~1 second for the past ~30 seconds of time gap data
const int TIME_GAPS_FOR_STATS_PACKET_INTERVAL_SAMPLES = USECS_PER_SECOND / BUFFER_SEND_INTERVAL_USECS;
const int TIME_GAPS_FOR_STATS_PACKET_WINDOW_INTERVALS = 30;

// the stats for calculating the average frames available  will recalculate every ~1 second
// and will include data for the past ~10 seconds 
const int FRAMES_AVAILABLE_STATS_INTERVAL_SAMPLES = USECS_PER_SECOND / BUFFER_SEND_INTERVAL_USECS;
const int FRAMES_AVAILABLE_STATS_WINDOW_INTERVALS = 10;

const int AUDIOMIXER_INBOUND_RING_BUFFER_FRAME_CAPACITY = 100;

class PositionalAudioRingBuffer : public AudioRingBuffer {
public:
    enum Type {
        Microphone,
        Injector
    };
    
    PositionalAudioRingBuffer(PositionalAudioRingBuffer::Type type, bool isStereo = false, bool dynamicJitterBuffers = false);
    
    virtual int parseDataAndHandleDroppedPackets(const QByteArray& packet, int packetsSkipped) = 0;

    int parsePositionalData(const QByteArray& positionalByteArray);
    int parseListenModeData(const QByteArray& listenModeByteArray);
    
    void updateNextOutputTrailingLoudness();
    float getNextOutputTrailingLoudness() const { return _nextOutputTrailingLoudness; }
    
    bool shouldBeAddedToMix();
    
    bool willBeAddedToMix() const { return _willBeAddedToMix; }
    void setWillBeAddedToMix(bool willBeAddedToMix) { _willBeAddedToMix = willBeAddedToMix; }
    
    bool shouldLoopbackForNode() const { return _shouldLoopbackForNode; }
    
    bool isStereo() const { return _isStereo; }
    
    PositionalAudioRingBuffer::Type getType() const { return _type; }
    const glm::vec3& getPosition() const { return _position; }
    const glm::quat& getOrientation() const { return _orientation; }
    
    AABox* getListenerUnattenuatedZone() const { return _listenerUnattenuatedZone; }
    void setListenerUnattenuatedZone(AABox* listenerUnattenuatedZone) { _listenerUnattenuatedZone = listenerUnattenuatedZone; }
    
    int getSamplesPerFrame() const { return _isStereo ? NETWORK_BUFFER_LENGTH_SAMPLES_STEREO : NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL; }

    const MovingMinMaxAvg<quint64>& getInterframeTimeGapStatsForStatsPacket() const { return _interframeTimeGapStatsForStatsPacket; }

    int getCalculatedDesiredJitterBufferFrames() const; /// returns what we would calculate our desired as if asked
    int getDesiredJitterBufferFrames() const { return _desiredJitterBufferFrames; }
    double getFramesAvailableAverage() const { return _framesAvailableStats.getWindowAverage(); }

    int getConsecutiveNotMixedCount() const { return _consecutiveNotMixedCount; }
    int getStarveCount() const { return _starveCount; }
    int getSilentFramesDropped() const { return _silentFramesDropped; }

protected:
    // disallow copying of PositionalAudioRingBuffer objects
    PositionalAudioRingBuffer(const PositionalAudioRingBuffer&);
    PositionalAudioRingBuffer& operator= (const PositionalAudioRingBuffer&);

    void frameReceivedUpdateTimingStats();
    void addDroppableSilentSamples(int numSilentSamples);
    
    PositionalAudioRingBuffer::Type _type;
    glm::vec3 _position;
    glm::quat _orientation;
    bool _willBeAddedToMix;
    bool _shouldLoopbackForNode;
    bool _shouldOutputStarveDebug;
    bool _isStereo;
    
    float _nextOutputTrailingLoudness;
    AABox* _listenerUnattenuatedZone;

    quint64 _lastFrameReceivedTime;
    MovingMinMaxAvg<quint64> _interframeTimeGapStatsForJitterCalc;
    MovingMinMaxAvg<quint64> _interframeTimeGapStatsForStatsPacket;
    MovingMinMaxAvg<int> _framesAvailableStats;
    
    int _desiredJitterBufferFrames;
    bool _dynamicJitterBuffers;

    // extra stats
    int _consecutiveNotMixedCount;
    int _starveCount;
    int _silentFramesDropped;
};

#endif // hifi_PositionalAudioRingBuffer_h
