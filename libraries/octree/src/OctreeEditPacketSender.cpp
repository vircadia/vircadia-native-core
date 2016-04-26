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

#include <assert.h>

#include <PerfStat.h>

#include <OctalCode.h>
#include <udt/PacketHeaders.h>
#include "OctreeLogging.h"
#include "OctreeEditPacketSender.h"

const int OctreeEditPacketSender::DEFAULT_MAX_PENDING_MESSAGES = PacketSender::DEFAULT_PACKETS_PER_SECOND;


OctreeEditPacketSender::OctreeEditPacketSender() :
    PacketSender(),
    _shouldSend(true),
    _maxPendingMessages(DEFAULT_MAX_PENDING_MESSAGES),
    _releaseQueuedMessagesPending(false),
    _serverJurisdictions(NULL)
{

}

OctreeEditPacketSender::~OctreeEditPacketSender() {
    _pendingPacketsLock.lock();
    _preServerSingleMessagePackets.clear();
    _preServerEdits.clear();
    _pendingPacketsLock.unlock();
}


bool OctreeEditPacketSender::serversExist() const {
    bool hasServers = false;
    bool atLeastOneJurisdictionMissing = false; // assume the best

    DependencyManager::get<NodeList>()->eachNodeBreakable([&](const SharedNodePointer& node){
        if (node->getType() == getMyNodeType() && node->getActiveSocket()) {

            QUuid nodeUUID = node->getUUID();
            // If we've got Jurisdictions set, then check to see if we know the jurisdiction for this server
            if (_serverJurisdictions) {
                // lookup our nodeUUID in the jurisdiction map, if it's missing then we're
                // missing at least one jurisdiction
                _serverJurisdictions->withReadLock([&] {
                    if ((*_serverJurisdictions).find(nodeUUID) == (*_serverJurisdictions).end()) {
                        atLeastOneJurisdictionMissing = true;
                    }
                });
            }
            hasServers = true;
        }

        if (atLeastOneJurisdictionMissing) {
            return false; // no point in looking further - return false from anonymous function
        } else {
            return true;
        }
    });

    return (hasServers && !atLeastOneJurisdictionMissing);
}

// This method is called when the edit packet layer has determined that it has a fully formed packet destined for
// a known nodeID.
void OctreeEditPacketSender::queuePacketToNode(const QUuid& nodeUUID, std::unique_ptr<NLPacket> packet) {

    bool wantDebug = false;
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

void OctreeEditPacketSender::processPreServerExistsPackets() {
    assert(serversExist()); // we should only be here if we have jurisdictions

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

    // if while waiting for the jurisdictions the caller called releaseQueuedMessages()
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
    if (!_shouldSend) {
        return; // bail early
    }

    assert(serversExist()); // we must have jurisdictions to be here!!

    const unsigned char* octCode = reinterpret_cast<unsigned char*>(packet->getPayload()) + sizeof(short) + sizeof(quint64);

    // We want to filter out edit messages for servers based on the server's Jurisdiction
    // But we can't really do that with a packed message, since each edit message could be destined
    // for a different server... So we need to actually manage multiple queued packets... one
    // for each server

    DependencyManager::get<NodeList>()->eachNode([&](const SharedNodePointer& node){
        // only send to the NodeTypes that are getMyNodeType()
        if (node->getActiveSocket() && node->getType() == getMyNodeType()) {
            QUuid nodeUUID = node->getUUID();
            bool isMyJurisdiction = true;
            // we need to get the jurisdiction for this
            // here we need to get the "pending packet" for this server
            _serverJurisdictions->withReadLock([&] {
                const JurisdictionMap& map = (*_serverJurisdictions)[nodeUUID];
                isMyJurisdiction = (map.isMyJurisdiction(octCode, CHECK_NODE_ONLY) == JurisdictionMap::WITHIN);
            });

            if (isMyJurisdiction) {
                // make a copy of this packet for this node and queue
                auto packetCopy = NLPacket::createCopy(*packet);
                queuePacketToNode(nodeUUID, std::move(packetCopy));
            }
        }
    });
}


// NOTE: editMessage - is JUST the octcode/color and does not contain the packet header
void OctreeEditPacketSender::queueOctreeEditMessage(PacketType type, QByteArray& editMessage) {

    if (!_shouldSend) {
        return; // bail early
    }

    // If we don't have jurisdictions, then we will simply queue up all of these packets and wait till we have
    // jurisdictions for processing
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

    // We want to filter out edit messages for servers based on the server's Jurisdiction
    // But we can't really do that with a packed message, since each edit message could be destined
    // for a different server... So we need to actually manage multiple queued packets... one
    // for each server
    _packetsQueueLock.lock();

    DependencyManager::get<NodeList>()->eachNode([&](const SharedNodePointer& node){
        // only send to the NodeTypes that are getMyNodeType()
        if (node->getActiveSocket() && node->getType() == getMyNodeType()) {
            QUuid nodeUUID = node->getUUID();
            bool isMyJurisdiction = true;

            if (type == PacketType::EntityErase) {
                isMyJurisdiction = true; // send erase messages to all servers
            } else if (_serverJurisdictions) {
                // we need to get the jurisdiction for this
                // here we need to get the "pending packet" for this server
                _serverJurisdictions->withReadLock([&] {
                    if ((*_serverJurisdictions).find(nodeUUID) != (*_serverJurisdictions).end()) {
                        const JurisdictionMap& map = (*_serverJurisdictions)[nodeUUID];
                        isMyJurisdiction = (map.isMyJurisdiction(reinterpret_cast<const unsigned char*>(editMessage.data()),
                            CHECK_NODE_ONLY) == JurisdictionMap::WITHIN);
                    } else {
                        isMyJurisdiction = false;
                    }
                });
            }
            if (isMyJurisdiction) {
                std::unique_ptr<NLPacket>& bufferedPacket = _pendingEditPackets[nodeUUID];

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
    });

    _packetsQueueLock.unlock();

}

void OctreeEditPacketSender::releaseQueuedMessages() {
    // if we don't yet have jurisdictions then we can't actually release messages yet because we don't
    // know where to send them to. Instead, just remember this request and when we eventually get jurisdictions
    // call release again at that time.
    if (!serversExist()) {
        _releaseQueuedMessagesPending = true;
    } else {
        _packetsQueueLock.lock();
        for (auto& i : _pendingEditPackets) {
            if (i.second) {
                // construct a null unique_ptr to an NL packet
                std::unique_ptr<NLPacket> releasedPacket;
                
                // swap the null ptr with the packet we want to release
                i.second.swap(releasedPacket);
                
                // move and release the queued packet
                releaseQueuedPacket(i.first, std::move(releasedPacket));
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
    // if we have server jurisdiction details, and we have pending pre-jurisdiction packets, then process those
    // before doing our normal process step. This processPreJurisdictionPackets()
    if (serversExist() && (!_preServerEdits.empty() || !_preServerSingleMessagePackets.empty() )) {
        processPreServerExistsPackets();
    }

    // base class does most of the work.
    return PacketSender::process();
}

void OctreeEditPacketSender::processNackPacket(ReceivedMessage& message, SharedNodePointer sendingNode) {
    // parse sending node from packet, retrieve packet history for that node

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
    // TODO: add locks
    QUuid nodeUUID = node->getUUID();
    _pendingEditPackets.erase(nodeUUID);
    _outgoingSequenceNumbers.erase(nodeUUID);
    _sentPacketHistories.erase(nodeUUID);
}
