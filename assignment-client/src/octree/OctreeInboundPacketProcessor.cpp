//
//  OctreeInboundPacketProcessor.cpp
//  assignment-client/src/octree
//
//  Created by Brad Hefta-Gaub on 8/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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


void OctreeInboundPacketProcessor::processPacket(const SharedNodePointer& sendingNode, const QByteArray& packet) {

    bool debugProcessPacket = _myServer->wantsVerboseDebug();

    if (debugProcessPacket) {
        printf("OctreeInboundPacketProcessor::processPacket() packetData=%p packetLength=%d\n", &packet, packet.size());
    }

    int numBytesPacketHeader = numBytesForPacketHeader(packet);


    // Ask our tree subclass if it can handle the incoming packet...
    PacketType packetType = packetTypeForPacket(packet);
    if (_myServer->getOctree()->handlesEditPacketType(packetType)) {
        PerformanceWarning warn(debugProcessPacket, "processPacket KNOWN TYPE",debugProcessPacket);
        _receivedPacketCount++;
        
        const unsigned char* packetData = reinterpret_cast<const unsigned char*>(packet.data());

        unsigned short int sequence = (*((unsigned short int*)(packetData + numBytesPacketHeader)));
        quint64 sentAt = (*((quint64*)(packetData + numBytesPacketHeader + sizeof(sequence))));
        quint64 arrivedAt = usecTimestampNow();
        quint64 transitTime = arrivedAt - sentAt;
        int editsInPacket = 0;
        quint64 processTime = 0;
        quint64 lockWaitTime = 0;

        if (_myServer->wantsDebugReceiving()) {
            qDebug() << "PROCESSING THREAD: got '" << packetType << "' packet - " << _receivedPacketCount
                    << " command from client receivedBytes=" << packet.size()
                    << " sequence=" << sequence << " transitTime=" << transitTime << " usecs";
        }
        int atByte = numBytesPacketHeader + sizeof(sequence) + sizeof(sentAt);
        unsigned char* editData = (unsigned char*)&packetData[atByte];
        while (atByte < packet.size()) {
            int maxSize = packet.size() - atByte;

            if (debugProcessPacket) {
                printf("OctreeInboundPacketProcessor::processPacket() %c "
                       "packetData=%p packetLength=%d voxelData=%p atByte=%d maxSize=%d\n",
                        packetType, packetData, packet.size(), editData, atByte, maxSize);
            }

            quint64 startLock = usecTimestampNow();
            _myServer->getOctree()->lockForWrite();
            quint64 startProcess = usecTimestampNow();
            int editDataBytesRead = _myServer->getOctree()->processEditPacketData(packetType,
                                                                                  reinterpret_cast<const unsigned char*>(packet.data()),
                                                                                  packet.size(),
                                                                                  editData, maxSize, sendingNode);
            _myServer->getOctree()->unlock();
            quint64 endProcess = usecTimestampNow();

            editsInPacket++;
            quint64 thisProcessTime = endProcess - startProcess;
            quint64 thisLockWaitTime = startProcess - startLock;
            processTime += thisProcessTime;
            lockWaitTime += thisLockWaitTime;

            // skip to next voxel edit record in the packet
            editData += editDataBytesRead;
            atByte += editDataBytesRead;
        }

        if (debugProcessPacket) {
            printf("OctreeInboundPacketProcessor::processPacket() DONE LOOPING FOR %c "
                   "packetData=%p packetLength=%d voxelData=%p atByte=%d\n",
                    packetType, packetData, packet.size(), editData, atByte);
        }

        // Make sure our Node and NodeList knows we've heard from this node.
        QUuid& nodeUUID = DEFAULT_NODE_ID_REF;
        if (sendingNode) {
            sendingNode->setLastHeardMicrostamp(usecTimestampNow());
            nodeUUID = sendingNode->getUUID();
            if (debugProcessPacket) {
                qDebug() << "sender has uuid=" << nodeUUID;
            }
        } else {
            if (debugProcessPacket) {
                qDebug() << "sender has no known nodeUUID.";
            }
        }
        trackInboundPackets(nodeUUID, sequence, transitTime, editsInPacket, processTime, lockWaitTime);
    } else {
        qDebug("unknown packet ignored... packetType=%d", packetType);
    }
}

void OctreeInboundPacketProcessor::trackInboundPackets(const QUuid& nodeUUID, int sequence, quint64 transitTime,
            int editsInPacket, quint64 processTime, quint64 lockWaitTime) {

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


