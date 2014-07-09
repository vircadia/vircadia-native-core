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

// this means that every 500 samples, the max for the past 10*500 samples will be calculated
const int TIME_GAP_NUM_SAMPLES_IN_INTERVAL = 500;
const int TIME_GAP_NUM_INTERVALS_IN_WINDOW = 10;

// class used to track time between incoming frames for the purpose of varying the jitter buffer length
class InterframeTimeGapStats {
public:
    InterframeTimeGapStats();

    void frameReceived();
    bool hasNewWindowMaxGapAvailable() const { return _newWindowMaxGapAvailable; }
    quint64 peekWindowMaxGap() const { return _windowMaxGap; }
    quint64 getWindowMaxGap();

private:
    quint64 _lastFrameReceivedTime;

    int _numSamplesInCurrentInterval;
    quint64 _currentIntervalMaxGap;
    quint64 _intervalMaxGaps[TIME_GAP_NUM_INTERVALS_IN_WINDOW];
    int _newestIntervalMaxGapAt;
    quint64 _windowMaxGap;
    bool _newWindowMaxGapAvailable;
};

const int AUDIOMIXER_INBOUND_RING_BUFFER_FRAME_CAPACITY = 100;

class PositionalAudioRingBuffer : public AudioRingBuffer {
public:
    enum Type {
        Microphone,
        Injector
    };
    
    PositionalAudioRingBuffer(PositionalAudioRingBuffer::Type type, bool isStereo = false, bool dynamicJitterBuffers = false);
    
    int parseData(const QByteArray& packet);
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

    int getCalculatedDesiredJitterBufferFrames() const; /// returns what we would calculate our desired as if asked
    int getDesiredJitterBufferFrames() const { return _desiredJitterBufferFrames; }
    int getCurrentJitterBufferFrames() const { return _currentJitterBufferFrames; }

    int getConsecutiveNotMixedCount() const { return _consecutiveNotMixedCount; }

protected:
    // disallow copying of PositionalAudioRingBuffer objects
    PositionalAudioRingBuffer(const PositionalAudioRingBuffer&);
    PositionalAudioRingBuffer& operator= (const PositionalAudioRingBuffer&);

    void updateDesiredJitterBufferFrames();
    
    PositionalAudioRingBuffer::Type _type;
    glm::vec3 _position;
    glm::quat _orientation;
    bool _willBeAddedToMix;
    bool _shouldLoopbackForNode;
    bool _shouldOutputStarveDebug;
    bool _isStereo;
    
    float _nextOutputTrailingLoudness;
    AABox* _listenerUnattenuatedZone;

    InterframeTimeGapStats _interframeTimeGapStats;
    int _desiredJitterBufferFrames;
    int _currentJitterBufferFrames;
    bool _dynamicJitterBuffers;

    // extra stats
    int _consecutiveNotMixedCount;
};

#endif // hifi_PositionalAudioRingBuffer_h
