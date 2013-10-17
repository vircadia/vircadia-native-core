//
//  AudioInjector.h
//  hifi
//
//  Created by Stephen Birarda on 4/23/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__AudioInjector__
#define __hifi__AudioInjector__

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/component_wise.hpp>

#include <QtCore/QObject>

#include <RegisteredMetaTypes.h>
#include <UDPSocket.h>

#include "AudioRingBuffer.h"

const int MAX_INJECTOR_VOLUME = 0xFF;

const int INJECT_INTERVAL_USECS = floorf((BUFFER_LENGTH_SAMPLES_PER_CHANNEL / SAMPLE_RATE) * 1000000);

class AudioInjector : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(glm::vec3 position READ getPosition WRITE setPosition)
    Q_PROPERTY(uchar volume READ getVolume WRITE setVolume);
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
    void setOrientation(const glm::quat& orientation) { _orientation = orientation; }
    
    float getRadius() const { return _radius; }
    void setRadius(float radius) { _radius = radius; }
    
    bool hasSamplesToInject() const { return _indexOfNextSlot > 0; }
    
    void addSample(const int16_t sample);
    void addSamples(int16_t* sampleBuffer, int numSamples);
    
    void clear();
public slots:
    int16_t& sampleAt(const int index);
    void insertSample(const int index, int sample);
private:
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
