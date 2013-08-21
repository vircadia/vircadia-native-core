//
//  VoxelServerPacketProcessor.cpp
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded network packet processor for the voxel-server
//

#include <PacketHeaders.h>
#include <PerfStat.h>

#include "VoxelServer.h"
#include "VoxelServerPacketProcessor.h"


void VoxelServerPacketProcessor::processPacket(sockaddr& senderAddress, unsigned char* packetData, ssize_t packetLength) {

    int numBytesPacketHeader = numBytesForPacketHeader(packetData);
    
    if (packetData[0] == PACKET_TYPE_SET_VOXEL || packetData[0] == PACKET_TYPE_SET_VOXEL_DESTRUCTIVE) {
        bool destructive = (packetData[0] == PACKET_TYPE_SET_VOXEL_DESTRUCTIVE);
        PerformanceWarning warn(::shouldShowAnimationDebug,
                                destructive ? "PACKET_TYPE_SET_VOXEL_DESTRUCTIVE" : "PACKET_TYPE_SET_VOXEL",
                                ::shouldShowAnimationDebug);
        
        ::receivedPacketCount++;
        
        unsigned short int itemNumber = (*((unsigned short int*)(packetData + numBytesPacketHeader)));
        if (::shouldShowAnimationDebug) {
            printf("got %s - command from client receivedBytes=%ld itemNumber=%d\n",
                destructive ? "PACKET_TYPE_SET_VOXEL_DESTRUCTIVE" : "PACKET_TYPE_SET_VOXEL",
                packetLength, itemNumber);
        }
        
        if (::debugVoxelReceiving) {
            printf("got %s - %d command from client receivedBytes=%ld itemNumber=%d\n",
                destructive ? "PACKET_TYPE_SET_VOXEL_DESTRUCTIVE" : "PACKET_TYPE_SET_VOXEL",
                ::receivedPacketCount, packetLength, itemNumber);
        }
        int atByte = numBytesPacketHeader + sizeof(itemNumber);
        unsigned char* voxelData = (unsigned char*)&packetData[atByte];
        while (atByte < packetLength) {
            unsigned char octets = (unsigned char)*voxelData;
            const int COLOR_SIZE_IN_BYTES = 3;
            int voxelDataSize = bytesRequiredForCodeLength(octets) + COLOR_SIZE_IN_BYTES;
            int voxelCodeSize = bytesRequiredForCodeLength(octets);

            // color randomization on insert
            int colorRandomizer = ::wantColorRandomizer ? randIntInRange (-50, 50) : 0;
            int red   = voxelData[voxelCodeSize + 0];
            int green = voxelData[voxelCodeSize + 1];
            int blue  = voxelData[voxelCodeSize + 2];

            if (::shouldShowAnimationDebug) {
                printf("insert voxels - wantColorRandomizer=%s old r=%d,g=%d,b=%d \n",
                    (::wantColorRandomizer?"yes":"no"),red,green,blue);
            }
        
            red   = std::max(0, std::min(255, red   + colorRandomizer));
            green = std::max(0, std::min(255, green + colorRandomizer));
            blue  = std::max(0, std::min(255, blue  + colorRandomizer));

            if (::shouldShowAnimationDebug) {
                printf("insert voxels - wantColorRandomizer=%s NEW r=%d,g=%d,b=%d \n",
                    (::wantColorRandomizer?"yes":"no"),red,green,blue);
            }
            voxelData[voxelCodeSize + 0] = red;
            voxelData[voxelCodeSize + 1] = green;
            voxelData[voxelCodeSize + 2] = blue;

            if (::shouldShowAnimationDebug) {
                float* vertices = firstVertexForCode(voxelData);
                printf("inserting voxel at: %f,%f,%f\n", vertices[0], vertices[1], vertices[2]);
                delete []vertices;
            }
        
            serverTree.readCodeColorBufferToTree(voxelData, destructive);
            // skip to next
            voxelData += voxelDataSize;
            atByte += voxelDataSize;
        }

        // Make sure our Node and NodeList knows we've heard from this node.
        Node* node = NodeList::getInstance()->nodeWithAddress(&senderAddress);
        if (node) {
            node->setLastHeardMicrostamp(usecTimestampNow());
        }

    } else if (packetData[0] == PACKET_TYPE_ERASE_VOXEL) {

        // Send these bits off to the VoxelTree class to process them
        pthread_mutex_lock(&::treeLock);
        ::serverTree.processRemoveVoxelBitstream((unsigned char*)packetData, packetLength);
        pthread_mutex_unlock(&::treeLock);

        // Make sure our Node and NodeList knows we've heard from this node.
        Node* node = NodeList::getInstance()->nodeWithAddress(&senderAddress);
        if (node) {
            node->setLastHeardMicrostamp(usecTimestampNow());
        }
    } else if (packetData[0] == PACKET_TYPE_Z_COMMAND) {

        // the Z command is a special command that allows the sender to send the voxel server high level semantic
        // requests, like erase all, or add sphere scene
        
        char* command = (char*) &packetData[numBytesPacketHeader]; // start of the command
        int commandLength = strlen(command); // commands are null terminated strings
        int totalLength = numBytesPacketHeader + commandLength + 1; // 1 for null termination
        printf("got Z message len(%ld)= %s\n", packetLength, command);
        bool rebroadcast = true; // by default rebroadcast

        while (totalLength <= packetLength) {
            if (strcmp(command, TEST_COMMAND) == 0) {
                printf("got Z message == a message, nothing to do, just report\n");
            }
            totalLength += commandLength + 1; // 1 for null termination
        }

        if (rebroadcast) {
            // Now send this to the connected nodes so they can also process these messages
            printf("rebroadcasting Z message to connected nodes... nodeList.broadcastToNodes()\n");
            NodeList::getInstance()->broadcastToNodes(packetData, packetLength, &NODE_TYPE_AGENT, 1);
        }

        // Make sure our Node and NodeList knows we've heard from this node.
        Node* node = NodeList::getInstance()->nodeWithAddress(&senderAddress);
        if (node) {
            node->setLastHeardMicrostamp(usecTimestampNow());
        }
    } else {
        printf("unknown packet ignored... packetData[0]=%c\n", packetData[0]);
    }
}

