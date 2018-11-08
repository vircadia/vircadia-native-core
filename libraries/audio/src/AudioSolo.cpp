//
//  AudioSolo.cpp
//
//
//  Created by Clement Brisset on 11/5/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioSolo.h"

#include <NodeList.h>

bool AudioSolo::isSoloing() const {
    Lock lock(_mutex);
    return !_nodesSoloed.empty();
}

QVector<QUuid> AudioSolo::getUUIDs() const {
    Lock lock(_mutex);
    return _nodesSoloed.values().toVector();
}

void AudioSolo::addUUIDs(QVector<QUuid> uuidList) {
    // create a reliable NLPacket with space for the solo UUIDs
    auto soloPacket = NLPacket::create(PacketType::AudioSoloRequest,
                                       uuidList.size() * NUM_BYTES_RFC4122_UUID + sizeof(uint8_t), true);
    uint8_t addToSoloList = (uint8_t)true;
    soloPacket->writePrimitive(addToSoloList);

    {
        Lock lock(_mutex);
        for (auto uuid : uuidList) {
            if (_nodesSoloed.contains(uuid)) {
                qWarning() << "Uuid already in solo list:" << uuid;
            } else {
                // write the node ID to the packet
                soloPacket->write(uuid.toRfc4122());
                _nodesSoloed.insert(uuid);
            }
        }
    }

    // send off this solo packet reliably to the matching node
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->broadcastToNodes(std::move(soloPacket), { NodeType::AudioMixer });
}

void AudioSolo::removeUUIDs(QVector<QUuid> uuidList) {
    // create a reliable NLPacket with space for the solo UUIDs
    auto soloPacket = NLPacket::create(PacketType::AudioSoloRequest,
                                       uuidList.size() * NUM_BYTES_RFC4122_UUID + sizeof(uint8_t), true);
    uint8_t addToSoloList = (uint8_t)false;
    soloPacket->writePrimitive(addToSoloList);

    {
        Lock lock(_mutex);
        for (auto uuid : uuidList) {
            if (!_nodesSoloed.contains(uuid)) {
                qWarning() << "Uuid not in solo list:" << uuid;
            } else {
                // write the node ID to the packet
                soloPacket->write(uuid.toRfc4122());
                _nodesSoloed.remove(uuid);
            }
        }
    }

    // send off this solo packet reliably to the matching node
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->broadcastToNodes(std::move(soloPacket), { NodeType::AudioMixer });
}

void AudioSolo::reset() {
    Lock lock(_mutex);
    removeUUIDs(getUUIDs());
}


void AudioSolo::resend() {
    Lock lock(_mutex);
    auto uuids = getUUIDs();
    _nodesSoloed.clear();
    addUUIDs(uuids);
}

