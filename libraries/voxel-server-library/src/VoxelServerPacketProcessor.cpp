//
//  VoxelServerPacketProcessor.cpp
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded network packet processor for the voxel-server
//

#include <PacketHeaders.h>
#include <PerfStat.h>

#include "VoxelServer.h"
#include "VoxelServerConsts.h"
#include "VoxelServerPacketProcessor.h"


VoxelServerPacketProcessor::VoxelServerPacketProcessor(VoxelServer* myServer) :
    _myServer(myServer),
    _receivedPacketCount(0) {
}


void VoxelServerPacketProcessor::processPacket(sockaddr& senderAddress, unsigned char* packetData, ssize_t packetLength) {

    bool debugProcessPacket = _myServer->wantsDebugVoxelReceiving();
    
    if (debugProcessPacket) {
        printf("VoxelServerPacketProcessor::processPacket(() packetData=%p packetLength=%ld\n", packetData, packetLength);
    }

    int numBytesPacketHeader = numBytesForPacketHeader(packetData);
    
    if (packetData[0] == PACKET_TYPE_SET_VOXEL || packetData[0] == PACKET_TYPE_SET_VOXEL_DESTRUCTIVE) {
        bool destructive = (packetData[0] == PACKET_TYPE_SET_VOXEL_DESTRUCTIVE);
        PerformanceWarning warn(_myServer->wantShowAnimationDebug(),
                                destructive ? "PACKET_TYPE_SET_VOXEL_DESTRUCTIVE" : "PACKET_TYPE_SET_VOXEL",
                                _myServer->wantShowAnimationDebug());
        
        _receivedPacketCount++;
        
        unsigned short int itemNumber = (*((unsigned short int*)(packetData + numBytesPacketHeader)));
        if (_myServer->wantShowAnimationDebug()) {
            printf("got %s - command from client receivedBytes=%ld itemNumber=%d\n",
                destructive ? "PACKET_TYPE_SET_VOXEL_DESTRUCTIVE" : "PACKET_TYPE_SET_VOXEL",
                packetLength, itemNumber);
        }
        
        if (_myServer->wantsDebugVoxelReceiving()) {
            printf("got %s - %d command from client receivedBytes=%ld itemNumber=%d\n",
                destructive ? "PACKET_TYPE_SET_VOXEL_DESTRUCTIVE" : "PACKET_TYPE_SET_VOXEL",
                _receivedPacketCount, packetLength, itemNumber);
        }
        int atByte = numBytesPacketHeader + sizeof(itemNumber);
        unsigned char* voxelData = (unsigned char*)&packetData[atByte];
        while (atByte < packetLength) {
            int maxSize = packetLength - atByte;

            if (debugProcessPacket) {
                printf("VoxelServerPacketProcessor::processPacket(() %s packetData=%p packetLength=%ld voxelData=%p atByte=%d maxSize=%d\n",
                    destructive ? "PACKET_TYPE_SET_VOXEL_DESTRUCTIVE" : "PACKET_TYPE_SET_VOXEL",
                    packetData, packetLength, voxelData, atByte, maxSize);
            }

            int octets = numberOfThreeBitSectionsInCode(voxelData, maxSize);
            
            if (octets == OVERFLOWED_OCTCODE_BUFFER) {
                printf("WARNING! Got voxel edit record that would overflow buffer in numberOfThreeBitSectionsInCode(), ");
                printf("bailing processing of packet!\n");
                break;
            }
            
            const int COLOR_SIZE_IN_BYTES = 3;
            int voxelDataSize = bytesRequiredForCodeLength(octets) + COLOR_SIZE_IN_BYTES;
            int voxelCodeSize = bytesRequiredForCodeLength(octets);

            if (atByte + voxelDataSize <= packetLength) {
                if (_myServer->wantShowAnimationDebug()) {
                    int red   = voxelData[voxelCodeSize + RED_INDEX];
                    int green = voxelData[voxelCodeSize + GREEN_INDEX];
                    int blue  = voxelData[voxelCodeSize + BLUE_INDEX];

                    float* vertices = firstVertexForCode(voxelData);
                    printf("inserting voxel: %f,%f,%f r=%d,g=%d,b=%d\n", vertices[0], vertices[1], vertices[2], red, green, blue);
                    delete[] vertices;
                }
        
                _myServer->getServerTree().lockForWrite();
                _myServer->getServerTree().readCodeColorBufferToTree(voxelData, destructive);
                _myServer->getServerTree().unlock();

                // skip to next voxel edit record in the packet
                voxelData += voxelDataSize;
                atByte += voxelDataSize;
            } else {
                printf("WARNING! Got voxel edit record that would overflow buffer, bailing processing of packet!\n");
                break;
            }
        }

        if (debugProcessPacket) {
            printf("VoxelServerPacketProcessor::processPacket(() DONE LOOPING FOR %s packetData=%p packetLength=%ld voxelData=%p atByte=%d\n",
                destructive ? "PACKET_TYPE_SET_VOXEL_DESTRUCTIVE" : "PACKET_TYPE_SET_VOXEL",
                packetData, packetLength, voxelData, atByte);
        }

        // Make sure our Node and NodeList knows we've heard from this node.
        Node* node = NodeList::getInstance()->nodeWithAddress(&senderAddress);
        if (node) {
            node->setLastHeardMicrostamp(usecTimestampNow());
        }

    } else if (packetData[0] == PACKET_TYPE_ERASE_VOXEL) {

        // Send these bits off to the VoxelTree class to process them
        _myServer->getServerTree().lockForWrite();
        _myServer->getServerTree().processRemoveVoxelBitstream((unsigned char*)packetData, packetLength);
        _myServer->getServerTree().unlock();

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

