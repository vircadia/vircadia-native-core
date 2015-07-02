//
//  OctreeEditPacketSender.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeEditPacketSender_h
#define hifi_OctreeEditPacketSender_h

#include <qqueue.h>
#include <PacketSender.h>
#include <PacketHeaders.h>

#include "EditPacketBuffer.h"
#include "JurisdictionMap.h"
#include "SentPacketHistory.h"

/// Utility for processing, packing, queueing and sending of outbound edit messages.
class OctreeEditPacketSender :  public PacketSender {
    Q_OBJECT
public:
    OctreeEditPacketSender();
    ~OctreeEditPacketSender();
    
    /// Queues a single edit message. Will potentially send a pending multi-command packet. Determines which server
    /// node or nodes the packet should be sent to. Can be called even before servers are known, in which case up to 
    /// MaxPendingMessages will be buffered and processed when servers are known.
    void queueOctreeEditMessage (PacketType::Value type, unsigned char* buffer, size_t length, qint64 satoshiCost = 0);

    /// Releases all queued messages even if those messages haven't filled an MTU packet. This will move the packed message 
    /// packets onto the send queue. If running in threaded mode, the caller does not need to do any further processing to
    /// have these packets get sent. If running in non-threaded mode, the caller must still call process() on a regular
    /// interval to ensure that the packets are actually sent. Can be called even before servers are known, in 
    /// which case  up to MaxPendingMessages of the released messages will be buffered and actually released when 
    /// servers are known.
    void releaseQueuedMessages();

    /// are we in sending mode. If we're not in sending mode then all packets and messages will be ignored and
    /// not queued and not sent
    bool getShouldSend() const { return _shouldSend; }

    /// set sending mode. By default we are set to shouldSend=TRUE and packets will be sent. If shouldSend=FALSE, then we'll
    /// switch to not sending mode, and all packets and messages will be ignored, not queued, and not sent. This might be used
    /// in an application like interface when all octree features are disabled.
    void setShouldSend(bool shouldSend) { _shouldSend = shouldSend; }

    /// call this to inform the OctreeEditPacketSender of the server jurisdictions. This is required for normal operation.
    /// The internal contents of the jurisdiction map may change throughout the lifetime of the OctreeEditPacketSender. This map
    /// can be set prior to servers being present, so long as the contents of the map accurately reflect the current
    /// known jurisdictions.
    void setServerJurisdictions(NodeToJurisdictionMap* serverJurisdictions) { 
        _serverJurisdictions = serverJurisdictions;
    }

    /// if you're running in non-threaded mode, you must call this method regularly
    virtual bool process();

    /// Set the desired number of pending messages that the OctreeEditPacketSender should attempt to queue even if
    /// servers are not present. This only applies to how the OctreeEditPacketSender will manage messages when no 
    /// servers are present. By default, this value is the same as the default packets that will be sent in one second.
    /// Which means the OctreeEditPacketSender will not buffer all messages given to it if no servers are present. 
    /// This is the maximum number of queued messages and single messages.
    void setMaxPendingMessages(int maxPendingMessages) { _maxPendingMessages = maxPendingMessages; }

    // the default number of pending messages we will store if no servers are available
    static const int DEFAULT_MAX_PENDING_MESSAGES;

    // is there an octree server available to send packets to    
    bool serversExist() const;

    /// Set the desired max packet size in bytes that the OctreeEditPacketSender should create
    void setMaxPacketSize(int maxPacketSize) { _maxPacketSize = maxPacketSize; }

    /// returns the current desired max packet size in bytes that the OctreeEditPacketSender will create
    int getMaxPacketSize() const { return _maxPacketSize; }

    // you must override these...
    virtual char getMyNodeType() const = 0;
    virtual void adjustEditPacketForClockSkew (PacketType::Value type, 
                        unsigned char* editPacketBuffer, size_t length, int clockSkew) { }
    
    bool hasDestinationWalletUUID() const { return !_destinationWalletUUID.isNull(); }
    void setDestinationWalletUUID(const QUuid& destinationWalletUUID) { _destinationWalletUUID = destinationWalletUUID; }
    const QUuid& getDestinationWalletUUID() { return _destinationWalletUUID; }
    
    void processNackPacket(const QByteArray& packet);

public slots:
    void nodeKilled(SharedNodePointer node);

signals:
    void octreePaymentRequired(qint64 satoshiAmount, const QUuid& nodeUUID, const QUuid& destinationWalletUUID);
    
protected:
    bool _shouldSend;
    void queuePacketToNode(const QUuid& nodeID, unsigned char* buffer, size_t length, qint64 satoshiCost = 0);
    void queuePendingPacketToNodes (PacketType::Value type, unsigned char* buffer, size_t length, qint64 satoshiCost = 0);
    void queuePacketToNodes(unsigned char* buffer, size_t length, qint64 satoshiCost = 0);
    void initializePacket(EditPacketBuffer& packetBuffer, PacketType::Value type, int nodeClockSkew);
    void releaseQueuedPacket(EditPacketBuffer& packetBuffer); // releases specific queued packet
    
    void processPreServerExistsPackets();

    // These are packets which are destined from know servers but haven't been released because they're still too small
    QHash<QUuid, EditPacketBuffer> _pendingEditPackets;
    
    // These are packets that are waiting to be processed because we don't yet know if there are servers
    int _maxPendingMessages;
    bool _releaseQueuedMessagesPending;
    QMutex _pendingPacketsLock;
    QMutex _packetsQueueLock; // don't let different threads release the queue while another thread is writing to it
    QVector<EditPacketBuffer*> _preServerPackets; // these will get packed into other larger packets
    QVector<EditPacketBuffer*> _preServerSingleMessagePackets; // these will go out as is

    NodeToJurisdictionMap* _serverJurisdictions;
    
    int _maxPacketSize;

    QMutex _releaseQueuedPacketMutex;

    // TODO: add locks for this and _pendingEditPackets
    QHash<QUuid, SentPacketHistory> _sentPacketHistories;
    QHash<QUuid, quint16> _outgoingSequenceNumbers;
    
    QUuid _destinationWalletUUID;
};
#endif // hifi_OctreeEditPacketSender_h
