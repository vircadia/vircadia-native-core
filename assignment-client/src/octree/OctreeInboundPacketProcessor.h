//
//  OctreeInboundPacketProcessor.h
//  assignment-client/src/octree
//
//  Created by Brad Hefta-Gaub on 8/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Threaded or non-threaded network packet processor for octree servers
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeInboundPacketProcessor_h
#define hifi_OctreeInboundPacketProcessor_h

#include <QtCore/QSharedPointer>

#include <ReceivedPacketProcessor.h>

#include "SequenceNumberStats.h"

class OctreeServer;

class SingleSenderStats {
public:
    SingleSenderStats();

    quint64 getAverageTransitTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalTransitTime / _totalPackets; }
    quint64 getAverageProcessTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalProcessTime / _totalPackets; }
    quint64 getAverageLockWaitTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalLockWaitTime / _totalPackets; }
    quint64 getTotalElementsProcessed() const { return _totalElementsInPacket; }
    quint64 getTotalPacketsProcessed() const { return _totalPackets; }
    quint64 getAverageProcessTimePerElement() const
                { return _totalElementsInPacket == 0 ? 0 : _totalProcessTime / _totalElementsInPacket; }
    quint64 getAverageLockWaitTimePerElement() const
                { return _totalElementsInPacket == 0 ? 0 : _totalLockWaitTime / _totalElementsInPacket; }
    
    const SequenceNumberStats& getIncomingEditSequenceNumberStats() const { return _incomingEditSequenceNumberStats; }
    SequenceNumberStats& getIncomingEditSequenceNumberStats() { return _incomingEditSequenceNumberStats; }

    void trackInboundPacket(unsigned short int incomingSequence, quint64 transitTime,
        int editsInPacket, quint64 processTime, quint64 lockWaitTime);

    quint64 _totalTransitTime;
    quint64 _totalProcessTime;
    quint64 _totalLockWaitTime;
    quint64 _totalElementsInPacket;
    quint64 _totalPackets;
    SequenceNumberStats _incomingEditSequenceNumberStats;
};

typedef QHash<QUuid, SingleSenderStats> NodeToSenderStatsMap;
typedef QHash<QUuid, SingleSenderStats>::iterator NodeToSenderStatsMapIterator;
typedef QHash<QUuid, SingleSenderStats>::const_iterator NodeToSenderStatsMapConstIterator;


/// Handles processing of incoming network packets for the octee servers. As with other ReceivedPacketProcessor classes
/// the user is responsible for reading inbound packets and adding them to the processing queue by calling queueReceivedPacket()
class OctreeInboundPacketProcessor : public ReceivedPacketProcessor {
    Q_OBJECT
public:
    OctreeInboundPacketProcessor(OctreeServer* myServer);

    quint64 getAverageTransitTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalTransitTime / _totalPackets; }
    quint64 getAverageProcessTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalProcessTime / _totalPackets; }
    quint64 getAverageLockWaitTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalLockWaitTime / _totalPackets; }
    quint64 getTotalElementsProcessed() const { return _totalElementsInPacket; }
    quint64 getTotalPacketsProcessed() const { return _totalPackets; }
    quint64 getAverageProcessTimePerElement() const
                { return _totalElementsInPacket == 0 ? 0 : _totalProcessTime / _totalElementsInPacket; }
    quint64 getAverageLockWaitTimePerElement() const
                { return _totalElementsInPacket == 0 ? 0 : _totalLockWaitTime / _totalElementsInPacket; }

    void resetStats();

    NodeToSenderStatsMap getSingleSenderStats() { QReadLocker locker(&_senderStatsLock); return _singleSenderStats; }

    virtual void terminating() override { _shuttingDown = true; ReceivedPacketProcessor::terminating(); }

protected:

    virtual void processPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) override;

    virtual uint32_t getMaxWait() const override;
    virtual void preProcess() override;
    virtual void midProcess() override;

private:
    int sendNackPackets();

private:
    void trackInboundPacket(const QUuid& nodeUUID, unsigned short int sequence, quint64 transitTime,
            int elementsInPacket, quint64 processTime, quint64 lockWaitTime);

    OctreeServer* _myServer;
    int _receivedPacketCount;
    
    std::atomic<uint64_t> _totalTransitTime;
    std::atomic<uint64_t> _totalProcessTime;
    std::atomic<uint64_t> _totalLockWaitTime;
    std::atomic<uint64_t> _totalElementsInPacket;
    std::atomic<uint64_t> _totalPackets;
    
    NodeToSenderStatsMap _singleSenderStats;
    QReadWriteLock _senderStatsLock;

    std::atomic<uint64_t> _lastNackTime;
    bool _shuttingDown;
};
#endif // hifi_OctreeInboundPacketProcessor_h
