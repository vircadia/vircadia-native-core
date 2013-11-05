//
//  VoxelEditPacketSender.cpp
//  interface
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded voxel packet Sender for the Application
//

#include <assert.h>

#include <PerfStat.h>

#include <OctalCode.h>
#include <PacketHeaders.h>
#include "VoxelEditPacketSender.h"


EditPacketBuffer::EditPacketBuffer(PACKET_TYPE type, unsigned char* buffer, ssize_t length, QUuid nodeUUID) {
    _nodeUUID = nodeUUID;
    _currentType = type;
    _currentSize = length;
    memcpy(_currentBuffer, buffer, length); 
};

const int VoxelEditPacketSender::DEFAULT_MAX_PENDING_MESSAGES = PacketSender::DEFAULT_PACKETS_PER_SECOND; 


VoxelEditPacketSender::VoxelEditPacketSender(PacketSenderNotify* notify) : 
    PacketSender(notify), 
    _shouldSend(true),
    _maxPendingMessages(DEFAULT_MAX_PENDING_MESSAGES),
    _releaseQueuedMessagesPending(false),
    _voxelServerJurisdictions(NULL) {
}

VoxelEditPacketSender::~VoxelEditPacketSender() {
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


void VoxelEditPacketSender::sendVoxelEditMessage(PACKET_TYPE type, VoxelDetail& detail) {
    // allows app to disable sending if for example voxels have been disabled
    if (!_shouldSend) {
        return; // bail early
    }

    unsigned char* bufferOut;
    int sizeOut;

    // This encodes the voxel edit message into a buffer...
    if (createVoxelEditMessage(type, 0, 1, &detail, bufferOut, sizeOut)){
        // If we don't have voxel jurisdictions, then we will simply queue up these packets and wait till we have
        // jurisdictions for processing
        if (!voxelServersExist()) {
            // If we're asked to save messages while waiting for voxel servers to arrive, then do so...
            if (_maxPendingMessages > 0) {
                EditPacketBuffer* packet = new EditPacketBuffer(type, bufferOut, sizeOut);
                _preServerSingleMessagePackets.push_back(packet);
                // if we've saved MORE than out max, then clear out the oldest packet...
                int allPendingMessages = _preServerSingleMessagePackets.size() + _preServerPackets.size();
                if (allPendingMessages > _maxPendingMessages) {
                    EditPacketBuffer* packet = _preServerSingleMessagePackets.front();
                    delete packet;
                    _preServerSingleMessagePackets.erase(_preServerSingleMessagePackets.begin());
                }
            }
            return; // bail early
        } else {
            queuePacketToNodes(bufferOut, sizeOut);
        }
        
        // either way, clean up the created buffer
        delete[] bufferOut;
    }
}

bool VoxelEditPacketSender::voxelServersExist() const {
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        // only send to the NodeTypes that are NODE_TYPE_VOXEL_SERVER
        if (node->getType() == NODE_TYPE_VOXEL_SERVER) {
            if (nodeList->getNodeActiveSocketOrPing(&(*node))) {
                return true;
            }
        }
    }
    return false;
}

// This method is called when the edit packet layer has determined that it has a fully formed packet destined for
// a known nodeID. However, we also want to handle the case where the 
void VoxelEditPacketSender::queuePacketToNode(const QUuid& nodeUUID, unsigned char* buffer, ssize_t length) {
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        // only send to the NodeTypes that are NODE_TYPE_VOXEL_SERVER
        if (node->getType() == NODE_TYPE_VOXEL_SERVER &&
            ((node->getUUID() == nodeUUID) || (nodeUUID.isNull()))) {
            if (nodeList->getNodeActiveSocketOrPing(&(*node))) {
                sockaddr* nodeAddress = node->getActiveSocket();
                queuePacketForSending(*nodeAddress, buffer, length);
            }
        }
    }
}

void VoxelEditPacketSender::queueVoxelEditMessages(PACKET_TYPE type, int numberOfDetails, VoxelDetail* details) {
    if (!_shouldSend) {
        return; // bail early
    }

    for (int i = 0; i < numberOfDetails; i++) {
        static unsigned char bufferOut[MAX_PACKET_SIZE];
        int sizeOut = 0;
        
        if (encodeVoxelEditMessageDetails(type, 1, &details[i], &bufferOut[0], MAX_PACKET_SIZE, sizeOut)) {
            queueVoxelEditMessage(type, bufferOut, sizeOut);
        }
    }    
}

void VoxelEditPacketSender::processPreServerExistsPackets() {
    assert(voxelServersExist()); // we should only be here if we have jurisdictions
    
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
        queueVoxelEditMessage(packet->_currentType, &packet->_currentBuffer[0], packet->_currentSize);
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

void VoxelEditPacketSender::queuePacketToNodes(unsigned char* buffer, ssize_t length) {
    if (!_shouldSend) {
        return; // bail early
    }
    
    assert(voxelServersExist()); // we must have jurisdictions to be here!!

    int headerBytes = numBytesForPacketHeader(buffer) + sizeof(short);
    unsigned char* octCode = buffer + headerBytes; // skip the packet header to get to the octcode
    
    // We want to filter out edit messages for voxel servers based on the server's Jurisdiction
    // But we can't really do that with a packed message, since each edit message could be destined 
    // for a different voxel server... So we need to actually manage multiple queued packets... one
    // for each voxel server
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        // only send to the NodeTypes that are NODE_TYPE_VOXEL_SERVER
        if (node->getActiveSocket() != NULL && node->getType() == NODE_TYPE_VOXEL_SERVER) {
            QUuid nodeUUID = node->getUUID();
            bool isMyJurisdiction = true;
            // we need to get the jurisdiction for this 
            // here we need to get the "pending packet" for this server
            const JurisdictionMap& map = (*_voxelServerJurisdictions)[nodeUUID];
            isMyJurisdiction = (map.isMyJurisdiction(octCode, CHECK_NODE_ONLY) == JurisdictionMap::WITHIN);
            if (isMyJurisdiction) {
                queuePacketToNode(nodeUUID, buffer, length);
            }
        }
    }
}


// NOTE: codeColorBuffer - is JUST the octcode/color and does not contain the packet header!
void VoxelEditPacketSender::queueVoxelEditMessage(PACKET_TYPE type, unsigned char* codeColorBuffer, ssize_t length) {
    if (!_shouldSend) {
        return; // bail early
    }
    
    // If we don't have voxel jurisdictions, then we will simply queue up all of these packets and wait till we have
    // jurisdictions for processing
    if (!voxelServersExist()) {
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
    
    // We want to filter out edit messages for voxel servers based on the server's Jurisdiction
    // But we can't really do that with a packed message, since each edit message could be destined 
    // for a different voxel server... So we need to actually manage multiple queued packets... one
    // for each voxel server
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        // only send to the NodeTypes that are NODE_TYPE_VOXEL_SERVER
        if (node->getActiveSocket() != NULL && node->getType() == NODE_TYPE_VOXEL_SERVER) {
            QUuid nodeUUID = node->getUUID();
            bool isMyJurisdiction = true;

            if (_voxelServerJurisdictions) {
                // we need to get the jurisdiction for this 
                // here we need to get the "pending packet" for this server
                const JurisdictionMap& map = (*_voxelServerJurisdictions)[nodeUUID];
                isMyJurisdiction = (map.isMyJurisdiction(codeColorBuffer, CHECK_NODE_ONLY) == JurisdictionMap::WITHIN);
            }
            if (isMyJurisdiction) {
                EditPacketBuffer& packetBuffer = _pendingEditPackets[nodeUUID];
                packetBuffer._nodeUUID = nodeUUID;
            
                // If we're switching type, then we send the last one and start over
                if ((type != packetBuffer._currentType && packetBuffer._currentSize > 0) || 
                    (packetBuffer._currentSize + length >= MAX_PACKET_SIZE)) {
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

void VoxelEditPacketSender::releaseQueuedMessages() {
    // if we don't yet have jurisdictions then we can't actually release messages yet because we don't 
    // know where to send them to. Instead, just remember this request and when we eventually get jurisdictions
    // call release again at that time.
    if (!voxelServersExist()) {
        _releaseQueuedMessagesPending = true;
    } else {
        for (std::map<QUuid, EditPacketBuffer>::iterator i = _pendingEditPackets.begin(); i != _pendingEditPackets.end(); i++) {
            releaseQueuedPacket(i->second);
        }
    }
}

void VoxelEditPacketSender::releaseQueuedPacket(EditPacketBuffer& packetBuffer) {
    queuePacketToNode(packetBuffer._nodeUUID, &packetBuffer._currentBuffer[0], packetBuffer._currentSize);
    packetBuffer._currentSize = 0;
    packetBuffer._currentType = PACKET_TYPE_UNKNOWN;
}

void VoxelEditPacketSender::initializePacket(EditPacketBuffer& packetBuffer, PACKET_TYPE type) {
    packetBuffer._currentSize = populateTypeAndVersion(&packetBuffer._currentBuffer[0], type);
    unsigned short int* sequenceAt = (unsigned short int*)&packetBuffer._currentBuffer[packetBuffer._currentSize];
    *sequenceAt = 0;
    packetBuffer._currentSize += sizeof(unsigned short int); // set to command + sequence
    packetBuffer._currentType = type;
}

bool VoxelEditPacketSender::process() {
    // if we have server jurisdiction details, and we have pending pre-jurisdiction packets, then process those
    // before doing our normal process step. This processPreJurisdictionPackets()
    if (voxelServersExist() && (!_preServerPackets.empty() || !_preServerSingleMessagePackets.empty() )) {
        processPreServerExistsPackets();
    }

    // base class does most of the work.
    return PacketSender::process();
}
