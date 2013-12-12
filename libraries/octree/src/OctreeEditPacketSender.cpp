//
//  OctreeEditPacketSender.cpp
//  interface
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded packet Sender for the Application
//

#include <assert.h>

#include <PerfStat.h>

#include <OctalCode.h>
#include <PacketHeaders.h>
#include "OctreeEditPacketSender.h"


EditPacketBuffer::EditPacketBuffer(PACKET_TYPE type, unsigned char* buffer, ssize_t length, QUuid nodeUUID) {
    _nodeUUID = nodeUUID;
    _currentType = type;
    _currentSize = length;
    memcpy(_currentBuffer, buffer, length); 
};

const int OctreeEditPacketSender::DEFAULT_MAX_PENDING_MESSAGES = PacketSender::DEFAULT_PACKETS_PER_SECOND; 


OctreeEditPacketSender::OctreeEditPacketSender(PacketSenderNotify* notify) : 
    PacketSender(notify), 
    _shouldSend(true),
    _maxPendingMessages(DEFAULT_MAX_PENDING_MESSAGES),
    _releaseQueuedMessagesPending(false),
    _serverJurisdictions(NULL),
    _sequenceNumber(0),
    _maxPacketSize(MAX_PACKET_SIZE) {
}

OctreeEditPacketSender::~OctreeEditPacketSender() {
    while (!_preServerSingleMessagePackets.empty()) {
        EditPacketBuffer* packet = _preServerSingleMessagePackets.front();
        delete packet;
        _preServerSingleMessagePackets.erase(_preServerSingleMessagePackets.begin());
    }
    while (!_preServerPackets.empty()) {
        EditPacketBuffer* packet = _preServerPackets.front();
        delete packet;
        _preServerPackets.erase(_preServerPackets.begin());
    }
}


bool OctreeEditPacketSender::serversExist() const {
    bool hasServers = false;
    bool atLeastOnJurisdictionMissing = false; // assume the best
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        // only send to the NodeTypes that are getMyNodeType()
        if (node->getType() == getMyNodeType()) {
            if (nodeList->getNodeActiveSocketOrPing(&(*node))) {
                QUuid nodeUUID = node->getUUID();
                // If we've got Jurisdictions set, then check to see if we know the jurisdiction for this server
                if (_serverJurisdictions) {
                    // lookup our nodeUUID in the jurisdiction map, if it's missing then we're 
                    // missing at least one jurisdiction
                    if ((*_serverJurisdictions).find(nodeUUID) == (*_serverJurisdictions).end()) {
                        atLeastOnJurisdictionMissing = true;
                    }
                }
                hasServers = true;
            }
        }
        if (atLeastOnJurisdictionMissing) {
            break; // no point in looking further...
        }
    }
    return (hasServers && !atLeastOnJurisdictionMissing);
}

// This method is called when the edit packet layer has determined that it has a fully formed packet destined for
// a known nodeID. However, we also want to handle the case where the 
void OctreeEditPacketSender::queuePacketToNode(const QUuid& nodeUUID, unsigned char* buffer, ssize_t length) {
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        // only send to the NodeTypes that are getMyNodeType()
        if (node->getType() == getMyNodeType() &&
            ((node->getUUID() == nodeUUID) || (nodeUUID.isNull()))) {
            if (nodeList->getNodeActiveSocketOrPing(&(*node))) {
                const HifiSockAddr* nodeAddress = node->getActiveSocket();
                queuePacketForSending(*nodeAddress, buffer, length);
                
                // debugging output...
                bool wantDebugging = false;
                if (wantDebugging) {
                    int numBytesPacketHeader = numBytesForPacketHeader(buffer);
                    unsigned short int sequence = (*((unsigned short int*)(buffer + numBytesPacketHeader)));
                    uint64_t createdAt = (*((uint64_t*)(buffer + numBytesPacketHeader + sizeof(sequence))));
                    uint64_t queuedAt = usecTimestampNow();
                    uint64_t transitTime = queuedAt - createdAt;
                    printf("OctreeEditPacketSender::queuePacketToNode() queued %c - command to node bytes=%ld sequence=%d transitTimeSoFar=%llu usecs\n",
                        buffer[0], length, sequence, transitTime);
                }                
            }
        }
    }
}

void OctreeEditPacketSender::processPreServerExistsPackets() {
    assert(serversExist()); // we should only be here if we have jurisdictions
    
    // First send out all the single message packets...
    while (!_preServerSingleMessagePackets.empty()) {
        EditPacketBuffer* packet = _preServerSingleMessagePackets.front();
        queuePacketToNodes(&packet->_currentBuffer[0], packet->_currentSize);
        delete packet;
        _preServerSingleMessagePackets.erase(_preServerSingleMessagePackets.begin());
    }

    // Then "process" all the packable messages...
    while (!_preServerPackets.empty()) {
        EditPacketBuffer* packet = _preServerPackets.front();
        queueOctreeEditMessage(packet->_currentType, &packet->_currentBuffer[0], packet->_currentSize);
        delete packet;
        _preServerPackets.erase(_preServerPackets.begin());
    }

    // if while waiting for the jurisdictions the caller called releaseQueuedMessages() 
    // then we want to honor that request now.
    if (_releaseQueuedMessagesPending) {
        releaseQueuedMessages();
        _releaseQueuedMessagesPending = false;
    }
}

void OctreeEditPacketSender::queuePendingPacketToNodes(PACKET_TYPE type, unsigned char* buffer, ssize_t length) {
    // If we're asked to save messages while waiting for voxel servers to arrive, then do so...
    if (_maxPendingMessages > 0) {
        EditPacketBuffer* packet = new EditPacketBuffer(type, buffer, length);
        _preServerSingleMessagePackets.push_back(packet);
        // if we've saved MORE than our max, then clear out the oldest packet...
        int allPendingMessages = _preServerSingleMessagePackets.size() + _preServerPackets.size();
        if (allPendingMessages > _maxPendingMessages) {
            EditPacketBuffer* packet = _preServerSingleMessagePackets.front();
            delete packet;
            _preServerSingleMessagePackets.erase(_preServerSingleMessagePackets.begin());
        }
    }
}

void OctreeEditPacketSender::queuePacketToNodes(unsigned char* buffer, ssize_t length) {
    if (!_shouldSend) {
        return; // bail early
    }
    
    assert(serversExist()); // we must have jurisdictions to be here!!

    int headerBytes = numBytesForPacketHeader(buffer) + sizeof(short) + sizeof(uint64_t);
    unsigned char* octCode = buffer + headerBytes; // skip the packet header to get to the octcode
    
    // We want to filter out edit messages for servers based on the server's Jurisdiction
    // But we can't really do that with a packed message, since each edit message could be destined 
    // for a different server... So we need to actually manage multiple queued packets... one
    // for each server
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        // only send to the NodeTypes that are getMyNodeType()
        if (node->getActiveSocket() != NULL && node->getType() == getMyNodeType()) {
            QUuid nodeUUID = node->getUUID();
            bool isMyJurisdiction = true;
            // we need to get the jurisdiction for this 
            // here we need to get the "pending packet" for this server
            const JurisdictionMap& map = (*_serverJurisdictions)[nodeUUID];
            isMyJurisdiction = (map.isMyJurisdiction(octCode, CHECK_NODE_ONLY) == JurisdictionMap::WITHIN);
            if (isMyJurisdiction) {
                queuePacketToNode(nodeUUID, buffer, length);
            }
        }
    }
}


// NOTE: codeColorBuffer - is JUST the octcode/color and does not contain the packet header!
void OctreeEditPacketSender::queueOctreeEditMessage(PACKET_TYPE type, unsigned char* codeColorBuffer, ssize_t length) {
    if (!_shouldSend) {
        return; // bail early
    }
    
    // If we don't have jurisdictions, then we will simply queue up all of these packets and wait till we have
    // jurisdictions for processing
    if (!serversExist()) {
        if (_maxPendingMessages > 0) {
            EditPacketBuffer* packet = new EditPacketBuffer(type, codeColorBuffer, length);
            _preServerPackets.push_back(packet);

            // if we've saved MORE than out max, then clear out the oldest packet...
            int allPendingMessages = _preServerSingleMessagePackets.size() + _preServerPackets.size();
            if (allPendingMessages > _maxPendingMessages) {
                EditPacketBuffer* packet = _preServerPackets.front();
                delete packet;
                _preServerPackets.erase(_preServerPackets.begin());
            }
        }
        return; // bail early
    }
    
    // We want to filter out edit messages for servers based on the server's Jurisdiction
    // But we can't really do that with a packed message, since each edit message could be destined 
    // for a different server... So we need to actually manage multiple queued packets... one
    // for each server
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        // only send to the NodeTypes that are getMyNodeType()
        if (node->getActiveSocket() != NULL && node->getType() == getMyNodeType()) {
            QUuid nodeUUID = node->getUUID();
            bool isMyJurisdiction = true;

            if (_serverJurisdictions) {
                // we need to get the jurisdiction for this 
                // here we need to get the "pending packet" for this server
                if ((*_serverJurisdictions).find(nodeUUID) != (*_serverJurisdictions).end()) {
                    const JurisdictionMap& map = (*_serverJurisdictions)[nodeUUID];
                    isMyJurisdiction = (map.isMyJurisdiction(codeColorBuffer, CHECK_NODE_ONLY) == JurisdictionMap::WITHIN);
                } else {
                    isMyJurisdiction = false;
                }
            }
            if (isMyJurisdiction) {
                EditPacketBuffer& packetBuffer = _pendingEditPackets[nodeUUID];
                packetBuffer._nodeUUID = nodeUUID;
            
                // If we're switching type, then we send the last one and start over
                if ((type != packetBuffer._currentType && packetBuffer._currentSize > 0) || 
                    (packetBuffer._currentSize + length >= _maxPacketSize)) {
                    releaseQueuedPacket(packetBuffer);
                    initializePacket(packetBuffer, type);
                }

                // If the buffer is empty and not correctly initialized for our type...
                if (type != packetBuffer._currentType && packetBuffer._currentSize == 0) {
                    initializePacket(packetBuffer, type);
                }

                memcpy(&packetBuffer._currentBuffer[packetBuffer._currentSize], codeColorBuffer, length);
                packetBuffer._currentSize += length;
            }
        }
    }
}

void OctreeEditPacketSender::releaseQueuedMessages() {
    // if we don't yet have jurisdictions then we can't actually release messages yet because we don't 
    // know where to send them to. Instead, just remember this request and when we eventually get jurisdictions
    // call release again at that time.
    if (!serversExist()) {
        _releaseQueuedMessagesPending = true;
    } else {
        for (std::map<QUuid, EditPacketBuffer>::iterator i = _pendingEditPackets.begin(); i != _pendingEditPackets.end(); i++) {
            releaseQueuedPacket(i->second);
        }
    }
}

void OctreeEditPacketSender::releaseQueuedPacket(EditPacketBuffer& packetBuffer) {
    if (packetBuffer._currentSize > 0 && packetBuffer._currentType != PACKET_TYPE_UNKNOWN) {
        queuePacketToNode(packetBuffer._nodeUUID, &packetBuffer._currentBuffer[0], packetBuffer._currentSize);
    }
    packetBuffer._currentSize = 0;
    packetBuffer._currentType = PACKET_TYPE_UNKNOWN;
}

void OctreeEditPacketSender::initializePacket(EditPacketBuffer& packetBuffer, PACKET_TYPE type) {
    packetBuffer._currentSize = populateTypeAndVersion(&packetBuffer._currentBuffer[0], type);

    // pack in sequence number
    unsigned short int* sequenceAt = (unsigned short int*)&packetBuffer._currentBuffer[packetBuffer._currentSize];
    *sequenceAt = _sequenceNumber;
    packetBuffer._currentSize += sizeof(unsigned short int); // nudge past sequence
    _sequenceNumber++;

    // pack in timestamp
    uint64_t now = usecTimestampNow();
    uint64_t* timeAt = (uint64_t*)&packetBuffer._currentBuffer[packetBuffer._currentSize];
    *timeAt = now;
    packetBuffer._currentSize += sizeof(uint64_t); // nudge past timestamp

    packetBuffer._currentType = type;
}

bool OctreeEditPacketSender::process() {
    // if we have server jurisdiction details, and we have pending pre-jurisdiction packets, then process those
    // before doing our normal process step. This processPreJurisdictionPackets()
    if (serversExist() && (!_preServerPackets.empty() || !_preServerSingleMessagePackets.empty() )) {
        processPreServerExistsPackets();
    }

    // base class does most of the work.
    return PacketSender::process();
}
