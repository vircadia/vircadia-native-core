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

#include <limits>

#include <NumericalConstants.h>
#include <udt/PacketHeaders.h>
#include <PerfStat.h>

#include "OctreeServer.h"
#include "OctreeServerConsts.h"
#include "OctreeInboundPacketProcessor.h"

static QUuid DEFAULT_NODE_ID_REF;
const quint64 TOO_LONG_SINCE_LAST_NACK = 1 * USECS_PER_SECOND;

OctreeInboundPacketProcessor::OctreeInboundPacketProcessor(OctreeServer* myServer) :
    _myServer(myServer),
    _receivedPacketCount(0),
    _totalTransitTime(0),
    _totalProcessTime(0),
    _totalLockWaitTime(0),
    _totalElementsInPacket(0),
    _totalPackets(0),
    _lastNackTime(usecTimestampNow()),
    _shuttingDown(false)
{
}

void OctreeInboundPacketProcessor::resetStats() {
    _totalTransitTime = 0;
    _totalProcessTime = 0;
    _totalLockWaitTime = 0;
    _totalElementsInPacket = 0;
    _totalPackets = 0;
    _lastNackTime = usecTimestampNow();

    _singleSenderStats.clear();
}

unsigned long OctreeInboundPacketProcessor::getMaxWait() const {
    // calculate time until next sendNackPackets()
    quint64 nextNackTime = _lastNackTime + TOO_LONG_SINCE_LAST_NACK;
    quint64 now = usecTimestampNow();
    if (now >= nextNackTime) {
        return 0;
    }
    return (nextNackTime - now) / USECS_PER_MSEC + 1;
}

void OctreeInboundPacketProcessor::preProcess() {
    // check if it's time to send a nack. If yes, do so
    quint64 now = usecTimestampNow();
    if (now - _lastNackTime >= TOO_LONG_SINCE_LAST_NACK) {
        _lastNackTime = now;
        sendNackPackets();
    }
}

void OctreeInboundPacketProcessor::midProcess() {
    // check if it's time to send a nack. If yes, do so
    quint64 now = usecTimestampNow();
    if (now - _lastNackTime >= TOO_LONG_SINCE_LAST_NACK) {
        _lastNackTime = now;
        sendNackPackets();
    }
}

void OctreeInboundPacketProcessor::processPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode) {
    if (_shuttingDown) {
        qDebug() << "OctreeInboundPacketProcessor::processPacket() while shutting down... ignoring incoming packet";
        return;
    }

    bool debugProcessPacket = _myServer->wantsVerboseDebug();

    if (debugProcessPacket) {
        qDebug("OctreeInboundPacketProcessor::processPacket() payload=%p payloadLength=%lld",
               packet->getPayload(),
               packet->getPayloadSize());
    }

    // Ask our tree subclass if it can handle the incoming packet...
    PacketType packetType = packet->getType();
    
    if (_myServer->getOctree()->handlesEditPacketType(packetType)) {
        PerformanceWarning warn(debugProcessPacket, "processPacket KNOWN TYPE", debugProcessPacket);
        _receivedPacketCount++;

        unsigned short int sequence;
        packet->readPrimitive(&sequence);

        quint64 sentAt;
        packet->readPrimitive(&sentAt);
        
        quint64 arrivedAt = usecTimestampNow();
        if (sentAt > arrivedAt) {
            if (debugProcessPacket || _myServer->wantsDebugReceiving()) {
                qDebug() << "unreasonable sentAt=" << sentAt << " usecs";
                qDebug() << "setting sentAt to arrivedAt=" << arrivedAt << " usecs";
            }
            sentAt = arrivedAt;
        }

        quint64 transitTime = arrivedAt - sentAt;
        int editsInPacket = 0;
        quint64 processTime = 0;
        quint64 lockWaitTime = 0;

        if (debugProcessPacket || _myServer->wantsDebugReceiving()) {
            qDebug() << "PROCESSING THREAD: got '" << packetType << "' packet - " << _receivedPacketCount << " command from client";
            qDebug() << "    receivedBytes=" << packet->getDataSize();
            qDebug() << "         sequence=" << sequence;
            qDebug() << "           sentAt=" << sentAt << " usecs";
            qDebug() << "        arrivedAt=" << arrivedAt << " usecs";
            qDebug() << "      transitTime=" << transitTime << " usecs";
            qDebug() << "      sendingNode->getClockSkewUsec()=" << sendingNode->getClockSkewUsec() << " usecs";


        }

        if (debugProcessPacket) {
            qDebug() << "    numBytesPacketHeader=" << NLPacket::totalHeaderSize(packetType);
            qDebug() << "    sizeof(sequence)=" << sizeof(sequence);
            qDebug() << "    sizeof(sentAt)=" << sizeof(sentAt);
            qDebug() << "    atByte (in payload)=" << packet->pos();
            qDebug() << "    payload size=" << packet->getPayloadSize();

            if (!packet->bytesLeftToRead()) {
                qDebug() << "    ----- UNEXPECTED ---- got a packet without any edit details!!!! --------";
            }
        }
        
        const unsigned char* editData = nullptr;
        
        while (packet->bytesLeftToRead() > 0) {

            editData = reinterpret_cast<const unsigned char*>(packet->getPayload() + packet->pos());

            int maxSize = packet->bytesLeftToRead();

            if (debugProcessPacket) {
                qDebug() << " --- inside while loop ---";
                qDebug() << "    maxSize=" << maxSize;
                qDebug("OctreeInboundPacketProcessor::processPacket() %hhu "
                       "payload=%p payloadLength=%lld editData=%p payloadPosition=%lld maxSize=%d",
                        packetType, packet->getPayload(), packet->getPayloadSize(), editData,
                        packet->pos(), maxSize);
            }

            quint64 startLock = usecTimestampNow();
            _myServer->getOctree()->lockForWrite();
            quint64 startProcess = usecTimestampNow();
            int editDataBytesRead =
                _myServer->getOctree()->processEditPacketData(*packet, editData, maxSize, sendingNode);

            if (debugProcessPacket) {
                qDebug() << "OctreeInboundPacketProcessor::processPacket() after processEditPacketData()..."
                                << "editDataBytesRead=" << editDataBytesRead;
            }

            _myServer->getOctree()->unlock();
            quint64 endProcess = usecTimestampNow();

            editsInPacket++;
            quint64 thisProcessTime = endProcess - startProcess;
            quint64 thisLockWaitTime = startProcess - startLock;
            processTime += thisProcessTime;
            lockWaitTime += thisLockWaitTime;

            // skip to next edit record in the packet
            packet->seek(packet->pos() + editDataBytesRead);

            if (debugProcessPacket) {
                qDebug() << "    editDataBytesRead=" << editDataBytesRead;
                qDebug() << "    AFTER processEditPacketData payload position=" << packet->pos();
                qDebug() << "    AFTER processEditPacketData payload size=" << packet->getPayloadSize();
            }

        }

        if (debugProcessPacket) {
            qDebug("OctreeInboundPacketProcessor::processPacket() DONE LOOPING FOR %hhu "
                   "payload=%p payloadLength=%lld editData=%p payloadPosition=%lld",
                    packetType, packet->getPayload(), packet->getPayloadSize(), editData, packet->pos());
        }

        // Make sure our Node and NodeList knows we've heard from this node.
        QUuid& nodeUUID = DEFAULT_NODE_ID_REF;
        if (sendingNode) {
            nodeUUID = sendingNode->getUUID();
            if (debugProcessPacket) {
                qDebug() << "sender has uuid=" << nodeUUID;
            }
        } else {
            if (debugProcessPacket) {
                qDebug() << "sender has no known nodeUUID.";
            }
        }
        trackInboundPacket(nodeUUID, sequence, transitTime, editsInPacket, processTime, lockWaitTime);
    } else {
        qDebug("unknown packet ignored... packetType=%hhu", packetType);
    }
}

void OctreeInboundPacketProcessor::trackInboundPacket(const QUuid& nodeUUID, unsigned short int sequence, quint64 transitTime,
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
        stats.trackInboundPacket(sequence, transitTime, editsInPacket, processTime, lockWaitTime);
        _singleSenderStats[nodeUUID] = stats;
    } else {
        SingleSenderStats& stats = _singleSenderStats[nodeUUID];
        stats.trackInboundPacket(sequence, transitTime, editsInPacket, processTime, lockWaitTime);
    }
}

int OctreeInboundPacketProcessor::sendNackPackets() {
    if (_shuttingDown) {
        qDebug() << "OctreeInboundPacketProcessor::sendNackPackets() while shutting down... ignore";
        return 0;
    }

    NLPacketList nackPacketList(_myServer->getMyEditNackType());
    auto nodeList = DependencyManager::get<NodeList>();
    int packetsSent = 0;

    NodeToSenderStatsMapIterator i = _singleSenderStats.begin();
    while (i != _singleSenderStats.end()) {

        QUuid nodeUUID = i.key();
        SingleSenderStats nodeStats = i.value();
        
        // check if this node is still alive.  Remove its stats if it's dead.
        if (!isAlive(nodeUUID)) {
            i = _singleSenderStats.erase(i);
            continue;
        }

        // if there are packets from _node that are waiting to be processed,
        // don't send a NACK since the missing packets may be among those waiting packets.
        if (hasPacketsToProcessFrom(nodeUUID)) {
            ++i;
            continue;
        }

        const SharedNodePointer& destinationNode = DependencyManager::get<NodeList>()->nodeWithUUID(nodeUUID);

        // retrieve sequence number stats of node, prune its missing set
        SequenceNumberStats& sequenceNumberStats = nodeStats.getIncomingEditSequenceNumberStats();
        sequenceNumberStats.pruneMissingSet();

        // construct nack packet(s) for this node
        const QSet<unsigned short int>& missingSequenceNumbers = sequenceNumberStats.getMissingSet();

        auto it = missingSequenceNumbers.constBegin();

        while (it != missingSequenceNumbers.constEnd()) {
            unsigned short int sequenceNumber = *it;
            nackPacketList.writePrimitive(sequenceNumber);
            ++it;
        }
        
        
        if (nackPacketList.getNumPackets()) {
            qDebug() << "NACK Sent back to editor/client... destinationNode=" << nodeUUID;
            
            packetsSent += nackPacketList.getNumPackets();
            
            // send the list of nack packets
            nodeList->sendPacketList(nackPacketList, *destinationNode);
        }
        
        ++i;
    }

    return packetsSent;
}


SingleSenderStats::SingleSenderStats()
    : _totalTransitTime(0),
    _totalProcessTime(0),
    _totalLockWaitTime(0),
    _totalElementsInPacket(0),
    _totalPackets(0),
    _incomingEditSequenceNumberStats()
{

}

void SingleSenderStats::trackInboundPacket(unsigned short int incomingSequence, quint64 transitTime,
    int editsInPacket, quint64 processTime, quint64 lockWaitTime) {

    // track sequence number
    _incomingEditSequenceNumberStats.sequenceNumberReceived(incomingSequence);

    // update other stats
    _totalTransitTime += transitTime;
    _totalProcessTime += processTime;
    _totalLockWaitTime += lockWaitTime;
    _totalElementsInPacket += editsInPacket;
    _totalPackets++;
}
