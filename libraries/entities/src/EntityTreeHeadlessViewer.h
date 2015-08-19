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
#include <OctreeHeadlessViewer.h>
#include <ViewFrustum.h>

#include "EntityTree.h"

class EntitySimulation;

// Generic client side Octree renderer class.
class EntityTreeHeadlessViewer : public OctreeHeadlessViewer {
    Q_OBJECT
public:
    EntityTreeHeadlessViewer();
    virtual ~EntityTreeHeadlessViewer();

    virtual char getMyNodeType() const { return NodeType::EntityServer; }
    virtual PacketType getMyQueryMessageType() const { return PacketType::EntityQuery; }
    virtual PacketType getExpectedPacketType() const { return PacketType::EntityData; }

    void update();

    EntityTree* getTree() { return (EntityTree*)_tree; }

    void processEraseMessage(NLPacket& packet, const SharedNodePointer& sourceNode);

    virtual void init();

protected:
    virtual Octree* createTree() { return new EntityTree(true); }

    EntitySimulation* _simulation;
};

#endif // hifi_EntityTreeHeadlessViewer_h
