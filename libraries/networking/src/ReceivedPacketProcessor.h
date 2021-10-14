//
//  ReceivedPacketProcessor.h
//  libraries/networking/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ReceivedPacketProcessor_h
#define hifi_ReceivedPacketProcessor_h

#include <QtCore/QSharedPointer>
#include <QWaitCondition>

#include "NodeList.h"

#include "GenericThread.h"

class ReceivedMessage;

/// Generalized threaded processor for handling received inbound packets.
class ReceivedPacketProcessor : public GenericThread {
    Q_OBJECT
public:
    static const uint64_t MAX_WAIT_TIME { 100 }; // Max wait time in ms

    ReceivedPacketProcessor();

    /// Add packet from network receive thread to the processing queue.
    void queueReceivedPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode);

    /// Are there received packets waiting to be processed
    bool hasPacketsToProcess() const { return _packets.size() > 0; }

    /// Is a specified node still alive?
    bool isAlive(const QUuid& nodeUUID) const {
        return _nodePacketCounts.contains(nodeUUID);
    }

    /// Are there received packets waiting to be processed from a specified node
    bool hasPacketsToProcessFrom(const SharedNodePointer& sendingNode) const {
        return hasPacketsToProcessFrom(sendingNode->getUUID());
    }

    /// Are there received packets waiting to be processed from a specified node
    bool hasPacketsToProcessFrom(const QUuid& nodeUUID) const {
        return _nodePacketCounts[nodeUUID] > 0;
    }

    /// How many received packets waiting are to be processed
    int packetsToProcessCount() const { return (int)_packets.size(); }

    float getIncomingPPS() const { return _incomingPPS.getAverage(); }
    float getProcessedPPS() const { return _processedPPS.getAverage(); }

    virtual void terminating() override;

public slots:
    void nodeKilled(SharedNodePointer node);

protected:
    /// Callback for processing of recieved packets. Implement this to process the incoming packets.
    /// \param SharedNodePointer& sendingNode the node that sent this packet
    /// \param QByteArray& the packet to be processed
    virtual void processPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) = 0;

    /// Implements generic processing behavior for this thread.
    virtual bool process() override;

    /// Determines the timeout of the wait when there are no packets to process. Default value is 100ms to allow for regular event processing.
    virtual uint32_t getMaxWait() const { return MAX_WAIT_TIME; }

    /// Override to do work before the packets processing loop. Default does nothing.
    virtual void preProcess() { }

    /// Override to do work inside the packet processing loop after a packet is processed. Default does nothing.
    virtual void midProcess() { }

    /// Override to do work after the packets processing loop.  Default does nothing.
    virtual void postProcess() { }

protected:
    std::list<NodeSharedReceivedMessagePair> _packets;
    QHash<QUuid, int> _nodePacketCounts;

    QWaitCondition _hasPackets;
    QMutex _waitingOnPacketsMutex;

    quint64 _lastWindowAt = 0;
    int _lastWindowIncomingPackets = 0;
    int _lastWindowProcessedPackets = 0;
    SimpleMovingAverage _incomingPPS;
    SimpleMovingAverage _processedPPS;
};

#endif // hifi_ReceivedPacketProcessor_h
