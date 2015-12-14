//
//  JurisdictionSender.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_JurisdictionSender_h
#define hifi_JurisdictionSender_h

#include <queue>
#include <QMutex>

#include <PacketSender.h>
#include <ReceivedPacketProcessor.h>
#include "JurisdictionMap.h"

/// Will process PacketType::_JURISDICTION_REQUEST packets and send out PacketType::_JURISDICTION packets
/// to requesting parties. As with other ReceivedPacketProcessor classes the user is responsible for reading inbound packets
/// and adding them to the processing queue by calling queueReceivedPacket()
class JurisdictionSender : public ReceivedPacketProcessor {
    Q_OBJECT
public:
    static const int DEFAULT_PACKETS_PER_SECOND = 1;

    JurisdictionSender(JurisdictionMap* map, NodeType_t type = NodeType::EntityServer);
    ~JurisdictionSender();

    void setJurisdiction(JurisdictionMap* map) { _jurisdictionMap = map; }

    virtual bool process() override;

    NodeType_t getNodeType() const { return _nodeType; }
    void setNodeType(NodeType_t type) { _nodeType = type; }

protected:
    virtual void processPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) override;

    /// Locks all the resources of the thread.
    void lockRequestingNodes() { _requestingNodeMutex.lock(); }

    /// Unlocks all the resources of the thread.
    void unlockRequestingNodes() { _requestingNodeMutex.unlock(); }


private:
    QMutex _requestingNodeMutex;
    JurisdictionMap* _jurisdictionMap;
    std::queue<QUuid> _nodesRequestingJurisdictions;
    NodeType_t _nodeType;
    
    PacketSender _packetSender;
};
#endif // hifi_JurisdictionSender_h
