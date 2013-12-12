//
//  ParticleScriptingInterface.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__ParticleScriptingInterface__
#define __hifi__ParticleScriptingInterface__

#include <QtCore/QObject>

#include <JurisdictionListener.h>
#include "ParticleEditPacketSender.h"

/// handles scripting of Particle commands from JS passed to assigned clients
class ParticleScriptingInterface : public QObject {
    Q_OBJECT
public:
    ParticleScriptingInterface();
    
    ParticleEditPacketSender* getParticlePacketSender() { return &_particlePacketSender; }
    JurisdictionListener* getJurisdictionListener() { return &_jurisdictionListener; }
public slots:
    /// queues the creation of a Particle which will be sent by calling process on the PacketSender
    /// returns the creatorTokenID for the newly created particle
    uint32_t queueParticleAdd(glm::vec3 position, float radius, 
            xColor color, glm::vec3 velocity, glm::vec3 gravity, float damping, QString updateScript);
    
    /// Set the desired max packet size in bytes that should be created
    void setMaxPacketSize(int maxPacketSize) { return _particlePacketSender.setMaxPacketSize(maxPacketSize); }

    /// returns the current desired max packet size in bytes that will be created
    int getMaxPacketSize() const { return _particlePacketSender.getMaxPacketSize(); }

    /// set the max packets per second send rate
    void setPacketsPerSecond(int packetsPerSecond) { return _particlePacketSender.setPacketsPerSecond(packetsPerSecond); }

    /// get the max packets per second send rate
    int getPacketsPerSecond() const  { return _particlePacketSender.getPacketsPerSecond(); }

    /// does a particle server exist to send to
    bool serversExist() const { return _particlePacketSender.serversExist(); }

    /// are there packets waiting in the send queue to be sent
    bool hasPacketsToSend() const { return _particlePacketSender.hasPacketsToSend(); }

    /// how many packets are there in the send queue waiting to be sent
    int packetsToSendCount() const { return _particlePacketSender.packetsToSendCount(); }

    /// returns the packets per second send rate of this object over its lifetime
    float getLifetimePPS() const { return _particlePacketSender.getLifetimePPS(); }

    /// returns the bytes per second send rate of this object over its lifetime
    float getLifetimeBPS() const { return _particlePacketSender.getLifetimeBPS(); }
    
    /// returns the packets per second queued rate of this object over its lifetime
    float getLifetimePPSQueued() const  { return _particlePacketSender.getLifetimePPSQueued(); }

    /// returns the bytes per second queued rate of this object over its lifetime
    float getLifetimeBPSQueued() const { return _particlePacketSender.getLifetimeBPSQueued(); }

    /// returns lifetime of this object from first packet sent to now in usecs
    long long unsigned int getLifetimeInUsecs() const { return _particlePacketSender.getLifetimeInUsecs(); }

    /// returns lifetime of this object from first packet sent to now in usecs
    float getLifetimeInSeconds() const { return _particlePacketSender.getLifetimeInSeconds(); }

    /// returns the total packets sent by this object over its lifetime
    long long unsigned int getLifetimePacketsSent() const { return _particlePacketSender.getLifetimePacketsSent(); }

    /// returns the total bytes sent by this object over its lifetime
    long long unsigned int getLifetimeBytesSent() const { return _particlePacketSender.getLifetimeBytesSent(); }

    /// returns the total packets queued by this object over its lifetime
    long long unsigned int getLifetimePacketsQueued() const { return _particlePacketSender.getLifetimePacketsQueued(); }

    /// returns the total bytes queued by this object over its lifetime
    long long unsigned int getLifetimeBytesQueued() const { return _particlePacketSender.getLifetimeBytesQueued(); }

private:
    /// attached ParticleEditPacketSender that handles queuing and sending of packets to VS
    ParticleEditPacketSender _particlePacketSender;
    JurisdictionListener _jurisdictionListener;
    
    void queueParticleAdd(PACKET_TYPE addPacketType, ParticleDetail& addParticleDetails);
    
    uint32_t _nextCreatorTokenID;
};

#endif /* defined(__hifi__ParticleScriptingInterface__) */
