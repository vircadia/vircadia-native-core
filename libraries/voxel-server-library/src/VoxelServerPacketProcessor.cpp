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

static QUuid DEFAULT_NODE_ID_REF;

VoxelServerPacketProcessor::VoxelServerPacketProcessor(VoxelServer* myServer) :
    _myServer(myServer),
    _receivedPacketCount(0),
    _totalTransitTime(0),
    _totalProcessTime(0),
    _totalLockWaitTime(0),
    _totalVoxelsInPacket(0),
    _totalPackets(0)
{
}

void VoxelServerPacketProcessor::resetStats() {
    _totalTransitTime = 0;
    _totalProcessTime = 0;
    _totalLockWaitTime = 0;
    _totalVoxelsInPacket = 0;
    _totalPackets = 0;
    
    _singleSenderStats.clear();
}


void VoxelServerPacketProcessor::processPacket(const HifiSockAddr& senderSockAddr,
                                               unsigned char* packetData, ssize_t packetLength) {

    bool debugProcessPacket = _myServer->wantsVerboseDebug();
    
    if (debugProcessPacket) {
        printf("VoxelServerPacketProcessor::processPacket() packetData=%p packetLength=%ld\n", packetData, packetLength);
    }

    int numBytesPacketHeader = numBytesForPacketHeader(packetData);
    
    if (packetData[0] == PACKET_TYPE_SET_VOXEL || packetData[0] == PACKET_TYPE_SET_VOXEL_DESTRUCTIVE) {
        bool destructive = (packetData[0] == PACKET_TYPE_SET_VOXEL_DESTRUCTIVE);
        PerformanceWarning warn(_myServer->wantShowAnimationDebug(),
                                destructive ? "PACKET_TYPE_SET_VOXEL_DESTRUCTIVE" : "PACKET_TYPE_SET_VOXEL",
                                _myServer->wantShowAnimationDebug());

        _receivedPacketCount++;
        
        unsigned short int sequence = (*((unsigned short int*)(packetData + numBytesPacketHeader)));
        uint64_t sentAt = (*((uint64_t*)(packetData + numBytesPacketHeader + sizeof(sequence))));
        uint64_t arrivedAt = usecTimestampNow();
        uint64_t transitTime = arrivedAt - sentAt;
        int voxelsInPacket = 0;
        uint64_t processTime = 0;
        uint64_t lockWaitTime = 0;
        
        if (_myServer->wantShowAnimationDebug() || _myServer->wantsDebugVoxelReceiving()) {
            printf("PROCESSING THREAD: got %s - %d command from client receivedBytes=%ld sequence=%d transitTime=%llu usecs\n",
                destructive ? "PACKET_TYPE_SET_VOXEL_DESTRUCTIVE" : "PACKET_TYPE_SET_VOXEL",
                _receivedPacketCount, packetLength, sequence, transitTime);
        }
        int atByte = numBytesPacketHeader + sizeof(sequence) + sizeof(sentAt);
        unsigned char* voxelData = (unsigned char*)&packetData[atByte];
        while (atByte < packetLength) {
            int maxSize = packetLength - atByte;

            if (debugProcessPacket) {
                printf("VoxelServerPacketProcessor::processPacket() %s packetData=%p packetLength=%ld voxelData=%p atByte=%d maxSize=%d\n",
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

                uint64_t startLock = usecTimestampNow();
                _myServer->getServerTree().lockForWrite();
                uint64_t startProcess = usecTimestampNow();
                _myServer->getServerTree().readCodeColorBufferToTree(voxelData, destructive);
                _myServer->getServerTree().unlock();
                uint64_t endProcess = usecTimestampNow();
                
                voxelsInPacket++;

                uint64_t thisProcessTime = endProcess - startProcess;
                uint64_t thisLockWaitTime = startProcess - startLock;

                processTime += thisProcessTime;
                lockWaitTime += thisLockWaitTime;

                // skip to next voxel edit record in the packet
                voxelData += voxelDataSize;
                atByte += voxelDataSize;
            } else {
                printf("WARNING! Got voxel edit record that would overflow buffer, bailing processing of packet!\n");
                break;
            }
        }

        if (debugProcessPacket) {
            printf("VoxelServerPacketProcessor::processPacket() DONE LOOPING FOR %s packetData=%p packetLength=%ld voxelData=%p atByte=%d\n",
                destructive ? "PACKET_TYPE_SET_VOXEL_DESTRUCTIVE" : "PACKET_TYPE_SET_VOXEL",
                packetData, packetLength, voxelData, atByte);
        }

        // Make sure our Node and NodeList knows we've heard from this node.
        Node* senderNode = NodeList::getInstance()->nodeWithAddress(senderSockAddr);
        QUuid& nodeUUID = DEFAULT_NODE_ID_REF;
        if (senderNode) {
            senderNode->setLastHeardMicrostamp(usecTimestampNow());
            nodeUUID = senderNode->getUUID();
            if (debugProcessPacket) {
                qDebug() << "sender has uuid=" << nodeUUID << "\n";
            }
        } else {
            if (debugProcessPacket) {
                qDebug() << "sender has no known nodeUUID.\n";
            }
        }
        trackInboundPackets(nodeUUID, sequence, transitTime, voxelsInPacket, processTime, lockWaitTime);

    } else if (packetData[0] == PACKET_TYPE_ERASE_VOXEL) {

        _receivedPacketCount++;
        
        unsigned short int sequence = (*((unsigned short int*)(packetData + numBytesPacketHeader)));
        uint64_t sentAt = (*((uint64_t*)(packetData + numBytesPacketHeader + sizeof(sequence))));
        uint64_t arrivedAt = usecTimestampNow();
        uint64_t transitTime = arrivedAt - sentAt;

        if (_myServer->wantShowAnimationDebug() || _myServer->wantsDebugVoxelReceiving()) {
            printf("PROCESSING THREAD: got PACKET_TYPE_ERASE_VOXEL - %d command from client receivedBytes=%ld sequence=%d transitTime=%llu usecs\n",
                _receivedPacketCount, packetLength, sequence, transitTime);
        }

        // Send these bits off to the VoxelTree class to process them
        _myServer->getServerTree().lockForWrite();
        _myServer->getServerTree().processRemoveVoxelBitstream((unsigned char*)packetData, packetLength);
        _myServer->getServerTree().unlock();

        // Make sure our Node and NodeList knows we've heard from this node.
        Node* node = NodeList::getInstance()->nodeWithAddress(senderSockAddr);
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
        Node* node = NodeList::getInstance()->nodeWithAddress(senderSockAddr);
        if (node) {
            node->setLastHeardMicrostamp(usecTimestampNow());
        }
    } else {
        printf("unknown packet ignored... packetData[0]=%c\n", packetData[0]);
    }
}

void VoxelServerPacketProcessor::trackInboundPackets(const QUuid& nodeUUID, int sequence, uint64_t transitTime, 
            int voxelsInPacket, uint64_t processTime, uint64_t lockWaitTime) {
            
    _totalTransitTime += transitTime;
    _totalProcessTime += processTime;
    _totalLockWaitTime += lockWaitTime;
    _totalVoxelsInPacket += voxelsInPacket;
    _totalPackets++;
    
    // find the individual senders stats and track them there too...
    // see if this is the first we've heard of this node...
    if (_singleSenderStats.find(nodeUUID) == _singleSenderStats.end()) {
        SingleSenderStats stats;

        stats._totalTransitTime += transitTime;
        stats._totalProcessTime += processTime;
        stats._totalLockWaitTime += lockWaitTime;
        stats._totalVoxelsInPacket += voxelsInPacket;
        stats._totalPackets++;

        _singleSenderStats[nodeUUID] = stats;
    } else {
        SingleSenderStats& stats = _singleSenderStats[nodeUUID];
        stats._totalTransitTime += transitTime;
        stats._totalProcessTime += processTime;
        stats._totalLockWaitTime += lockWaitTime;
        stats._totalVoxelsInPacket += voxelsInPacket;
        stats._totalPackets++;
    }
}


SingleSenderStats::SingleSenderStats() {
    _totalTransitTime = 0; 
    _totalProcessTime = 0;
    _totalLockWaitTime = 0;
    _totalVoxelsInPacket = 0;
    _totalPackets = 0;
}


