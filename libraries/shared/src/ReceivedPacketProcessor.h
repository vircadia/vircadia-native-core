//
//  ReceivedPacketProcessor.h
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded received packet processor.
//

#ifndef __shared__ReceivedPacketProcessor__
#define __shared__ReceivedPacketProcessor__

#include "GenericThread.h"
#include "NetworkPacket.h"

/// Generalized threaded processor for handling received inbound packets. 
class ReceivedPacketProcessor : public virtual GenericThread {
public:

    /// Add packet from network receive thread to the processing queue.
    /// \param sockaddr& senderAddress the address of the sender
    /// \param packetData pointer to received data
    /// \param ssize_t packetLength size of received data
    /// \thread network receive thread
    void queueReceivedPacket(sockaddr& senderAddress, unsigned char*  packetData, ssize_t packetLength);

    /// Are there received packets waiting to be processed
    bool hasPacketsToProcess() const { return _packets.size() > 0; }

    /// How many received packets waiting are to be processed
    int packetsToProcessCount() const { return _packets.size(); }
    
protected:
    /// Callback for processing of recieved packets. Implement this to process the incoming packets.
    /// \param sockaddr& senderAddress the address of the sender
    /// \param packetData pointer to received data
    /// \param ssize_t packetLength size of received data
    /// \thread "this" individual processing thread
    virtual void processPacket(sockaddr& senderAddress, unsigned char*  packetData, ssize_t packetLength) = 0;

    /// Implements generic processing behavior for this thread.
    virtual bool process();

private:

    std::vector<NetworkPacket> _packets;
};

#endif // __shared__PacketReceiver__
