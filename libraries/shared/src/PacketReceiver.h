//
//  PacketReceiver.h
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded packet receiver.
//

#ifndef __shared__PacketReceiver__
#define __shared__PacketReceiver__

#include "NetworkPacket.h"

class PacketReceiver {
public:
    PacketReceiver();
    virtual ~PacketReceiver();

    // Call this when your network receive gets a packet
    void queuePacket(sockaddr& senderAddress, unsigned char*  packetData, ssize_t packetLength);
    
    // implement this to process the incoming packets
    virtual void processPacket(sockaddr& senderAddress, unsigned char*  packetData, ssize_t packetLength);

    // Call to start the thread
    void initialize(bool isThreaded);

    // Call when you're ready to stop the thread
    void terminate();
    
    // If you're running in non-threaded mode, you must call this regularly
    void* threadRoutine();
    
private:
    bool        _stopThread;
    bool        _isThreaded;
    pthread_t   _thread;
    std::vector<NetworkPacket> _packets;
};

extern "C" void* PacketReceiverThreadEntry(void* arg);

#endif // __shared__PacketReceiver__
