//
//  AudioInjectionManager.cpp
//  hifi
//
//  Created by Stephen Birarda on 5/16/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <sys/time.h>

#include "SharedUtil.h"
#include "NodeList.h"
#include "NodeTypes.h"
#include "Node.h"
#include "PacketHeaders.h"

#include "AudioInjectionManager.h"

UDPSocket* AudioInjectionManager::_injectorSocket = NULL;
sockaddr AudioInjectionManager::_destinationSocket;
bool AudioInjectionManager::_isDestinationSocketExplicit = false;
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

void AudioInjectionManager::setDestinationSocket(sockaddr& destinationSocket) {
    _destinationSocket = destinationSocket;
    _isDestinationSocketExplicit = true;
}

void* AudioInjectionManager::injectAudioViaThread(void* args) {
    AudioInjector* injector = (AudioInjector*) args;
    
    // if we don't have an injectorSocket then grab the one from the node list
    if (!_injectorSocket) {
        _injectorSocket = NodeList::getInstance()->getNodeSocket();
    }
    
    // if we don't have an explicit destination socket then pull active socket for current audio mixer from node list
    if (!_isDestinationSocketExplicit) {
        Node* audioMixer = NodeList::getInstance()->soloNodeOfType(NODE_TYPE_AUDIO_MIXER);
        if (audioMixer) {
            _destinationSocket = *audioMixer->getActiveSocket();
        }
    }
    
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