//
//  Created by Bradley Austin Davis on 2015/11/18
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AbstractAudioInterface.h"

#include <QtCore/QSharedPointer>

#include <Node.h>
#include <NodeType.h>
#include <DependencyManager.h>
#include <NodeList.h>
#include <NLPacket.h>
#include <Transform.h>

#include "AudioConstants.h"

void AbstractAudioInterface::emitAudioPacket(const void* audioData, size_t bytes, quint16& sequenceNumber, 
                                const Transform& transform, PacketType packetType, QString codecName) {
    static std::mutex _mutex;
    using Locker = std::unique_lock<std::mutex>;
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);
    if (audioMixer && audioMixer->getActiveSocket()) {
        Locker lock(_mutex);
        auto audioPacket = NLPacket::create(packetType);

        // FIXME - this is not a good way to determine stereoness with codecs.... 
        quint8 isStereo = bytes == AudioConstants::NETWORK_FRAME_BYTES_STEREO ? 1 : 0;

        // write sequence number
        auto sequence = sequenceNumber++;
        audioPacket->writePrimitive(sequence);

        // write the codec - don't include this for injected audio
        if (packetType != PacketType::InjectAudio) {
            auto stringSize = audioPacket->writeString(codecName);
        }

        if (packetType == PacketType::SilentAudioFrame) {
            // pack num silent samples
            quint16 numSilentSamples = isStereo ?
                AudioConstants::NETWORK_FRAME_SAMPLES_STEREO :
                AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL;
            audioPacket->writePrimitive(numSilentSamples);
        } else {
            // set the mono/stereo byte
            audioPacket->writePrimitive(isStereo);
        }

        // pack the three float positions
        audioPacket->writePrimitive(transform.getTranslation());
        // pack the orientation
        audioPacket->writePrimitive(transform.getRotation());

        if (audioPacket->getType() != PacketType::SilentAudioFrame) {
            // audio samples have already been packed (written to networkAudioSamples)
            int leadingBytes = audioPacket->getPayloadSize();
            audioPacket->setPayloadSize(leadingBytes + bytes);
            memcpy(audioPacket->getPayload() + leadingBytes, audioData, bytes);
        }
        nodeList->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SendAudioPacket);
        nodeList->sendUnreliablePacket(*audioPacket, *audioMixer);
    }
}
