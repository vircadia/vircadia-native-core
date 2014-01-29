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
    PositionalAudioRingBuffer(PositionalAudioRingBuffer::Microphone) {
    
}

int AvatarAudioRingBuffer::parseData(const QByteArray& packet) {
    _shouldLoopbackForNode = (packetTypeForPacket(packet) == PacketTypeMicrophoneAudioWithEcho);
    return PositionalAudioRingBuffer::parseData(packet);
}