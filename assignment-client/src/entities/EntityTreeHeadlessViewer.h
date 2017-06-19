//
//  EntityTreeHeadlessViewer.h
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 2/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityTreeHeadlessViewer_h
#define hifi_EntityTreeHeadlessViewer_h

#include <udt/PacketHeaders.h>
#include <SharedUtil.h>
#include <Octree.h>
#include <OctreePacketData.h>
#include <ViewFrustum.h>

#include "../octree/OctreeHeadlessViewer.h"
#include "EntityTree.h"

class EntitySimulation;

// Generic client side Octree renderer class.
class EntityTreeHeadlessViewer : public OctreeHeadlessViewer {
    Q_OBJECT
public:
    EntityTreeHeadlessViewer();
    virtual ~EntityTreeHeadlessViewer();

    virtual char getMyNodeType() const override { return NodeType::EntityServer; }
    virtual PacketType getMyQueryMessageType() const override { return PacketType::EntityQuery; }
    virtual PacketType getExpectedPacketType() const override { return PacketType::EntityData; }

    void update();

    EntityTreePointer getTree() { return std::static_pointer_cast<EntityTree>(_tree); }

    void processEraseMessage(ReceivedMessage& message, const SharedNodePointer& sourceNode);

    virtual void init() override;

protected:
    virtual OctreePointer createTree() override {
        EntityTreePointer newTree = EntityTreePointer(new EntityTree(true));
        newTree->createRootElement();
        return newTree;
    }

    EntitySimulationPointer _simulation;
};

#endif // hifi_EntityTreeHeadlessViewer_h
