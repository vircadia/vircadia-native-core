//
//  OctreeScriptingInterface.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeScriptingInterface_h
#define hifi_OctreeScriptingInterface_h

#include <QtCore/QObject>

#include "OctreeEditPacketSender.h"

/// handles scripting of Particle commands from JS passed to assigned clients
class OctreeScriptingInterface : public QObject {
    Q_OBJECT
public:
    OctreeScriptingInterface(OctreeEditPacketSender* packetSender = nullptr);

    ~OctreeScriptingInterface();

    OctreeEditPacketSender* getPacketSender() const { return _packetSender; }
    void setPacketSender(OctreeEditPacketSender* packetSender);
    void init();

    virtual NodeType_t getServerNodeType() const = 0;
    virtual OctreeEditPacketSender* createPacketSender() = 0;

private slots:
    void cleanupManagedObjects();

public slots:

    /*@jsdoc
     * Sets the maximum number of entity packets that the client can send per second.
     * @function Entities.setPacketsPerSecond
     * @param {number} packetsPerSecond - Integer maximum number of entity packets that the client can send per second.
     */
    /// set the max packets per second send rate
    void setPacketsPerSecond(int packetsPerSecond) { return _packetSender->setPacketsPerSecond(packetsPerSecond); }

    /*@jsdoc
     * Gets the maximum number of entity packets that the client can send per second.
     * @function Entities.getPacketsPerSecond
     * @returns {number} Integer maximum number of entity packets that the client can send per second.
     */
    /// get the max packets per second send rate
    int getPacketsPerSecond() const  { return _packetSender->getPacketsPerSecond(); }

    /*@jsdoc
     * Checks whether servers exist for the client to send entity packets to, i.e., whether you are connected to a domain and
     * its entity server is working.
     * @function Entities.serversExist
     * @returns {boolean} <code>true</code> if servers exist for the client to send entity packets to, otherwise 
     *     <code>false</code>.
     */
    /// does a server exist to send to
    bool serversExist() const { return _packetSender->serversExist(); }

    /*@jsdoc
     * Checks whether the client has entity packets waiting to be sent.
     * @function Entities.hasPacketsToSend
     * @returns {boolean} <code>true</code> if the client has entity packets waiting to be sent, otherwise <code>false</code>.
     */
    /// are there packets waiting in the send queue to be sent
    bool hasPacketsToSend() const { return _packetSender->hasPacketsToSend(); }

    /*@jsdoc
     * Gets the number of entity packets the client has waiting to be sent.
     * @function Entities.packetsToSendCount
     * @returns {number} Integer number of entity packets the client has waiting to be sent.
     */
    /// how many packets are there in the send queue waiting to be sent
    int packetsToSendCount() const { return (int)_packetSender->packetsToSendCount(); }

    /*@jsdoc
     * Gets the entity packets per second send rate of the client over its lifetime.
     * @function Entities.getLifetimePPS
     * @returns {number} Entity packets per second send rate of the client over its lifetime.
     */
    /// returns the packets per second send rate of this object over its lifetime
    float getLifetimePPS() const { return _packetSender->getLifetimePPS(); }

    /*@jsdoc
     * Gets the entity bytes per second send rate of the client over its lifetime.
     * @function Entities.getLifetimeBPS
     * @returns {number} Entity bytes per second send rate of the client over its lifetime.
     */
    /// returns the bytes per second send rate of this object over its lifetime
    float getLifetimeBPS() const { return _packetSender->getLifetimeBPS(); }

    /*@jsdoc
     * Gets the entity packets per second queued rate of the client over its lifetime.
     * @function Entities.getLifetimePPSQueued
     * @returns {number} Entity packets per second queued rate of the client over its lifetime.
     */
    /// returns the packets per second queued rate of this object over its lifetime
    float getLifetimePPSQueued() const  { return _packetSender->getLifetimePPSQueued(); }

    /*@jsdoc
     * Gets the entity bytes per second queued rate of the client over its lifetime.
     * @function Entities.getLifetimeBPSQueued
     * @returns {number} Entity bytes per second queued rate of the client over its lifetime.
     */
    /// returns the bytes per second queued rate of this object over its lifetime
    float getLifetimeBPSQueued() const { return _packetSender->getLifetimeBPSQueued(); }

    /*@jsdoc
     * Gets the lifetime of the client from the first entity packet sent until now, in microseconds.
     * @function Entities.getLifetimeInUsecs
     * @returns {number} Lifetime of the client from the first entity packet sent until now, in microseconds.
     */
    /// returns lifetime of this object from first packet sent to now in usecs
    long long unsigned int getLifetimeInUsecs() const { return _packetSender->getLifetimeInUsecs(); }

    /*@jsdoc
     * Gets the lifetime of the client from the first entity packet sent until now, in seconds.
     * @function Entities.getLifetimeInSeconds
     * @returns {number} Lifetime of the client from the first entity packet sent until now, in seconds.
     */
    /// returns lifetime of this object from first packet sent to now in secs
    float getLifetimeInSeconds() const { return _packetSender->getLifetimeInSeconds(); }

    /*@jsdoc
     * Gets the total number of entity packets sent by the client over its lifetime.
     * @function Entities.getLifetimePacketsSent
     * @returns {number} The total number of entity packets sent by the client over its lifetime.
     */
    /// returns the total packets sent by this object over its lifetime
    long long unsigned int getLifetimePacketsSent() const { return _packetSender->getLifetimePacketsSent(); }

    /*@jsdoc
     * Gets the total bytes of entity packets sent by the client over its lifetime.
     * @function Entities.getLifetimeBytesSent
     * @returns {number} The total bytes of entity packets sent by the client over its lifetime.
     */
    /// returns the total bytes sent by this object over its lifetime
    long long unsigned int getLifetimeBytesSent() const { return _packetSender->getLifetimeBytesSent(); }

    /*@jsdoc
     * Gets the total number of entity packets queued by the client over its lifetime.
     * @function Entities.getLifetimePacketsQueued
     * @returns {number} The total number of entity packets queued by the client over its lifetime.
     */
    /// returns the total packets queued by this object over its lifetime
    long long unsigned int getLifetimePacketsQueued() const { return _packetSender->getLifetimePacketsQueued(); }

    /*@jsdoc
     * Gets the total bytes of entity packets queued by the client over its lifetime.
     * @function Entities.getLifetimeBytesQueued
     * @returns {number} The total bytes of entity packets queued by the client over its lifetime.
     */
    /// returns the total bytes queued by this object over its lifetime
    long long unsigned int getLifetimeBytesQueued() const { return _packetSender->getLifetimeBytesQueued(); }

protected:
    /// attached OctreeEditPacketSender that handles queuing and sending of packets to VS
    OctreeEditPacketSender* _packetSender = nullptr;
    bool _managedPacketSender;
    bool _initialized;
};

#endif // hifi_OctreeScriptingInterface_h
