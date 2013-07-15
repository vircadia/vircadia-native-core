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

#include "PacketHeaders.h"
#include "SharedUtil.h"

#include "AudioInjector.h"

AudioInjector::AudioInjector(const char* filename) :
    _position(0.0f, 0.0f, 0.0f),
    _orientation(0.0f, 0.0f, 0.0f, 0.0f),
    _radius(0.0f),
    _volume(MAX_INJECTOR_VOLUME),
    _indexOfNextSlot(0),
    _isInjectingAudio(false),
    _lastFrameIntensity(0.0f)
{
    loadRandomIdentifier(_streamIdentifier, STREAM_IDENTIFIER_NUM_BYTES);
    
    std::fstream sourceFile;
    
    sourceFile.open(filename, std::ios::in | std::ios::binary);
    sourceFile.seekg(0, std::ios::end);
    
    int totalBytes = sourceFile.tellg();
    if (totalBytes == -1) {
        printf("Error reading audio data from file %s\n", filename);
        _audioSampleArray = NULL;
    } else {
        printf("Read %d bytes from audio file\n", totalBytes);
        sourceFile.seekg(0, std::ios::beg);
        _numTotalSamples = totalBytes / 2;
        _audioSampleArray = new int16_t[_numTotalSamples];
        
        sourceFile.read((char *)_audioSampleArray, totalBytes);
    }
}

AudioInjector::AudioInjector(int maxNumSamples) :
    _numTotalSamples(maxNumSamples),
    _position(0.0f, 0.0f, 0.0f),
    _orientation(),
    _radius(0.0f),
    _volume(MAX_INJECTOR_VOLUME),
    _indexOfNextSlot(0),
    _isInjectingAudio(false),
    _lastFrameIntensity(0.0f)
{
    loadRandomIdentifier(_streamIdentifier, STREAM_IDENTIFIER_NUM_BYTES);
    
    _audioSampleArray = new int16_t[maxNumSamples];
    memset(_audioSampleArray, 0, _numTotalSamples * sizeof(int16_t));
}

AudioInjector::~AudioInjector() {
    delete[] _audioSampleArray;
}

void AudioInjector::injectAudio(UDPSocket* injectorSocket, sockaddr* destinationSocket) {
    if (_audioSampleArray) {
        _isInjectingAudio = true;
        
        timeval startTime;
        
        // calculate the number of bytes required for additional data
        int leadingBytes = numBytesForPacketHeader((unsigned char*) &PACKET_TYPE_INJECT_AUDIO)
            + sizeof(_streamIdentifier)
            + sizeof(_position)
            + sizeof(_orientation)
            + sizeof(_radius)
            + sizeof(_volume);
        
        unsigned char dataPacket[(BUFFER_LENGTH_SAMPLES_PER_CHANNEL * sizeof(int16_t)) + leadingBytes];
        
        unsigned char* currentPacketPtr = dataPacket + populateTypeAndVersion(dataPacket, PACKET_TYPE_INJECT_AUDIO);
        
        // copy the identifier for this injector
        memcpy(currentPacketPtr, &_streamIdentifier, sizeof(_streamIdentifier));
        currentPacketPtr += sizeof(_streamIdentifier);
        
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
            int numSamplesToCopy = BUFFER_LENGTH_SAMPLES_PER_CHANNEL;
            
            if (_numTotalSamples - i < BUFFER_LENGTH_SAMPLES_PER_CHANNEL) {
                numSamplesToCopy = _numTotalSamples - i;
                memset(currentPacketPtr + numSamplesToCopy,
                       0,
                       BUFFER_LENGTH_BYTES_PER_CHANNEL - (numSamplesToCopy * sizeof(int16_t)));
            }
            
            memcpy(currentPacketPtr, _audioSampleArray + i, numSamplesToCopy * sizeof(int16_t));
            
            injectorSocket->send(destinationSocket, dataPacket, sizeof(dataPacket));
            
            // calculate the intensity for this frame
            float lastRMS = 0;
            
            for (int j = 0; j < BUFFER_LENGTH_SAMPLES_PER_CHANNEL; j++) {
                lastRMS +=  _audioSampleArray[i + j] * _audioSampleArray[i + j];
            }
            
            lastRMS /= BUFFER_LENGTH_SAMPLES_PER_CHANNEL;
            lastRMS = sqrtf(lastRMS);
            
            _lastFrameIntensity = lastRMS / std::numeric_limits<int16_t>::max();
            
            int usecToSleep = usecTimestamp(&startTime) + (++nextFrame * INJECT_INTERVAL_USECS) - usecTimestampNow();
            if (usecToSleep > 0) {
                usleep(usecToSleep);
            }
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
        printf("Copied %d samples to the buffer\n", numSamples);
        _indexOfNextSlot += numSamples;
    }
}
