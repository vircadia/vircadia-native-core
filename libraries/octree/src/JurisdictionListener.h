//
//  JurisdictionListener.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Voxel Packet Sender
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_JurisdictionListener_h
#define hifi_JurisdictionListener_h

#include <NodeList.h>
#include <PacketSender.h>
#include <ReceivedPacketProcessor.h>

#include "JurisdictionMap.h"

/// Sends out PacketType::_JURISDICTION_REQUEST packets to all voxel servers and then listens for and processes
/// the PacketType::_JURISDICTION packets it receives in order to maintain an accurate state of all jurisidictions
/// within the domain. As with other ReceivedPacketProcessor classes the user is responsible for reading inbound packets
/// and adding them to the processing queue by calling queueReceivedPacket()
class JurisdictionListener : public ReceivedPacketProcessor {
    Q_OBJECT
public:
    static const int DEFAULT_PACKETS_PER_SECOND = 1;
    static const int NO_SERVER_CHECK_RATE = 60; // if no servers yet detected, keep checking at 60fps

    JurisdictionListener(NodeType_t type = NodeType::EntityServer);
    
    virtual bool process() override;

    NodeToJurisdictionMap* getJurisdictions() { return &_jurisdictions; }


    NodeType_t getNodeType() const { return _nodeType; }
    void setNodeType(NodeType_t type) { _nodeType = type; }

public slots:
    /// Called by NodeList to inform us that a node has been killed.
    void nodeKilled(SharedNodePointer node);
    
protected:
    /// Callback for processing of received packets. Will process any queued PacketType::_JURISDICTION and update the
    /// jurisdiction map member variable
    virtual void processPacket(QSharedPointer<ReceivedMessage> messsage, SharedNodePointer sendingNode) override;

private:
    NodeToJurisdictionMap _jurisdictions;
    NodeType_t _nodeType;

    bool queueJurisdictionRequest();

    PacketSender _packetSender;
};
#endif // hifi_JurisdictionListener_h
