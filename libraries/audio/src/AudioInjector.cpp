//
//  AudioInjector.cpp
//  hifi
//
//  Created by Stephen Birarda on 4/23/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <cstring>
#include <fstream>
#include <limits>

#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "AudioInjector.h"

AudioInjector::AudioInjector(int maxNumSamples) :
    _streamIdentifier(QUuid::createUuid()),
    _numTotalSamples(maxNumSamples),
    _position(0.0f, 0.0f, 0.0f),
    _orientation(),
    _radius(0.0f),
    _volume(MAX_INJECTOR_VOLUME),
    _indexOfNextSlot(0),
    _isInjectingAudio(false)
{
    _audioSampleArray = new int16_t[maxNumSamples];
    memset(_audioSampleArray, 0, _numTotalSamples * sizeof(int16_t));
}

AudioInjector::~AudioInjector() {
    delete[] _audioSampleArray;
}

void AudioInjector::injectAudio(UDPSocket* injectorSocket, sockaddr* destinationSocket) {
    if (_audioSampleArray && _indexOfNextSlot > 0) {
        _isInjectingAudio = true;
        
        timeval startTime;
        
        // calculate the number of bytes required for additional data
        int leadingBytes = numBytesForPacketHeader((unsigned char*) &PACKET_TYPE_INJECT_AUDIO)
            + NUM_BYTES_RFC4122_UUID
            + NUM_BYTES_RFC4122_UUID
            + sizeof(_position)
            + sizeof(_orientation)
            + sizeof(_radius)
            + sizeof(_volume);
        
        unsigned char dataPacket[(BUFFER_LENGTH_SAMPLES_PER_CHANNEL * sizeof(int16_t)) + leadingBytes];
        
        unsigned char* currentPacketPtr = dataPacket + populateTypeAndVersion(dataPacket, PACKET_TYPE_INJECT_AUDIO);
        
        // copy the UUID for the owning node
        QByteArray rfcUUID = NodeList::getInstance()->getOwnerUUID().toRfc4122();
        memcpy(currentPacketPtr, rfcUUID.constData(), rfcUUID.size());
        currentPacketPtr += rfcUUID.size();
        
        // copy the stream identifier
        QByteArray rfcStreamIdentifier = _streamIdentifier.toRfc4122();
        memcpy(currentPacketPtr, rfcStreamIdentifier.constData(), rfcStreamIdentifier.size());
        currentPacketPtr += rfcStreamIdentifier.size();
        
        memcpy(currentPacketPtr, &_position, sizeof(_position));
        currentPacketPtr += sizeof(_position);
        
        memcpy(currentPacketPtr, &_orientation, sizeof(_orientation));
        currentPacketPtr += sizeof(_orientation);
        
        memcpy(currentPacketPtr, &_radius, sizeof(_radius));
        currentPacketPtr += sizeof(_radius);
        
        *currentPacketPtr = _volume;
        currentPacketPtr++;        
        
        gettimeofday(&startTime, NULL);
        int nextFrame = 0;
        
        for (int i = 0; i < _numTotalSamples; i += BUFFER_LENGTH_SAMPLES_PER_CHANNEL) {
            int usecToSleep = usecTimestamp(&startTime) + (nextFrame++ * INJECT_INTERVAL_USECS) - usecTimestampNow();
            if (usecToSleep > 0) {
                usleep(usecToSleep);
            }
            
            int numSamplesToCopy = BUFFER_LENGTH_SAMPLES_PER_CHANNEL;
            
            if (_numTotalSamples - i < BUFFER_LENGTH_SAMPLES_PER_CHANNEL) {
                numSamplesToCopy = _numTotalSamples - i;
                memset(currentPacketPtr + numSamplesToCopy,
                       0,
                       BUFFER_LENGTH_BYTES_PER_CHANNEL - (numSamplesToCopy * sizeof(int16_t)));
            }
            
            memcpy(currentPacketPtr, _audioSampleArray + i, numSamplesToCopy * sizeof(int16_t));
            
            injectorSocket->send(destinationSocket, dataPacket, sizeof(dataPacket));
        }
        
        _isInjectingAudio = false;
    }
}

void AudioInjector::addSample(const int16_t sample) {
    if (_indexOfNextSlot != _numTotalSamples) {
        // only add this sample if we actually have space for it
        _audioSampleArray[_indexOfNextSlot++] = sample;
    }
}

void AudioInjector::addSamples(int16_t* sampleBuffer, int numSamples) {    
    if (_audioSampleArray + _indexOfNextSlot + numSamples <= _audioSampleArray + _numTotalSamples) {
        // only copy the audio from the sample buffer if there's space
        memcpy(_audioSampleArray + _indexOfNextSlot, sampleBuffer, numSamples * sizeof(int16_t));
        _indexOfNextSlot += numSamples;
    }
}

void AudioInjector::clear() {
    _indexOfNextSlot = 0;
    memset(_audioSampleArray, 0, _numTotalSamples * sizeof(int16_t));
}

int16_t& AudioInjector::sampleAt(const int index) {
    assert(index >= 0 && index < _numTotalSamples);
    
    return _audioSampleArray[index];
}

void AudioInjector::insertSample(const int index, int sample) {
    assert (index >= 0 && index < _numTotalSamples);
    
    _audioSampleArray[index] = (int16_t) sample;
    _indexOfNextSlot = index + 1;
}
