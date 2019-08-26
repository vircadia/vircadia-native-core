//
//  OctreeEditPacketSender.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OctreeEditPacketSender.h"

#include <assert.h>

#include <PerfStat.h>

#include <OctalCode.h>
#include <udt/PacketHeaders.h>
#include "OctreeLogging.h"

const int OctreeEditPacketSender::DEFAULT_MAX_PENDING_MESSAGES = PacketSender::DEFAULT_PACKETS_PER_SECOND;


OctreeEditPacketSender::OctreeEditPacketSender() :
    _maxPendingMessages(DEFAULT_MAX_PENDING_MESSAGES),
    _releaseQueuedMessagesPending(false)
{
}

OctreeEditPacketSender::~OctreeEditPacketSender() {
    _pendingPacketsLock.lock();
    _preServerSingleMessagePackets.clear();
    _preServerEdits.clear();
    _pendingPacketsLock.unlock();
}


bool OctreeEditPacketSender::serversExist() const {
    auto nodeList = DependencyManager::get<NodeList>();
    if (!nodeList) {
        return false;
    }
    auto node = nodeList->soloNodeOfType(getMyNodeType());
    return node && node->getActiveSocket();
}

// This method is called when the edit packet layer has determined that it has a fully formed packet destined for
// a known nodeID.
void OctreeEditPacketSender::queuePacketToNode(const QUuid& nodeUUID, std::unique_ptr<NLPacket> packet) {

    bool wantDebug = false;
    QMutexLocker lock(&_packetsQueueLock);
    DependencyManager::get<NodeList>()->eachNode([&](const SharedNodePointer& node){
        // only send to the NodeTypes that are getMyNodeType()
        if (node->getType() == getMyNodeType()
            && ((node->getUUID() == nodeUUID) || (nodeUUID.isNull()))
            && node->getActiveSocket()) {

            // jump to the beginning of the payload
            packet->seek(0);

            // pack sequence number
            quint16 sequence = _outgoingSequenceNumbers[nodeUUID]++;
            packet->writePrimitive(sequence);

            // debugging output...
            if (wantDebug) {
                unsigned short int sequence;
                quint64 createdAt;

                packet->seek(0);

                // read the sequence number and createdAt
                packet->readPrimitive(&sequence);
                packet->readPrimitive(&createdAt);

                quint64 queuedAt = usecTimestampNow();
                quint64 transitTime = queuedAt - createdAt;

                qCDebug(octree) << "OctreeEditPacketSender::queuePacketToNode() queued " << packet->getType()
                    << " - command to node bytes=" << packet->getDataSize()
                    << " sequence=" << sequence << " transitTimeSoFar=" << transitTime << " usecs";
            }

            // add packet to history
            _sentPacketHistories[nodeUUID].packetSent(sequence, *packet);

            queuePacketForSending(node, NLPacket::createCopy(*packet));
        }
    });
}

// This method is called when the edit packet layer has determined that it has a fully formed packet destined for
// a known nodeID.
void OctreeEditPacketSender::queuePacketListToNode(const QUuid& nodeUUID, std::unique_ptr<NLPacketList> packetList) {
    DependencyManager::get<NodeList>()->eachNode([&](const SharedNodePointer& node) {
        // only send to the NodeTypes that are getMyNodeType()
        if (node->getType() == getMyNodeType()
            && ((node->getUUID() == nodeUUID) || (nodeUUID.isNull()))
            && node->getActiveSocket()) {

            // NOTE: unlike packets, the packet lists don't get rewritten sequence numbers.
            // or do history for resend
            queuePacketListForSending(node, std::move(packetList));
        }
    });
}

void OctreeEditPacketSender::processPreServerExistsPackets() {
    assert(serversExist()); // we should only be here if we have servers

    // First send out all the single message packets...
    _pendingPacketsLock.lock();
    while (!_preServerSingleMessagePackets.empty()) {
        queuePacketToNodes(std::move(_preServerSingleMessagePackets.front()));
        _preServerSingleMessagePackets.pop_front();
    }

    // Then "process" all the packable messages...
    while (!_preServerEdits.empty()) {
        EditMessagePair& editMessage = _preServerEdits.front();
        queueOctreeEditMessage(editMessage.first, editMessage.second);
        _preServerEdits.pop_front();
    }

    _pendingPacketsLock.unlock();

    // if while waiting for the servers the caller called releaseQueuedMessages()
    // then we want to honor that request now.
    if (_releaseQueuedMessagesPending) {
        releaseQueuedMessages();
        _releaseQueuedMessagesPending = false;
    }
}

void OctreeEditPacketSender::queuePendingPacketToNodes(std::unique_ptr<NLPacket> packet) {
    // If we're asked to save messages while waiting for voxel servers to arrive, then do so...

    if (_maxPendingMessages > 0) {
        _pendingPacketsLock.lock();
        _preServerSingleMessagePackets.push_back(std::move(packet));
        // if we've saved MORE than our max, then clear out the oldest packet...
        int allPendingMessages = (int)(_preServerSingleMessagePackets.size() + _preServerEdits.size());
        if (allPendingMessages > _maxPendingMessages) {
            _preServerSingleMessagePackets.pop_front();
        }
        _pendingPacketsLock.unlock();
    }
}

void OctreeEditPacketSender::queuePacketToNodes(std::unique_ptr<NLPacket> packet) {
    assert(serversExist()); // we must have servers to be here!!

    auto node = DependencyManager::get<NodeList>()->soloNodeOfType(getMyNodeType());
    if (node && node->getActiveSocket()) {
        queuePacketToNode(node->getUUID(), std::move(packet));
    }
}


// NOTE: editMessage - is JUST the octcode/color and does not contain the packet header
void OctreeEditPacketSender::queueOctreeEditMessage(PacketType type, QByteArray& editMessage) {

    // If we don't have servers, then we will simply queue up all of these packets and wait till we have
    // servers for processing
    if (!serversExist()) {
        if (_maxPendingMessages > 0) {
            EditMessagePair messagePair { type, QByteArray(editMessage) };

            _pendingPacketsLock.lock();
            _preServerEdits.push_back(messagePair);

            // if we've saved MORE than out max, then clear out the oldest packet...
            int allPendingMessages = (int)(_preServerSingleMessagePackets.size() + _preServerEdits.size());
            if (allPendingMessages > _maxPendingMessages) {
                _preServerEdits.pop_front();
            }
            _pendingPacketsLock.unlock();
        }
        return; // bail early
    }

    _packetsQueueLock.lock();

    auto node = DependencyManager::get<NodeList>()->soloNodeOfType(getMyNodeType());
    if (node && node->getActiveSocket()) {
        QUuid nodeUUID = node->getUUID();

        // for edit messages, we will attempt to combine multiple edit commands where possible, we
        // don't do this for add because we send those reliably
        if (type == PacketType::EntityAdd) {
            auto newPacket = NLPacketList::create(type, QByteArray(), true, true);
            auto nodeClockSkew = node->getClockSkewUsec();

            // pack sequence number
            quint16 sequence = _outgoingSequenceNumbers[nodeUUID]++;
            newPacket->writePrimitive(sequence);

            // pack in timestamp
            quint64 now = usecTimestampNow() + nodeClockSkew;
            newPacket->writePrimitive(now);


            // We call this virtual function that allows our specific type of EditPacketSender to
            // fixup the buffer for any clock skew
            if (nodeClockSkew != 0) {
                adjustEditPacketForClockSkew(type, editMessage, nodeClockSkew);
            }

            newPacket->write(editMessage);

            // release the new packet
            releaseQueuedPacketList(nodeUUID, std::move(newPacket));

            // tell the sent packet history that we used a sequence number for an untracked packet
            auto& sentPacketHistory = _sentPacketHistories[nodeUUID];
            sentPacketHistory.untrackedPacketSent(sequence);
        } else {
            // only a NLPacket for now
            std::unique_ptr<NLPacket>& bufferedPacket = _pendingEditPackets[nodeUUID].first;

            if (!bufferedPacket) {
                bufferedPacket = initializePacket(type, node->getClockSkewUsec());
            } else {
                // If we're switching type, then we send the last one and start over
                if ((type != bufferedPacket->getType() && bufferedPacket->getPayloadSize() > 0) ||
                    (editMessage.size() >= bufferedPacket->bytesAvailableForWrite())) {

                    // create the new packet and swap it with the packet in _pendingEditPackets
                    auto packetToRelease = initializePacket(type, node->getClockSkewUsec());
                    bufferedPacket.swap(packetToRelease);

                    // release the previously buffered packet
                    releaseQueuedPacket(nodeUUID, std::move(packetToRelease));
                }
            }

            // This is really the first time we know which server/node this particular edit message
            // is going to, so we couldn't adjust for clock skew till now. But here's our chance.
            // We call this virtual function that allows our specific type of EditPacketSender to
            // fixup the buffer for any clock skew
            if (node->getClockSkewUsec() != 0) {
                adjustEditPacketForClockSkew(type, editMessage, node->getClockSkewUsec());
            }

            bufferedPacket->write(editMessage);
        }
    }

    _packetsQueueLock.unlock();

}

void OctreeEditPacketSender::releaseQueuedMessages() {
    // if we don't yet have servers then we can't actually release messages yet because we don't
    // know where to send them to. Instead, just remember this request and when we eventually get servers
    // call release again at that time.
    if (!serversExist()) {
        _releaseQueuedMessagesPending = true;
    } else {
        _packetsQueueLock.lock();
        for (auto& i : _pendingEditPackets) {
            if (i.second.first) {
                // construct a null unique_ptr to an NL packet
                std::unique_ptr<NLPacket> releasedPacket;
                
                // swap the null ptr with the packet we want to release
                i.second.first.swap(releasedPacket);
                
                // move and release the queued packet
                releaseQueuedPacket(i.first, std::move(releasedPacket));
            } else if (i.second.second) {
                // construct a null unique_ptr to an NLPacketList
                std::unique_ptr<NLPacketList> releasedPacketList;

                // swap the null ptr with the NLPacketList we want to release
                i.second.second.swap(releasedPacketList);

                // move and release the queued NLPacketList
                releaseQueuedPacketList(i.first, std::move(releasedPacketList));
            }
            
        }
        _packetsQueueLock.unlock();
    }
}

void OctreeEditPacketSender::releaseQueuedPacket(const QUuid& nodeID, std::unique_ptr<NLPacket> packet) {
    _releaseQueuedPacketMutex.lock();
    if (packet->getPayloadSize() > 0 && packet->getType() != PacketType::Unknown) {
        queuePacketToNode(nodeID, std::move(packet));
    }
    _releaseQueuedPacketMutex.unlock();
}

void OctreeEditPacketSender::releaseQueuedPacketList(const QUuid& nodeID, std::unique_ptr<NLPacketList> packetList) {
    _releaseQueuedPacketMutex.lock();
    if (packetList->getMessageSize() > 0 && packetList->getType() != PacketType::Unknown) {
        queuePacketListToNode(nodeID, std::move(packetList));
    }
    _releaseQueuedPacketMutex.unlock();
}

std::unique_ptr<NLPacket> OctreeEditPacketSender::initializePacket(PacketType type, qint64 nodeClockSkew) {
    auto newPacket = NLPacket::create(type);

    // skip over sequence number for now; will be packed when packet is ready to be sent out
    newPacket->seek(sizeof(quint16));

    // pack in timestamp
    quint64 now = usecTimestampNow() + nodeClockSkew;
    newPacket->writePrimitive(now);

    return newPacket;
}

bool OctreeEditPacketSender::process() {
    // if we have servers, and we have pending pre-servers exist packets, then process those
    // before doing our normal process step. This processPreServerExistPackets()
    if (serversExist() && (!_preServerEdits.empty() || !_preServerSingleMessagePackets.empty() )) {
        processPreServerExistsPackets();
    }

    // base class does most of the work.
    return PacketSender::process();
}

void OctreeEditPacketSender::processNackPacket(ReceivedMessage& message, SharedNodePointer sendingNode) {
    // parse sending node from packet, retrieve packet history for that node

    QMutexLocker lock(&_packetsQueueLock);

    // if packet history doesn't exist for the sender node (somehow), bail
    if (_sentPacketHistories.count(sendingNode->getUUID()) == 0) {
        return;
    }
    
    const SentPacketHistory& sentPacketHistory = _sentPacketHistories[sendingNode->getUUID()];

    // read sequence numbers and queue packets for resend
    while (message.getBytesLeftToRead() > 0) {
        unsigned short int sequenceNumber;
        message.readPrimitive(&sequenceNumber);
        
        // retrieve packet from history
        const NLPacket* packet = sentPacketHistory.getPacket(sequenceNumber);
        if (packet) {
            queuePacketForSending(sendingNode, NLPacket::createCopy(*packet));
        }
    }
}

void OctreeEditPacketSender::nodeKilled(SharedNodePointer node) {
    QMutexLocker lock(&_packetsQueueLock);
    QUuid nodeUUID = node->getUUID();
    _pendingEditPackets.erase(nodeUUID);
    _outgoingSequenceNumbers.erase(nodeUUID);
    _sentPacketHistories.erase(nodeUUID);
}
