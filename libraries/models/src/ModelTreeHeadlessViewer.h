//
//  ModelTreeHeadlessViewer.h
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 2/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelTreeHeadlessViewer_h
#define hifi_ModelTreeHeadlessViewer_h

#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <Octree.h>
#include <OctreePacketData.h>
#include <OctreeHeadlessViewer.h>
#include <ViewFrustum.h>

#include "ModelTree.h"

// Generic client side Octree renderer class.
class ModelTreeHeadlessViewer : public OctreeHeadlessViewer {
    Q_OBJECT
public:
    ModelTreeHeadlessViewer();
    virtual ~ModelTreeHeadlessViewer();

    virtual Octree* createTree() { return new ModelTree(true); }
    virtual NodeType_t getMyNodeType() const { return NodeType::ModelServer; }
    virtual PacketType getMyQueryMessageType() const { return PacketTypeModelQuery; }
    virtual PacketType getExpectedPacketType() const { return PacketTypeModelData; }

    void update();

    ModelTree* getTree() { return (ModelTree*)_tree; }

    void processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode);

    virtual void init();
};

#endif // hifi_ModelTreeHeadlessViewer_h
