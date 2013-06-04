//
//  AudioInjector.h
//  hifi
//
//  Created by Stephen Birarda on 4/23/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__AudioInjector__
#define __hifi__AudioInjector__

#include <iostream>

#include <glm/glm.hpp>

#include "AudioRingBuffer.h"

const int BUFFER_LENGTH_BYTES = 512;
const int BUFFER_LENGTH_SAMPLES = BUFFER_LENGTH_BYTES / sizeof(int16_t);
const float SAMPLE_RATE = 22050.0f;
const float BUFFER_SEND_INTERVAL_USECS = (BUFFER_LENGTH_SAMPLES / SAMPLE_RATE) * 1000000;

class AudioInjector {
public:
    AudioInjector(const char* filename);
    AudioInjector(int maxNumSamples);
    ~AudioInjector();

    void injectAudio(UDPSocket* injectorSocket, sockaddr* destinationSocket);
    
    bool isInjectingAudio() const { return _isInjectingAudio; }
    
    unsigned char getVolume() const  { return _volume; }
    void setVolume(unsigned char volume) { _volume = volume; }
    
    const glm::vec3& getPosition() const { return _position; }
    void setPosition(const glm::vec3& position) { _position = position; }
    
    const glm::quat& getOrientation() const { return _orientation; }
    void setOperation(const glm::quat& orientation) { _orientation = orientation; }
    
    float getRadius() const { return _radius; }
    void setRadius(float radius) { _radius = radius; }
    
    void addSample(const int16_t sample);
    void addSamples(int16_t* sampleBuffer, int numSamples);
private:
    unsigned char _streamIdentifier[STREAM_IDENTIFIER_NUM_BYTES];
    int16_t* _audioSampleArray;
    int _numTotalSamples;
    glm::vec3 _position;
    glm::quat _orientation;
    float _radius;
    unsigned char _volume;
    int _indexOfNextSlot;
    bool _isInjectingAudio;
};

#endif /* defined(__hifi__AudioInjector__) */
