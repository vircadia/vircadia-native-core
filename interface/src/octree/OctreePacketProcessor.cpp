//
//  OctreePacketProcessor.cpp
//  interface/src/octree
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <PerfStat.h>

#include "Application.h"
#include "Menu.h"
#include "OctreePacketProcessor.h"
#include "SceneScriptingInterface.h"

OctreePacketProcessor::OctreePacketProcessor() {
    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    
    packetReceiver.registerDirectListenerForTypes({ PacketType::OctreeStats, PacketType::EntityData, PacketType::EntityErase },
                                                  this, "handleOctreePacket");
}

void OctreePacketProcessor::handleOctreePacket(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    queueReceivedPacket(packet, senderNode);
}

void OctreePacketProcessor::processPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode) {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "OctreePacketProcessor::processPacket()");

    const int WAY_BEHIND = 300;

    if (packetsToProcessCount() > WAY_BEHIND && Application::getInstance()->getLogger()->extraDebugging()) {
        qDebug("OctreePacketProcessor::processPacket() packets to process=%d", packetsToProcessCount());
    }
    
    Application* app = Application::getInstance();
    bool wasStatsPacket = false;

    PacketType octreePacketType = packet->getType();

    // note: PacketType_OCTREE_STATS can have PacketType_VOXEL_DATA
    // immediately following them inside the same packet. So, we process the PacketType_OCTREE_STATS first
    // then process any remaining bytes as if it was another packet
    if (octreePacketType == PacketType::OctreeStats) {
        int statsMessageLength = app->processOctreeStats(*packet, sendingNode);

        wasStatsPacket = true;
        int piggybackBytes = packet->getPayloadSize() - statsMessageLength;
        
        if (piggybackBytes) {
            // construct a new packet from the piggybacked one
            auto buffer = std::unique_ptr<char[]>(new char[piggybackBytes]);
            memcpy(buffer.get(), packet->getPayload() + statsMessageLength, piggybackBytes);
            
            auto newPacket = NLPacket::fromReceivedPacket(std::move(buffer), piggybackBytes, packet->getSenderSockAddr());
            packet = QSharedPointer<NLPacket>(newPacket.release());
        } else {
            // Note... stats packets don't have sequence numbers, so we don't want to send those to trackIncomingVoxelPacket()
            return; // bail since no piggyback data
        }
    } // fall through to piggyback message

    PacketType packetType = packet->getType();

    // check version of piggyback packet against expected version
    if (packet->getVersion() != versionForPacketType(packet->getType())) {
        static QMultiMap<QUuid, PacketType> versionDebugSuppressMap;

        const QUuid& senderUUID = packet->getSourceID();
        if (!versionDebugSuppressMap.contains(senderUUID, packetType)) {
            
            qDebug() << "OctreePacketProcessor - piggyback packet version mismatch on" << packetType << "- Sender"
                << senderUUID << "sent" << (int) packet->getVersion() << "but"
                << (int) versionForPacketType(packetType) << "expected.";

            emit packetVersionMismatch();

            versionDebugSuppressMap.insert(senderUUID, packetType);
        }
        return; // bail since piggyback version doesn't match
    }

    app->trackIncomingOctreePacket(*packet, sendingNode, wasStatsPacket);
    
    // seek back to beginning of packet after tracking
    packet->seek(0);

    switch(packetType) {
        case PacketType::EntityErase: {
            if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderEntities()) {
                app->_entities.processEraseMessage(*packet, sendingNode);
            }
        } break;

        case PacketType::EntityData: {
            if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderEntities()) {
                app->_entities.processDatagram(*packet, sendingNode);
            }
        } break;

        default: {
            // nothing to do
        } break;
    }
}
