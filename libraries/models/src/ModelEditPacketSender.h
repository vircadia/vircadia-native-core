//
//  ModelEditPacketSender.h
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelEditPacketSender_h
#define hifi_ModelEditPacketSender_h

#include <OctreeEditPacketSender.h>

#include "ModelItem.h"

/// Utility for processing, packing, queueing and sending of outbound edit voxel messages.
class ModelEditPacketSender :  public OctreeEditPacketSender {
    Q_OBJECT
public:
    /// Send model add message immediately
    /// NOTE: ModelItemProperties assumes that all distances are in meter units
    void sendEditModelMessage(PacketType type, ModelItemID modelID, const ModelItemProperties& properties);

    /// Queues an array of several voxel edit messages. Will potentially send a pending multi-command packet. Determines
    /// which voxel-server node or nodes the packet should be sent to. Can be called even before voxel servers are known, in
    /// which case up to MaxPendingMessages will be buffered and processed when voxel servers are known.
    /// NOTE: ModelItemProperties assumes that all distances are in meter units
    void queueModelEditMessage(PacketType type, ModelItemID modelID, const ModelItemProperties& properties);

    // My server type is the model server
    virtual char getMyNodeType() const { return NodeType::ModelServer; }
    virtual void adjustEditPacketForClockSkew(unsigned char* codeColorBuffer, size_t length, int clockSkew);
};
#endif // hifi_ModelEditPacketSender_h
