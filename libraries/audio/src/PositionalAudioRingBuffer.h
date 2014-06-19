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

class PositionalAudioRingBuffer : public AudioRingBuffer {
public:
    enum Type {
        Microphone,
        Injector
    };
    
    PositionalAudioRingBuffer(PositionalAudioRingBuffer::Type type, bool isStereo = false);
    
    int parseData(const QByteArray& packet);
    int parsePositionalData(const QByteArray& positionalByteArray);
    int parseListenModeData(const QByteArray& listenModeByteArray);
    
    void updateNextOutputTrailingLoudness();
    float getNextOutputTrailingLoudness() const { return _nextOutputTrailingLoudness; }
    
    bool shouldBeAddedToMix(int numJitterBufferSamples);
    
    bool willBeAddedToMix() const { return _willBeAddedToMix; }
    void setWillBeAddedToMix(bool willBeAddedToMix) { _willBeAddedToMix = willBeAddedToMix; }
    
    bool shouldLoopbackForNode() const { return _shouldLoopbackForNode; }
    
    bool isStereo() const { return _isStereo; }
    
    PositionalAudioRingBuffer::Type getType() const { return _type; }
    const glm::vec3& getPosition() const { return _position; }
    const glm::quat& getOrientation() const { return _orientation; }
    
    AABox* getListenerUnattenuatedZone() const { return _listenerUnattenuatedZone; }
    void setListenerUnattenuatedZone(AABox* listenerUnattenuatedZone) { _listenerUnattenuatedZone = listenerUnattenuatedZone; }
    
protected:
    // disallow copying of PositionalAudioRingBuffer objects
    PositionalAudioRingBuffer(const PositionalAudioRingBuffer&);
    PositionalAudioRingBuffer& operator= (const PositionalAudioRingBuffer&);
    
    PositionalAudioRingBuffer::Type _type;
    glm::vec3 _position;
    glm::quat _orientation;
    bool _willBeAddedToMix;
    bool _shouldLoopbackForNode;
    bool _shouldOutputStarveDebug;
    bool _isStereo;
    
    float _nextOutputTrailingLoudness;
    AABox* _listenerUnattenuatedZone;
};

#endif // hifi_PositionalAudioRingBuffer_h
