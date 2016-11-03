//
//  OctreeRenderer.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeRenderer_h
#define hifi_OctreeRenderer_h

#include <glm/glm.hpp>
#include <stdint.h>

#include <QObject>

#include <udt/PacketHeaders.h>
#include <RenderArgs.h>
#include <SharedUtil.h>
#include <ViewFrustum.h>

#include "Octree.h"
#include "OctreePacketData.h"

class OctreeRenderer;


// Generic client side Octree renderer class.
class OctreeRenderer : public QObject, public QEnableSharedFromThis<OctreeRenderer> {
    Q_OBJECT
public:
    OctreeRenderer();
    virtual ~OctreeRenderer();

    virtual char getMyNodeType() const = 0;
    virtual PacketType getMyQueryMessageType() const = 0;
    virtual PacketType getExpectedPacketType() const = 0;
    virtual void renderElement(OctreeElementPointer element, RenderArgs* args) { }

    virtual void setTree(OctreePointer newTree);

    /// process incoming data
    virtual void processDatagram(ReceivedMessage& message, SharedNodePointer sourceNode);

    /// initialize and GPU/rendering related resources
    virtual void init();

    /// render the content of the octree
    virtual void render(RenderArgs* renderArgs);

    const ViewFrustum& getViewFrustum() const { return _viewFrustum; }
    void setViewFrustum(const ViewFrustum& viewFrustum) { _viewFrustum = viewFrustum; }

    static bool renderOperation(OctreeElementPointer element, void* extraData);

    /// clears the tree
    virtual void clear();

    float getAverageElementsPerPacket() const { return _elementsPerPacket.getAverage(); }
    float getAverageEntitiesPerPacket() const { return _entitiesPerPacket.getAverage(); }

    float getAveragePacketsPerSecond() const { return _packetsPerSecond.getAverage(); }
    float getAverageElementsPerSecond() const { return _elementsPerSecond.getAverage(); }
    float getAverageEntitiesPerSecond() const { return _entitiesPerSecond.getAverage(); }

    float getAverageWaitLockPerPacket() const { return _waitLockPerPacket.getAverage(); }
    float getAverageUncompressPerPacket() const { return _uncompressPerPacket.getAverage(); }
    float getAverageReadBitstreamPerPacket() const { return _readBitstreamPerPacket.getAverage(); }

protected:
    virtual OctreePointer createTree() = 0;

    OctreePointer _tree;
    bool _managedTree;
    ViewFrustum _viewFrustum;

    SimpleMovingAverage _elementsPerPacket;
    SimpleMovingAverage _entitiesPerPacket;

    SimpleMovingAverage _packetsPerSecond;
    SimpleMovingAverage _elementsPerSecond;
    SimpleMovingAverage _entitiesPerSecond;

    SimpleMovingAverage _waitLockPerPacket;
    SimpleMovingAverage _uncompressPerPacket;
    SimpleMovingAverage _readBitstreamPerPacket;

    quint64 _lastWindowAt = 0;
    int _packetsInLastWindow = 0;
    int _elementsInLastWindow = 0;
    int _entitiesInLastWindow = 0;

};

#endif // hifi_OctreeRenderer_h
