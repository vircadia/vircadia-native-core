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

const int BUFFER_LENGTH_BYTES = 512;
const int BUFFER_LENGTH_SAMPLES = BUFFER_LENGTH_BYTES / sizeof(int16_t);
const float SAMPLE_RATE = 22050.0f;
const float BUFFER_SEND_INTERVAL_USECS = (BUFFER_LENGTH_SAMPLES / SAMPLE_RATE) * 1000000;

class AudioInjector {
    friend class AudioInjectionManager;
    
public:
    AudioInjector(const char* filename);
    AudioInjector(int maxNumSamples);
    ~AudioInjector();

    void injectAudio(UDPSocket* injectorSocket, sockaddr* destinationSocket);
    
    bool isInjectingAudio() const { return _isInjectingAudio; }
    void setIsInjectingAudio(bool isInjectingAudio) { _isInjectingAudio = isInjectingAudio; }
    
    unsigned char getVolume() const  { return _volume; }
    void setVolume(unsigned char volume) { _volume = volume; }
    
    const glm::vec3& getPosition() const { return _position; }
    void setPosition(const glm::vec3& position) { _position = position; }
    
    float getBearing() const { return _bearing; }
    void setBearing(float bearing) { _bearing = bearing; }
    
    void addSample(const int16_t sample);
    void addSamples(int16_t* sampleBuffer, int numSamples);
private:
    int16_t* _audioSampleArray;
    int _numTotalSamples;
    glm::vec3 _position;
    float _bearing;
    unsigned char _volume;
    int _indexOfNextSlot;
    bool _isInjectingAudio;
};

#endif /* defined(__hifi__AudioInjector__) */
