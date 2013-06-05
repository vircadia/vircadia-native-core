//
//  PositionalAudioRingBuffer.h
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__PositionalAudioRingBuffer__
#define __hifi__PositionalAudioRingBuffer__

#include <glm/gtx/quaternion.hpp>

#include <AudioRingBuffer.h>

class PositionalAudioRingBuffer : public AudioRingBuffer {
public:
    PositionalAudioRingBuffer();
    
    int parseData(unsigned char* sourceBuffer, int numBytes);
    int parsePositionalData(unsigned char* sourceBuffer, int numBytes);
    
    bool shouldBeAddedToMix(int numJitterBufferSamples);
    
    bool wasAddedToMix() const { return _wasAddedToMix; }
    void setWasAddedToMix(bool wasAddedToMix) { _wasAddedToMix = wasAddedToMix; }
    
    const glm::vec3& getPosition() const { return _position; }
    const glm::quat& getOrientation() const { return _orientation; }
    
    bool shouldLoopbackForAgent() const { return _shouldLoopbackForAgent; }
    
protected:
    // disallow copying of PositionalAudioRingBuffer objects
    PositionalAudioRingBuffer(const PositionalAudioRingBuffer&);
    PositionalAudioRingBuffer& operator= (const PositionalAudioRingBuffer&);
    
    glm::vec3 _position;
    glm::quat _orientation;
    bool _shouldLoopbackForAgent;
    bool _wasAddedToMix;
};

#endif /* defined(__hifi__PositionalAudioRingBuffer__) */
