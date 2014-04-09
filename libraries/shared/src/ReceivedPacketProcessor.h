//
//  ReceivedPacketProcessor.h
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __shared__ReceivedPacketProcessor__
#define __shared__ReceivedPacketProcessor__

#include <QWaitCondition>

#include "GenericThread.h"
#include "NetworkPacket.h"

/// Generalized threaded processor for handling received inbound packets. 
class ReceivedPacketProcessor : public GenericThread {
    Q_OBJECT
public:
    ReceivedPacketProcessor() { }

    /// Add packet from network receive thread to the processing queue.
    /// \param sockaddr& senderAddress the address of the sender
    /// \param packetData pointer to received data
    /// \param ssize_t packetLength size of received data
    /// \thread network receive thread
    void queueReceivedPacket(const SharedNodePointer& destinationNode, const QByteArray& packet);

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
    virtual void processPacket(const SharedNodePointer& sendingNode, const QByteArray& packet) = 0;

    /// Implements generic processing behavior for this thread.
    virtual bool process();

    virtual void terminating();

private:

    std::vector<NetworkPacket> _packets;
    QWaitCondition _hasPackets;
    QMutex _waitingOnPacketsMutex;
};

#endif // __shared__PacketReceiver__
