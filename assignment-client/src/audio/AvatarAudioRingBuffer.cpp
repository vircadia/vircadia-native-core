//
//  AvatarAudioRingBuffer.cpp
//  assignment-client/src/audio
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <PacketHeaders.h>

#include "AvatarAudioRingBuffer.h"

AvatarAudioRingBuffer::AvatarAudioRingBuffer(bool isStereo) :
    PositionalAudioRingBuffer(PositionalAudioRingBuffer::Microphone, isStereo) {
    
}

int AvatarAudioRingBuffer::parseData(const QByteArray& packet) {
    _interframeTimeGapHistory.frameReceived();
    updateDesiredJitterBufferNumSamples();

    _shouldLoopbackForNode = (packetTypeForPacket(packet) == PacketTypeMicrophoneAudioWithEcho);
    return PositionalAudioRingBuffer::parseData(packet);
}
