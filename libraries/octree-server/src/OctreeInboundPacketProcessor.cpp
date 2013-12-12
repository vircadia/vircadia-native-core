//
//  OctreeInboundPacketProcessor.cpp
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded network packet processor for the voxel-server
//

#include <PacketHeaders.h>
#include <PerfStat.h>

#include "OctreeServer.h"
#include "OctreeServerConsts.h"
#include "OctreeInboundPacketProcessor.h"

static QUuid DEFAULT_NODE_ID_REF;

OctreeInboundPacketProcessor::OctreeInboundPacketProcessor(OctreeServer* myServer) :
    _myServer(myServer),
    _receivedPacketCount(0),
    _totalTransitTime(0),
    _totalProcessTime(0),
    _totalLockWaitTime(0),
    _totalElementsInPacket(0),
    _totalPackets(0)
{
}

void OctreeInboundPacketProcessor::resetStats() {
    _totalTransitTime = 0;
    _totalProcessTime = 0;
    _totalLockWaitTime = 0;
    _totalElementsInPacket = 0;
    _totalPackets = 0;
    
    _singleSenderStats.clear();
}


void OctreeInboundPacketProcessor::processPacket(const HifiSockAddr& senderSockAddr,
                                               unsigned char* packetData, ssize_t packetLength) {

    bool debugProcessPacket = _myServer->wantsVerboseDebug();
    
    if (debugProcessPacket) {
        printf("OctreeInboundPacketProcessor::processPacket() packetData=%p packetLength=%ld\n", packetData, packetLength);
    }

    int numBytesPacketHeader = numBytesForPacketHeader(packetData);
    
    
    // Ask our tree subclass if it can handle the incoming packet...
    PACKET_TYPE packetType = packetData[0];
    if (_myServer->getOctree()->handlesEditPacketType(packetType)) {
        PerformanceWarning warn(debugProcessPacket, "processPacket KNOWN TYPE",debugProcessPacket);
        _receivedPacketCount++;

        Node* senderNode = NodeList::getInstance()->nodeWithAddress(senderSockAddr);
        
        unsigned short int sequence = (*((unsigned short int*)(packetData + numBytesPacketHeader)));
        uint64_t sentAt = (*((uint64_t*)(packetData + numBytesPacketHeader + sizeof(sequence))));
        uint64_t arrivedAt = usecTimestampNow();
        uint64_t transitTime = arrivedAt - sentAt;
        int editsInPacket = 0;
        uint64_t processTime = 0;
        uint64_t lockWaitTime = 0;
        
        if (_myServer->wantsDebugReceiving()) {
            printf("PROCESSING THREAD: got '%c' packet - %d command from client "
                   "receivedBytes=%ld sequence=%d transitTime=%llu usecs\n",
                    packetType, _receivedPacketCount, packetLength, sequence, transitTime);
        }
        int atByte = numBytesPacketHeader + sizeof(sequence) + sizeof(sentAt);
        unsigned char* editData = (unsigned char*)&packetData[atByte];
        while (atByte < packetLength) {
            int maxSize = packetLength - atByte;

            if (debugProcessPacket) {
                printf("OctreeInboundPacketProcessor::processPacket() %c "
                       "packetData=%p packetLength=%ld voxelData=%p atByte=%d maxSize=%d\n",
                        packetType, packetData, packetLength, editData, atByte, maxSize);
            }

            uint64_t startLock = usecTimestampNow();
            _myServer->getOctree()->lockForWrite();
            uint64_t startProcess = usecTimestampNow();
            int editDataBytesRead = _myServer->getOctree()->processEditPacketData(packetType, 
                                                                packetData, packetLength, editData, maxSize, senderNode);
            _myServer->getOctree()->unlock();
            uint64_t endProcess = usecTimestampNow();

            editsInPacket++;
            uint64_t thisProcessTime = endProcess - startProcess;
            uint64_t thisLockWaitTime = startProcess - startLock;
            processTime += thisProcessTime;
            lockWaitTime += thisLockWaitTime;

            // skip to next voxel edit record in the packet
            editData += editDataBytesRead;
            atByte += editDataBytesRead;
        }

        if (debugProcessPacket) {
            printf("OctreeInboundPacketProcessor::processPacket() DONE LOOPING FOR %c "
                   "packetData=%p packetLength=%ld voxelData=%p atByte=%d\n",
                    packetType, packetData, packetLength, editData, atByte);
        }

        // Make sure our Node and NodeList knows we've heard from this node.
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
        trackInboundPackets(nodeUUID, sequence, transitTime, editsInPacket, processTime, lockWaitTime);
    } else {
        printf("unknown packet ignored... packetData[0]=%c\n", packetData[0]);
    }
}

void OctreeInboundPacketProcessor::trackInboundPackets(const QUuid& nodeUUID, int sequence, uint64_t transitTime, 
            int editsInPacket, uint64_t processTime, uint64_t lockWaitTime) {
            
    _totalTransitTime += transitTime;
    _totalProcessTime += processTime;
    _totalLockWaitTime += lockWaitTime;
    _totalElementsInPacket += editsInPacket;
    _totalPackets++;
    
    // find the individual senders stats and track them there too...
    // see if this is the first we've heard of this node...
    if (_singleSenderStats.find(nodeUUID) == _singleSenderStats.end()) {
        SingleSenderStats stats;

        stats._totalTransitTime += transitTime;
        stats._totalProcessTime += processTime;
        stats._totalLockWaitTime += lockWaitTime;
        stats._totalElementsInPacket += editsInPacket;
        stats._totalPackets++;

        _singleSenderStats[nodeUUID] = stats;
    } else {
        SingleSenderStats& stats = _singleSenderStats[nodeUUID];
        stats._totalTransitTime += transitTime;
        stats._totalProcessTime += processTime;
        stats._totalLockWaitTime += lockWaitTime;
        stats._totalElementsInPacket += editsInPacket;
        stats._totalPackets++;
    }
}


SingleSenderStats::SingleSenderStats() {
    _totalTransitTime = 0; 
    _totalProcessTime = 0;
    _totalLockWaitTime = 0;
    _totalElementsInPacket = 0;
    _totalPackets = 0;
}


