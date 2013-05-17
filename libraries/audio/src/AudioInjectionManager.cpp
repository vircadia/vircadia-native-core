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
    
    injector->injectAudio(_injectorSocket, &_destinationSocket);
    
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