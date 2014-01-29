//
//  VoxelPacketProcessor.cpp
//  interface
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded voxel packet receiver for the Application
//

#include <PerfStat.h>

#include "Application.h"
#include "Menu.h"
#include "VoxelPacketProcessor.h"

void VoxelPacketProcessor::processPacket(const HifiSockAddr& senderSockAddr, const QByteArray& packet) {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "VoxelPacketProcessor::processPacket()");
    
    QByteArray mutablePacket = packet;

    const int WAY_BEHIND = 300;

    if (packetsToProcessCount() > WAY_BEHIND && Application::getInstance()->getLogger()->extraDebugging()) {
        qDebug("VoxelPacketProcessor::processPacket() packets to process=%d", packetsToProcessCount());
    }
    ssize_t messageLength = mutablePacket.size();

    Application* app = Application::getInstance();
    bool wasStatsPacket = false;


    // check to see if the UI thread asked us to kill the voxel tree. since we're the only thread allowed to do that
    if (app->_wantToKillLocalVoxels) {
        app->_voxels.killLocalVoxels();
        app->_wantToKillLocalVoxels = false;
    }
    
    PacketType voxelPacketType = packetTypeForPacket(mutablePacket);

    // note: PacketType_OCTREE_STATS can have PacketType_VOXEL_DATA
    // immediately following them inside the same packet. So, we process the PacketType_OCTREE_STATS first
    // then process any remaining bytes as if it was another packet
    if (voxelPacketType == PacketTypeOctreeStats) {

        int statsMessageLength = app->parseOctreeStats(mutablePacket, senderSockAddr);
        wasStatsPacket = true;
        if (messageLength > statsMessageLength) {
            mutablePacket = mutablePacket.mid(statsMessageLength);
            if (!packetVersionMatch(packet)) {
                return; // bail since piggyback data doesn't match our versioning
            }
        } else {
            // Note... stats packets don't have sequence numbers, so we don't want to send those to trackIncomingVoxelPacket()
            return; // bail since no piggyback data
        }
    } // fall through to piggyback message
    
    voxelPacketType = packetTypeForPacket(mutablePacket);
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::Voxels)) {
        app->trackIncomingVoxelPacket(mutablePacket, senderSockAddr, wasStatsPacket);

        SharedNodePointer serverNode = NodeList::getInstance()->nodeWithAddress(senderSockAddr);
        if (serverNode && serverNode->getActiveSocket() && *serverNode->getActiveSocket() == senderSockAddr) {

            switch(voxelPacketType) {
                case PacketTypeParticleErase: {
                    app->_particles.processEraseMessage(mutablePacket, senderSockAddr, serverNode.data());
                } break;

                case PacketTypeParticleData: {
                    app->_particles.processDatagram(mutablePacket, senderSockAddr, serverNode.data());
                } break;

                case PacketTypeEnvironmentData: {
                    app->_environment.parseData(senderSockAddr, mutablePacket);
                } break;

                default : {
                    app->_voxels.setDataSourceUUID(serverNode->getUUID());
                    app->_voxels.parseData(mutablePacket);
                    app->_voxels.setDataSourceUUID(QUuid());
                } break;
            }
        }
    }
}

