//Merge branch 'master' of ssh://github.com/highfidelity/hifi into isentropic/
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

void OctreePacketProcessor::processPacket(const SharedNodePointer& sendingNode, const QByteArray& packet) {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "OctreePacketProcessor::processPacket()");
    
    QByteArray mutablePacket = packet;

    const int WAY_BEHIND = 300;

    if (packetsToProcessCount() > WAY_BEHIND && Application::getInstance()->getLogger()->extraDebugging()) {
        qDebug("OctreePacketProcessor::processPacket() packets to process=%d", packetsToProcessCount());
    }
    int messageLength = mutablePacket.size();

    Application* app = Application::getInstance();
    bool wasStatsPacket = false;


    PacketType::Value voxelPacketType = packetTypeForPacket(mutablePacket);
    
    // note: PacketType_OCTREE_STATS can have PacketType_VOXEL_DATA
    // immediately following them inside the same packet. So, we process the PacketType_OCTREE_STATS first
    // then process any remaining bytes as if it was another packet
    if (voxelPacketType == PacketTypeOctreeStats) {
        int statsMessageLength = app->parseOctreeStats(mutablePacket, sendingNode);
        wasStatsPacket = true;
        if (messageLength > statsMessageLength) {
            mutablePacket = mutablePacket.mid(statsMessageLength);
            
            // TODO: this does not look correct, the goal is to test the packet version for the piggyback, but
            //       this is testing the version and hash of the original packet
            if (!DependencyManager::get<NodeList>()->packetVersionAndHashMatch(packet)) {
                return; // bail since piggyback data doesn't match our versioning
            }
        } else {
            // Note... stats packets don't have sequence numbers, so we don't want to send those to trackIncomingVoxelPacket()
            return; // bail since no piggyback data
        }
    } // fall through to piggyback message
    
    voxelPacketType = packetTypeForPacket(mutablePacket);
    PacketVersion packetVersion = mutablePacket[1];
    PacketVersion expectedVersion = versionForPacketType(voxelPacketType);
    
    // check version of piggyback packet against expected version
    if (packetVersion != expectedVersion) {
        static QMultiMap<QUuid, PacketType> versionDebugSuppressMap;
        
        QUuid senderUUID = uuidFromPacketHeader(packet);
        if (!versionDebugSuppressMap.contains(senderUUID, voxelPacketType)) {
            qDebug() << "Packet version mismatch on" << voxelPacketType << "- Sender"
            << senderUUID << "sent" << (int)packetVersion << "but"
            << (int)expectedVersion << "expected.";
            
            emit packetVersionMismatch();

            versionDebugSuppressMap.insert(senderUUID, voxelPacketType);
        }
        return; // bail since piggyback version doesn't match
    }
    
    app->trackIncomingOctreePacket(mutablePacket, sendingNode, wasStatsPacket);

    if (sendingNode) {

        switch(voxelPacketType) {
            case PacketTypeEntityErase: {
                if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderEntities()) {
                    app->_entities.processEraseMessage(mutablePacket, sendingNode);
                }
            } break;

            case PacketTypeEntityData: {
                if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderEntities()) {
                    app->_entities.processDatagram(mutablePacket, sendingNode);
                }
            } break;

            case PacketTypeEnvironmentData: {
                app->_environment.parseData(*sendingNode->getActiveSocket(), mutablePacket);
            } break;

            default: {
                // nothing to do
            } break;
        }
    }
}

