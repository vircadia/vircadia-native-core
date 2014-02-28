//
//  VoxelTreeHeadlessViewer.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/26/14
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__VoxelTreeHeadlessViewer__
#define __hifi__VoxelTreeHeadlessViewer__

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

    virtual Octree* createTree() { return new VoxelTree(true); }
    virtual NodeType_t getMyNodeType() const { return NodeType::VoxelServer; }
    virtual PacketType getMyQueryMessageType() const { return PacketTypeVoxelQuery; }
    virtual PacketType getExpectedPacketType() const { return PacketTypeVoxelData; }

    VoxelTree* getTree() { return (VoxelTree*)_tree; }

    virtual void init();
};

#endif /* defined(__hifi__VoxelTreeHeadlessViewer__) */