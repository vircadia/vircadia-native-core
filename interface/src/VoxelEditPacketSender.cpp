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
    _app(app),
    _currentType(PACKET_TYPE_UNKNOWN),
    _currentSize(0)
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
        actuallySendMessage(bufferOut, sizeOut);
        delete[] bufferOut;
    }

    // Tell the application's bandwidth meters about what we've sent
    _app->_bandwidthMeter.outputStream(BandwidthMeter::VOXELS).updateValue(totalBytesSent); 
}

void VoxelEditPacketSender::actuallySendMessage(unsigned char* bufferOut, ssize_t sizeOut) {
    qDebug("VoxelEditPacketSender::actuallySendMessage() sizeOut=%lu\n", sizeOut);
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        // only send to the NodeTypes we are asked to send to.
        if (node->getActiveSocket() != NULL && node->getType() == NODE_TYPE_VOXEL_SERVER) {
            // we know which socket is good for this node, send there
            sockaddr* nodeAddress = node->getActiveSocket();
            queuePacket(*nodeAddress, bufferOut, sizeOut);
        }
    }
}

void VoxelEditPacketSender::queueVoxelEditMessage(PACKET_TYPE type, unsigned char* codeColorBuffer, ssize_t length) {
    // If we're switching type, then we send the last one and start over
    if ((type != _currentType && _currentSize > 0) || (_currentSize + length >= MAX_PACKET_SIZE)) {
        flushQueue();
        initializePacket(type);
    }

    // If the buffer is empty and not correctly initialized for our type...
    if (type != _currentType && _currentSize == 0) {
        initializePacket(type);
    }

    memcpy(&_currentBuffer[_currentSize], codeColorBuffer, length);
    _currentSize += length;
}

void VoxelEditPacketSender::flushQueue() {
    actuallySendMessage(&_currentBuffer[0], _currentSize);
    _currentSize = 0;
    _currentType = PACKET_TYPE_UNKNOWN;
}

void VoxelEditPacketSender::initializePacket(PACKET_TYPE type) {
    _currentSize = populateTypeAndVersion(&_currentBuffer[0], type);
    unsigned short int* sequenceAt = (unsigned short int*)&_currentBuffer[_currentSize];
    *sequenceAt = 0;
    _currentSize += sizeof(unsigned short int); // set to command + sequence
    _currentType = type;
}
