//
//  VoxelTreeHeadlessViewer.h
//  libraries/voxels/src
//
//  Created by Brad Hefta-Gaub on 2/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VoxelTreeHeadlessViewer_h
#define hifi_VoxelTreeHeadlessViewer_h

#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <Octree.h>
#include <OctreePacketData.h>
#include <OctreeHeadlessViewer.h>
#include "VoxelTree.h"
#include <ViewFrustum.h>

// Generic client side Octree renderer class.
class VoxelTreeHeadlessViewer : public OctreeHeadlessViewer {
    Q_OBJECT
public:
    VoxelTreeHeadlessViewer();
    virtual ~VoxelTreeHeadlessViewer();

    virtual char getMyNodeType() const { return NodeType::VoxelServer; }
    virtual PacketType getMyQueryMessageType() const { return PacketTypeVoxelQuery; }
    virtual PacketType getExpectedPacketType() const { return PacketTypeVoxelData; }

    VoxelTree* getTree() { return (VoxelTree*)_tree; }

    virtual void init();

protected:
    virtual Octree* createTree() { return new VoxelTree(true); }
};

#endif // hifi_VoxelTreeHeadlessViewer_h
