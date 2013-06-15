//
//  AvatarAudioRingBuffer.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <PacketHeaders.h>

#include "AvatarAudioRingBuffer.h"

AvatarAudioRingBuffer::AvatarAudioRingBuffer() :
    _twoPoles(),
    _shouldLoopbackForAgent(false) {
    
}

AvatarAudioRingBuffer::~AvatarAudioRingBuffer() {
    // enumerate the freeVerbs map and delete the FreeVerb objects
    for (TwoPoleAgentMap::iterator poleIterator = _twoPoles.begin(); poleIterator != _twoPoles.end(); poleIterator++) {
        delete poleIterator->second;
    }
}

int AvatarAudioRingBuffer::parseData(unsigned char* sourceBuffer, int numBytes) {
    _shouldLoopbackForAgent = (sourceBuffer[0] == PACKET_HEADER_MICROPHONE_AUDIO_WITH_ECHO);
    return PositionalAudioRingBuffer::parseData(sourceBuffer, numBytes);
}