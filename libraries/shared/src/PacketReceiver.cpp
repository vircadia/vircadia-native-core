//
//  PacketReceiver.cpp
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded packet receiver.
//

#include "PacketReceiver.h"

PacketReceiver::PacketReceiver() :
    _stopThread(false),
    _isThreaded(false) // assume non-threaded, must call initialize()
{
}

PacketReceiver::~PacketReceiver() {
    terminate();
}

void PacketReceiver::initialize(bool isThreaded) {
    _isThreaded = isThreaded;
    if (_isThreaded) {
        pthread_create(&_thread, NULL, PacketReceiverThreadEntry, this);
    }
}

void PacketReceiver::terminate() {
    if (_isThreaded) {
        _stopThread = true;
        pthread_join(_thread, NULL); 
        _isThreaded = false;
    }
}

void PacketReceiver::queuePacket(sockaddr& senderAddress, unsigned char* packetData, ssize_t packetLength) {
    _packets.push_back(NetworkPacket(senderAddress, packetData, packetLength));
}

void PacketReceiver::processPacket(sockaddr& senderAddress, unsigned char* packetData, ssize_t packetLength) {
    // Default implementation does nothing... packet is discarded
}

void* PacketReceiver::threadRoutine() {
    while (!_stopThread) {
        while (_packets.size() > 0) {
            NetworkPacket& packet = _packets.front();
            processPacket(packet.getSenderAddress(), packet.getData(), packet.getLength());
            _packets.erase(_packets.begin());
        }
        
        // In non-threaded mode, this will break each time you call it so it's the 
        // callers responsibility to continuously call this method
        if (!_isThreaded) {
            break;
        }
    }
    
    if (_isThreaded) {
        pthread_exit(0); 
    }
    return NULL; 
}

extern "C" void* PacketReceiverThreadEntry(void* arg) {
    PacketReceiver* packetReceiver = (PacketReceiver*)arg;
    return packetReceiver->threadRoutine();
}