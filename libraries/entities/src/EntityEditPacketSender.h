//
//  EntityEditPacketSender.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityEditPacketSender_h
#define hifi_EntityEditPacketSender_h

#include <OctreeEditPacketSender.h>

#include "EntityItem.h"

/// Utility for processing, packing, queueing and sending of outbound edit voxel messages.
class EntityEditPacketSender :  public OctreeEditPacketSender {
    Q_OBJECT
public:
    EntityEditPacketSender();

    /// Queues an array of several voxel edit messages. Will potentially send a pending multi-command packet. Determines
    /// which voxel-server node or nodes the packet should be sent to. Can be called even before voxel servers are known, in
    /// which case up to MaxPendingMessages will be buffered and processed when voxel servers are known.
    /// NOTE: EntityItemProperties assumes that all distances are in meter units
    void queueEditEntityMessage(PacketType type, EntityItemID modelID, const EntityItemProperties& properties);

    void queueEraseEntityMessage(const EntityItemID& entityItemID);

    // My server type is the model server
    virtual char getMyNodeType() const { return NodeType::EntityServer; }
    virtual void adjustEditPacketForClockSkew(PacketType type, QByteArray& buffer, int clockSkew);

public slots:
    void processEntityEditNackPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode);
    void toggleNackPackets() { _shouldProcessNack = !_shouldProcessNack; }

private:
    bool _shouldProcessNack = true;
};
#endif // hifi_EntityEditPacketSender_h
