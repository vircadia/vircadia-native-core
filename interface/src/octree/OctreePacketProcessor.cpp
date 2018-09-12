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

#include "OctreePacketProcessor.h"

#include <PerfStat.h>

#include "Application.h"
#include "Menu.h"
#include "SceneScriptingInterface.h"

OctreePacketProcessor::OctreePacketProcessor():
    _safeLanding(new SafeLanding())
{
    setObjectName("Octree Packet Processor");

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    const PacketReceiver::PacketTypeList octreePackets =
        { PacketType::OctreeStats, PacketType::EntityData, PacketType::EntityErase, PacketType::EntityQueryInitialResultsComplete };
    packetReceiver.registerDirectListenerForTypes(octreePackets, this, "handleOctreePacket");
}

OctreePacketProcessor::~OctreePacketProcessor() { }

void OctreePacketProcessor::handleOctreePacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode) {
    queueReceivedPacket(message, senderNode);
}

void OctreePacketProcessor::processPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "OctreePacketProcessor::processPacket()");

    const int WAY_BEHIND = 300;

    if (packetsToProcessCount() > WAY_BEHIND && qApp->getLogger()->extraDebugging()) {
        qDebug("OctreePacketProcessor::processPacket() packets to process=%d", packetsToProcessCount());
    }
    
    bool wasStatsPacket = false;

    PacketType octreePacketType = message->getType();

    // note: PacketType_OCTREE_STATS can have PacketType_VOXEL_DATA
    // immediately following them inside the same packet. So, we process the PacketType_OCTREE_STATS first
    // then process any remaining bytes as if it was another packet
    if (octreePacketType == PacketType::OctreeStats) {
        int statsMessageLength = qApp->processOctreeStats(*message, sendingNode);

        wasStatsPacket = true;
        int piggybackBytes = message->getSize() - statsMessageLength;
        
        if (piggybackBytes) {
            // construct a new packet from the piggybacked one
            auto buffer = std::unique_ptr<char[]>(new char[piggybackBytes]);
            memcpy(buffer.get(), message->getRawMessage() + statsMessageLength, piggybackBytes);
            
            auto newPacket = NLPacket::fromReceivedPacket(std::move(buffer), piggybackBytes, message->getSenderSockAddr());
            message = QSharedPointer<ReceivedMessage>::create(*newPacket);
        } else {
            // Note... stats packets don't have sequence numbers, so we don't want to send those to trackIncomingVoxelPacket()
            return; // bail since no piggyback data
        }
    } // fall through to piggyback message

    PacketType packetType = message->getType();

    // check version of piggyback packet against expected version
    if (message->getVersion() != versionForPacketType(message->getType())) {
        static QMultiMap<QUuid, PacketType> versionDebugSuppressMap;

        const QUuid& senderUUID = sendingNode->getUUID();
        if (!versionDebugSuppressMap.contains(senderUUID, packetType)) {
            
            qDebug() << "Was stats packet? " << wasStatsPacket;
            qDebug() << "OctreePacketProcessor - piggyback packet version mismatch on" << packetType << "- Sender"
                << senderUUID << "sent" << (int) message->getVersion() << "but"
                << (int) versionForPacketType(packetType) << "expected.";

            emit packetVersionMismatch();

            versionDebugSuppressMap.insert(senderUUID, packetType);
        }
        return; // bail since piggyback version doesn't match
    }

    qApp->trackIncomingOctreePacket(*message, sendingNode, wasStatsPacket);
    
    // seek back to beginning of packet after tracking
    message->seek(0);

    switch(packetType) {
        case PacketType::EntityErase: {
            if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderEntities()) {
                auto renderer = qApp->getEntities();
                if (renderer) {
                    renderer->processEraseMessage(*message, sendingNode);
                }
            }
        } break;

        case PacketType::EntityData: {
            if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderEntities()) {
                auto renderer = qApp->getEntities();
                if (renderer) {
                    renderer->processDatagram(*message, sendingNode);
                    _safeLanding->noteReceivedsequenceNumber(renderer->getLastOctreeMessageSequence());
                }
            }
        } break;

        case PacketType::EntityQueryInitialResultsComplete: {
            // Read sequence #
            OCTREE_PACKET_SEQUENCE completionNumber;
            message->readPrimitive(&completionNumber);
            _safeLanding->setCompletionSequenceNumbers(0, completionNumber);
        } break;

        default: {
            // nothing to do
        } break;
    }
}

void OctreePacketProcessor::startEntitySequence() {
    _safeLanding->startEntitySequence(qApp->getEntities());
}
