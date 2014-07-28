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

AvatarAudioRingBuffer::AvatarAudioRingBuffer(bool isStereo, bool dynamicJitterBuffer) :
    PositionalAudioRingBuffer(PositionalAudioRingBuffer::Microphone, isStereo, dynamicJitterBuffer) {
    
}

int AvatarAudioRingBuffer::parseDataAndHandleDroppedPackets(const QByteArray& packet, int packetsSkipped) {
    frameReceivedUpdateTimingStats();

    _shouldLoopbackForNode = (packetTypeForPacket(packet) == PacketTypeMicrophoneAudioWithEcho);

    // skip the packet header (includes the source UUID)
    int readBytes = numBytesForPacketHeader(packet);

    // skip the sequence number
    readBytes += sizeof(quint16);

    // hop over the channel flag that has already been read in AudioMixerClientData
    readBytes += sizeof(quint8);
    // read the positional data
    readBytes += parsePositionalData(packet.mid(readBytes));

    if (packetTypeForPacket(packet) == PacketTypeSilentAudioFrame) {
        // this source had no audio to send us, but this counts as a packet
        // write silence equivalent to the number of silent samples they just sent us
        int16_t numSilentSamples;

        memcpy(&numSilentSamples, packet.data() + readBytes, sizeof(int16_t));
        readBytes += sizeof(int16_t);

        // add silent samples for the dropped packets as well. 
        // ASSUME that each dropped packet had same number of silent samples as this one
        numSilentSamples *= (packetsSkipped + 1);

        // NOTE: fixes a bug in old clients that would send garbage for their number of silentSamples
        // CAN'T DO THIS because ScriptEngine.cpp sends frames of different size due to having a different sending interval
        // (every 16.667ms) than Audio.cpp (every 10.667ms)
        //numSilentSamples = getSamplesPerFrame();

        addDroppableSilentSamples(numSilentSamples);

    } else {
        int numAudioBytes = packet.size() - readBytes;
        int numAudioSamples = numAudioBytes / sizeof(int16_t);

        // add silent samples for the dropped packets.
        // ASSUME that each dropped packet had same number of samples as this one
        if (packetsSkipped > 0) {
            addDroppableSilentSamples(packetsSkipped * numAudioSamples);
        }

        // there is audio data to read
        readBytes += writeData(packet.data() + readBytes, numAudioBytes);
    }
    return readBytes;
}
