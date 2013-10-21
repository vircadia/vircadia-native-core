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
    PositionalAudioRingBuffer(PositionalAudioRingBuffer::Microphone),
    _shouldLoopbackForNode(false) {
    
}

int AvatarAudioRingBuffer::parseData(unsigned char* sourceBuffer, int numBytes) {
    _shouldLoopbackForNode = (sourceBuffer[0] == PACKET_TYPE_MICROPHONE_AUDIO_WITH_ECHO);
    return PositionalAudioRingBuffer::parseData(sourceBuffer, numBytes);
}