//
//  OctreeRenderer.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__OctreeRenderer__
#define __hifi__OctreeRenderer__

#include <glm/glm.hpp>
#include <stdint.h>

#include <PacketHeaders.h>
#include <SharedUtil.h>

#include "Octree.h"
#include "OctreePacketData.h"
#include "ViewFrustum.h"

class OctreeRenderer;

class RenderArgs {
public:
    int _renderedItems;
    OctreeRenderer* _renderer;
    ViewFrustum* _viewFrustum;
};


// Generic client side Octree renderer class.
class OctreeRenderer  {
public:
    OctreeRenderer();
    virtual ~OctreeRenderer();

    virtual Octree* createTree() = 0;
    virtual NodeType_t getMyNodeType() const = 0;
    virtual PacketType getMyQueryMessageType() const = 0;
    virtual PacketType getExpectedPacketType() const = 0;
    virtual void renderElement(OctreeElement* element, RenderArgs* args) = 0;
    
    /// process incoming data
    virtual void processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr, Node* sourceNode);

    /// initialize and GPU/rendering related resources
    void init();

    /// render the content of the octree
    virtual void render();

    void setDataSourceUUID(const QUuid& dataSourceUUID) { _dataSourceUUID = dataSourceUUID; }
    const QUuid&  getDataSourceUUID() const { return _dataSourceUUID; }
    ViewFrustum* getViewFrustum() const { return _viewFrustum; }
    void setViewFrustum(ViewFrustum* viewFrustum) { _viewFrustum = viewFrustum; }

    static bool renderOperation(OctreeElement* element, void* extraData);

    /// clears the tree
    void clear();
protected:
    Octree* _tree;
    QUuid _dataSourceUUID;
    ViewFrustum* _viewFrustum;
};

#endif /* defined(__hifi__OctreeRenderer__) */