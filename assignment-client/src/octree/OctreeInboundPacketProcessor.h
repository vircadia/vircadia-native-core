//
//  OctreeInboundPacketProcessor.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded network packet processor for the voxel-server
//

#ifndef __octree_server__OctreeInboundPacketProcessor__
#define __octree_server__OctreeInboundPacketProcessor__

#include <map>

#include <ReceivedPacketProcessor.h>
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
        
    quint64 _totalTransitTime; 
    quint64 _totalProcessTime;
    quint64 _totalLockWaitTime;
    quint64 _totalElementsInPacket;
    quint64 _totalPackets;
};

typedef std::map<QUuid, SingleSenderStats> NodeToSenderStatsMap;
typedef std::map<QUuid, SingleSenderStats>::iterator NodeToSenderStatsMapIterator;


/// Handles processing of incoming network packets for the voxel-server. As with other ReceivedPacketProcessor classes 
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

    NodeToSenderStatsMap& getSingleSenderStats() { return _singleSenderStats; }

protected:
    virtual void processPacket(const SharedNodePointer& sendingNode, const QByteArray& packet);

private:
    void trackInboundPackets(const QUuid& nodeUUID, int sequence, quint64 transitTime, 
            int voxelsInPacket, quint64 processTime, quint64 lockWaitTime);

    OctreeServer* _myServer;
    int _receivedPacketCount;
    
    quint64 _totalTransitTime; 
    quint64 _totalProcessTime;
    quint64 _totalLockWaitTime;
    quint64 _totalElementsInPacket;
    quint64 _totalPackets;
    
    NodeToSenderStatsMap _singleSenderStats;
};
#endif // __octree_server__OctreeInboundPacketProcessor__
