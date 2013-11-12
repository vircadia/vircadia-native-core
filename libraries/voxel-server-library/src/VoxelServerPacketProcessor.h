//
//  VoxelServerPacketProcessor.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded network packet processor for the voxel-server
//

#ifndef __voxel_server__VoxelServerPacketProcessor__
#define __voxel_server__VoxelServerPacketProcessor__

#include <map>

#include <ReceivedPacketProcessor.h>
class VoxelServer;

class SingleSenderStats {
public:
    SingleSenderStats();

    uint64_t getAverateTransitTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalTransitTime / _totalPackets; }
    uint64_t getAverageProcessTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalProcessTime / _totalPackets; }
    uint64_t getAverageLockWaitTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalLockWaitTime / _totalPackets; }
    uint64_t getTotalVoxelsProcessed() const { return _totalVoxelsInPacket; }
    uint64_t getTotalPacketsProcessed() const { return _totalPackets; }
    uint64_t getAverageProcessTimePerVoxel() const 
                { return _totalVoxelsInPacket == 0 ? 0 : _totalProcessTime / _totalVoxelsInPacket; }
    uint64_t getAverageLockWaitTimePerVoxel() const 
                { return _totalVoxelsInPacket == 0 ? 0 : _totalLockWaitTime / _totalVoxelsInPacket; }
        
    uint64_t _totalTransitTime; 
    uint64_t _totalProcessTime;
    uint64_t _totalLockWaitTime;
    uint64_t _totalVoxelsInPacket;
    uint64_t _totalPackets;
};

typedef std::map<QUuid, SingleSenderStats> NodeToSenderStatsMap;
typedef std::map<QUuid, SingleSenderStats>::iterator NodeToSenderStatsMapIterator;


/// Handles processing of incoming network packets for the voxel-server. As with other ReceivedPacketProcessor classes 
/// the user is responsible for reading inbound packets and adding them to the processing queue by calling queueReceivedPacket()
class VoxelServerPacketProcessor : public ReceivedPacketProcessor {

public:
    VoxelServerPacketProcessor(VoxelServer* myServer);

    uint64_t getAverateTransitTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalTransitTime / _totalPackets; }
    uint64_t getAverageProcessTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalProcessTime / _totalPackets; }
    uint64_t getAverageLockWaitTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalLockWaitTime / _totalPackets; }
    uint64_t getTotalVoxelsProcessed() const { return _totalVoxelsInPacket; }
    uint64_t getTotalPacketsProcessed() const { return _totalPackets; }
    uint64_t getAverageProcessTimePerVoxel() const 
                { return _totalVoxelsInPacket == 0 ? 0 : _totalProcessTime / _totalVoxelsInPacket; }
    uint64_t getAverageLockWaitTimePerVoxel() const 
                { return _totalVoxelsInPacket == 0 ? 0 : _totalLockWaitTime / _totalVoxelsInPacket; }

    void resetStats();

    NodeToSenderStatsMap& getSingleSenderStats() { return _singleSenderStats; }

protected:
    virtual void processPacket(sockaddr& senderAddress, unsigned char*  packetData, ssize_t packetLength);

private:
    void trackInboundPackets(const QUuid& nodeUUID, int sequence, uint64_t transitTime, 
            int voxelsInPacket, uint64_t processTime, uint64_t lockWaitTime);

    VoxelServer* _myServer;
    int _receivedPacketCount;
    
    uint64_t _totalTransitTime; 
    uint64_t _totalProcessTime;
    uint64_t _totalLockWaitTime;
    uint64_t _totalVoxelsInPacket;
    uint64_t _totalPackets;
    
    NodeToSenderStatsMap _singleSenderStats;
};
#endif // __voxel_server__VoxelServerPacketProcessor__
