//
//  AudioInjector.cpp
//  hifi
//
//  Created by Stephen Birarda on 4/23/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <fstream>
#include <cstring>

#include <SharedUtil.h>
#include <PacketHeaders.h>
#include <UDPSocket.h>

#include "AudioInjector.h"

const int MAX_INJECTOR_VOLUME = 255;

AudioInjector::AudioInjector(const char* filename) :
    _position(),
    _bearing(0),
    _volume(MAX_INJECTOR_VOLUME),
    _indexOfNextSlot(0),
    _isInjectingAudio(false)
{
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
    _position(),
    _bearing(0),
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
    if (_audioSampleArray) {
        _isInjectingAudio = true;
        
        timeval startTime;
        
        // one byte for header, 3 positional floats, 1 bearing float, 1 attenuation modifier byte
        int leadingBytes = 1 + (sizeof(float) * 4) + 1;
        unsigned char dataPacket[BUFFER_LENGTH_BYTES + leadingBytes];
        
        dataPacket[0] = PACKET_HEADER_INJECT_AUDIO;
        unsigned char *currentPacketPtr = dataPacket + 1;
        
        memcpy(currentPacketPtr, &_position, sizeof(_position));
        currentPacketPtr += sizeof(_position);
        
        *currentPacketPtr = _volume;
        currentPacketPtr++;
        
        memcpy(currentPacketPtr, &_bearing, sizeof(_bearing));
        currentPacketPtr += sizeof(_bearing);
        
        for (int i = 0; i < _numTotalSamples; i += BUFFER_LENGTH_SAMPLES) {
            gettimeofday(&startTime, NULL);
            
            int numSamplesToCopy = BUFFER_LENGTH_SAMPLES;
            
            if (_numTotalSamples - i < BUFFER_LENGTH_SAMPLES) {
                numSamplesToCopy = _numTotalSamples - i;
                memset(currentPacketPtr + numSamplesToCopy, 0, BUFFER_LENGTH_BYTES - (numSamplesToCopy * sizeof(int16_t)));
            }
            
            memcpy(currentPacketPtr, _audioSampleArray + i, numSamplesToCopy * sizeof(int16_t));
            
            injectorSocket->send(destinationSocket, dataPacket, sizeof(dataPacket));
            
            double usecToSleep = BUFFER_SEND_INTERVAL_USECS - (usecTimestampNow() - usecTimestamp(&startTime));
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
    if (_audioSampleArray + _indexOfNextSlot + numSamples <= _audioSampleArray + (_numTotalSamples / sizeof(int16_t))) {
        // only copy the audio from the sample buffer if there's space
        memcpy(_audioSampleArray + _indexOfNextSlot, sampleBuffer, numSamples * sizeof(int16_t));
        _indexOfNextSlot += numSamples;
    }
}
