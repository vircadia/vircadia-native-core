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

void VoxelPacketProcessor::processPacket(const HifiSockAddr& senderSockAddr, unsigned char* packetData, ssize_t packetLength) {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "VoxelPacketProcessor::processPacket()");
                            
    const int WAY_BEHIND = 300;
    if (packetsToProcessCount() > WAY_BEHIND && Menu::getInstance()->isOptionChecked(MenuOption::ExtraDebugging)) {
        qDebug("VoxelPacketProcessor::processPacket() packets to process=%d\n", packetsToProcessCount());
    }
    ssize_t messageLength = packetLength;

    Application* app = Application::getInstance();
    bool wasStatsPacket = false;
    
    
    // check to see if the UI thread asked us to kill the voxel tree. since we're the only thread allowed to do that
    if (app->_wantToKillLocalVoxels) {
        app->_voxels.killLocalVoxels();
        app->_wantToKillLocalVoxels = false;
    }
    
    // note: PACKET_TYPE_OCTREE_STATS can have PACKET_TYPE_VOXEL_DATA
    // immediately following them inside the same packet. So, we process the PACKET_TYPE_OCTREE_STATS first
    // then process any remaining bytes as if it was another packet
    if (packetData[0] == PACKET_TYPE_OCTREE_STATS) {
    
        int statsMessageLength = app->parseOctreeStats(packetData, messageLength, senderSockAddr);
        wasStatsPacket = true;
        if (messageLength > statsMessageLength) {
            packetData += statsMessageLength;
            messageLength -= statsMessageLength;
            if (!packetVersionMatch(packetData)) {
                return; // bail since piggyback data doesn't match our versioning
            }
        } else {
            // Note... stats packets don't have sequence numbers, so we don't want to send those to trackIncomingVoxelPacket()
            return; // bail since no piggyback data
        }
    } // fall through to piggyback message

    if (Menu::getInstance()->isOptionChecked(MenuOption::Voxels)) {
        app->trackIncomingVoxelPacket(packetData, messageLength, senderSockAddr, wasStatsPacket);
        
        Node* voxelServer = NodeList::getInstance()->nodeWithAddress(senderSockAddr);
        if (voxelServer && *voxelServer->getActiveSocket() == senderSockAddr) {
        
            switch(packetData[0]) {
                case PACKET_TYPE_PARTICLE_DATA: {
                    app->_particles.processDatagram(QByteArray((char*) packetData, messageLength), senderSockAddr);
                } break;
                
                case PACKET_TYPE_ENVIRONMENT_DATA: {
                    app->_environment.parseData(senderSockAddr, packetData, messageLength);
                } break;
                
                default : {
                    app->_voxels.setDataSourceUUID(voxelServer->getUUID());
                    app->_voxels.parseData(packetData, messageLength);
                    app->_voxels.setDataSourceUUID(QUuid());
                } break;
            }
        }
    }
}

