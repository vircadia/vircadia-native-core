//
//  AvatarAudioRingBuffer.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "AvatarAudioRingBuffer.h"

AvatarAudioRingBuffer::AvatarAudioRingBuffer() : _freeVerbs() {
    
}

AvatarAudioRingBuffer::~AvatarAudioRingBuffer() {
    // enumerate the freeVerbs map and delete the FreeVerb objects
    for (FreeVerbAgentMap::iterator verbIterator = _freeVerbs.begin(); verbIterator != _freeVerbs.end(); verbIterator++) {
        delete verbIterator->second;
    }
}