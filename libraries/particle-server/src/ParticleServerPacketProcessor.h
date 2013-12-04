//
//  ParticleServerPacketProcessor.h
//  particle-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded network packet processor for the particle-server
//

#ifndef __particle_server__ParticleServerPacketProcessor__
#define __particle_server__ParticleServerPacketProcessor__

#include <map>

#include <ReceivedPacketProcessor.h>
class ParticleServer;

class SingleSenderStats {
public:
    SingleSenderStats();

    uint64_t getAverageTransitTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalTransitTime / _totalPackets; }
    uint64_t getAverageProcessTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalProcessTime / _totalPackets; }
    uint64_t getAverageLockWaitTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalLockWaitTime / _totalPackets; }
    uint64_t getTotalParticlesProcessed() const { return _totalParticlesInPacket; }
    uint64_t getTotalPacketsProcessed() const { return _totalPackets; }
    uint64_t getAverageProcessTimePerParticle() const 
                { return _totalParticlesInPacket == 0 ? 0 : _totalProcessTime / _totalParticlesInPacket; }
    uint64_t getAverageLockWaitTimePerParticle() const 
                { return _totalParticlesInPacket == 0 ? 0 : _totalLockWaitTime / _totalParticlesInPacket; }
        
    uint64_t _totalTransitTime; 
    uint64_t _totalProcessTime;
    uint64_t _totalLockWaitTime;
    uint64_t _totalParticlesInPacket;
    uint64_t _totalPackets;
};

typedef std::map<QUuid, SingleSenderStats> NodeToSenderStatsMap;
typedef std::map<QUuid, SingleSenderStats>::iterator NodeToSenderStatsMapIterator;


/// Handles processing of incoming network packets for the particle-server. As with other ReceivedPacketProcessor classes 
/// the user is responsible for reading inbound packets and adding them to the processing queue by calling queueReceivedPacket()
class ParticleServerPacketProcessor : public ReceivedPacketProcessor {

public:
    ParticleServerPacketProcessor(ParticleServer* myServer);

    uint64_t getAverageTransitTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalTransitTime / _totalPackets; }
    uint64_t getAverageProcessTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalProcessTime / _totalPackets; }
    uint64_t getAverageLockWaitTimePerPacket() const { return _totalPackets == 0 ? 0 : _totalLockWaitTime / _totalPackets; }
    uint64_t getTotalParticlesProcessed() const { return _totalParticlesInPacket; }
    uint64_t getTotalPacketsProcessed() const { return _totalPackets; }
    uint64_t getAverageProcessTimePerParticle() const 
                { return _totalParticlesInPacket == 0 ? 0 : _totalProcessTime / _totalParticlesInPacket; }
    uint64_t getAverageLockWaitTimePerParticle() const 
                { return _totalParticlesInPacket == 0 ? 0 : _totalLockWaitTime / _totalParticlesInPacket; }

    void resetStats();

    NodeToSenderStatsMap& getSingleSenderStats() { return _singleSenderStats; }

protected:
    virtual void processPacket(sockaddr& senderAddress, unsigned char*  packetData, ssize_t packetLength);

private:
    void trackInboundPackets(const QUuid& nodeUUID, int sequence, uint64_t transitTime, 
            int particlesInPacket, uint64_t processTime, uint64_t lockWaitTime);

    ParticleServer* _myServer;
    int _receivedPacketCount;
    
    uint64_t _totalTransitTime; 
    uint64_t _totalProcessTime;
    uint64_t _totalLockWaitTime;
    uint64_t _totalParticlesInPacket;
    uint64_t _totalPackets;
    
    NodeToSenderStatsMap _singleSenderStats;
};
#endif // __particle_server__ParticleServerPacketProcessor__
