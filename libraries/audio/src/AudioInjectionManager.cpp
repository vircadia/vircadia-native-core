//
//  AudioInjectionManager.cpp
//  hifi
//
//  Created by Stephen Birarda on 5/16/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <sys/time.h>

#include "SharedUtil.h"
#include "PacketHeaders.h"

#include "AudioInjectionManager.h"

UDPSocket* AudioInjectionManager::_injectorSocket = NULL;
sockaddr AudioInjectionManager::_destinationSocket;
AudioInjector* AudioInjectionManager::_injectors[50] = {};

AudioInjector* AudioInjectionManager::injectorWithSamplesFromFile(const char* filename) {
    for (int i = 0; i < MAX_CONCURRENT_INJECTORS; i++) {
        if (!_injectors[i]) {
            _injectors[i] = new AudioInjector(filename);
            return _injectors[i];
        }
    }
    
    return NULL;
}

AudioInjector* AudioInjectionManager::injectorWithCapacity(int capacity) {
    for (int i = 0; i < MAX_CONCURRENT_INJECTORS; i++) {
        if (!_injectors[i]) {
            _injectors[i] = new AudioInjector(capacity);
            return _injectors[i];
        }
    }
    
    return NULL;
}

void* AudioInjectionManager::injectAudioViaThread(void* args) {
    AudioInjector* injector = (AudioInjector*) args;
    
    if (injector->_audioSampleArray) {
        injector->setIsInjectingAudio(true);
        
        timeval startTime;
        
        // one byte for header, 3 positional floats, 1 bearing float, 1 attenuation modifier byte
        int leadingBytes = 1 + (sizeof(float) * 4) + 1;
        unsigned char dataPacket[BUFFER_LENGTH_BYTES + leadingBytes];
        
        dataPacket[0] = PACKET_HEADER_INJECT_AUDIO;
        unsigned char *currentPacketPtr = dataPacket + 1;
        
        memcpy(currentPacketPtr, &injector->getPosition(), sizeof(injector->getPosition()));
        currentPacketPtr += sizeof(injector->getPosition());
        
        *currentPacketPtr = injector->getVolume();
        currentPacketPtr++;
        
        memcpy(currentPacketPtr, &injector->_bearing, sizeof(injector->_bearing));
        currentPacketPtr += sizeof(injector->_bearing);
        
        for (int i = 0; i < injector->_numTotalSamples; i += BUFFER_LENGTH_SAMPLES) {
            gettimeofday(&startTime, NULL);
            
            int numSamplesToCopy = BUFFER_LENGTH_SAMPLES;
            
            if (injector->_numTotalSamples - i < BUFFER_LENGTH_SAMPLES) {
                numSamplesToCopy = injector->_numTotalSamples - i;
                memset(currentPacketPtr + numSamplesToCopy, 0, BUFFER_LENGTH_BYTES - (numSamplesToCopy * sizeof(int16_t)));
            }
            
            memcpy(currentPacketPtr, injector->_audioSampleArray + i, numSamplesToCopy * sizeof(int16_t));
            
            _injectorSocket->send(&_destinationSocket, dataPacket, sizeof(dataPacket));
            
            double usecToSleep = BUFFER_SEND_INTERVAL_USECS - (usecTimestampNow() - usecTimestamp(&startTime));
            if (usecToSleep > 0) {
                usleep(usecToSleep);
            }
        }
        
        injector->_isInjectingAudio = false;
    }
    
    // if this an injector inside the injection manager's array we're responsible for deletion
    for (int i = 0; i < MAX_CONCURRENT_INJECTORS; i++) {
        if (_injectors[i] == injector) {
            // pointer matched - delete this injector
            delete injector;
            
            // set the pointer to NULL so we can reuse this spot
            _injectors[i] = NULL;
        }
    }
    
    pthread_exit(0);
}

void AudioInjectionManager::threadInjector(AudioInjector* injector) {
    pthread_t audioInjectThread;
    pthread_create(&audioInjectThread, NULL, injectAudioViaThread, (void*) injector);
}