//
//  VoxelEditPacketSender.cpp
//  interface
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded voxel packet Sender for the Application
//

#include <PerfStat.h>

#include "Application.h"
#include "VoxelEditPacketSender.h"

VoxelEditPacketSender::VoxelEditPacketSender(Application* app) :
    _app(app)
{
}

void VoxelEditPacketSender::sendVoxelEditMessage(PACKET_TYPE type, VoxelDetail& detail) {

    // if the app has Voxels disabled, we don't do any of this...
    if (!_app->_renderVoxels->isChecked()) {
        return; // bail early
    }

    unsigned char* bufferOut;
    int sizeOut;
    int totalBytesSent = 0;

    if (createVoxelEditMessage(type, 0, 1, &detail, bufferOut, sizeOut)){
        actuallySendMessage(UNKNOWN_NODE_ID, bufferOut, sizeOut); // sends to all servers... not ideal!
        delete[] bufferOut;
    }

    // Tell the application's bandwidth meters about what we've sent
    _app->_bandwidthMeter.outputStream(BandwidthMeter::VOXELS).updateValue(totalBytesSent); 
}

void VoxelEditPacketSender::actuallySendMessage(uint16_t nodeID, unsigned char* bufferOut, ssize_t sizeOut) {
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        // only send to the NodeTypes that are NODE_TYPE_VOXEL_SERVER
        if (node->getActiveSocket() != NULL && node->getType() == NODE_TYPE_VOXEL_SERVER && 
            ((node->getNodeID() == nodeID) || (nodeID == (uint16_t)UNKNOWN_NODE_ID))  ) {
            sockaddr* nodeAddress = node->getActiveSocket();
            queuePacket(*nodeAddress, bufferOut, sizeOut);
        }
    }
}

void VoxelEditPacketSender::queueVoxelEditMessage(PACKET_TYPE type, unsigned char* codeColorBuffer, ssize_t length) {
    // We want to filter out edit messages for voxel servers based on the server's Jurisdiction
    // But we can't really do that with a packed message, since each edit message could be destined 
    // for a different voxel server... So we need to actually manage multiple queued packets... one
    // for each voxel server
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        // only send to the NodeTypes that are NODE_TYPE_VOXEL_SERVER
        if (node->getActiveSocket() != NULL && node->getType() == NODE_TYPE_VOXEL_SERVER) {
        
            // we need to get the jurisdiction for this 
            // here we need to get the "pending packet" for this server
            uint16_t nodeID = node->getNodeID();
            const JurisdictionMap& map = _app->_voxelServerJurisdictions[nodeID];
            if (map.isMyJurisdiction(codeColorBuffer, CHECK_NODE_ONLY) == JurisdictionMap::WITHIN) {
                EditPacketBuffer& packetBuffer = _pendingEditPackets[nodeID];
                packetBuffer._nodeID = nodeID;
            
                // If we're switching type, then we send the last one and start over
                if ((type != packetBuffer._currentType && packetBuffer._currentSize > 0) || 
                    (packetBuffer._currentSize + length >= MAX_PACKET_SIZE)) {
                    flushQueue(packetBuffer);
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

void VoxelEditPacketSender::flushQueue() {
    for (std::map<uint16_t,EditPacketBuffer>::iterator i = _pendingEditPackets.begin(); i != _pendingEditPackets.end(); i++) {
        flushQueue(i->second);
    }
}

void VoxelEditPacketSender::flushQueue(EditPacketBuffer& packetBuffer) {
    actuallySendMessage(packetBuffer._nodeID, &packetBuffer._currentBuffer[0], packetBuffer._currentSize);
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
