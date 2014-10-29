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

#include <PacketHeaders.h>
#include <RenderArgs.h>
#include <SharedUtil.h>

#include "Octree.h"
#include "OctreePacketData.h"
#include "ViewFrustum.h"

class OctreeRenderer;


// Generic client side Octree renderer class.
class OctreeRenderer : public QObject {
    Q_OBJECT
public:
    OctreeRenderer();
    virtual ~OctreeRenderer();

    virtual Octree* createTree() = 0;
    virtual char getMyNodeType() const = 0;
    virtual PacketType getMyQueryMessageType() const = 0;
    virtual PacketType getExpectedPacketType() const = 0;
    virtual void renderElement(OctreeElement* element, RenderArgs* args) = 0;
    virtual float getSizeScale() const { return DEFAULT_OCTREE_SIZE_SCALE; }
    virtual int getBoundaryLevelAdjust() const { return 0; }

    virtual void setTree(Octree* newTree);
    
    /// process incoming data
    virtual void processDatagram(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode);

    /// initialize and GPU/rendering related resources
    virtual void init();

    /// render the content of the octree
    virtual void render(RenderArgs::RenderMode renderMode = RenderArgs::DEFAULT_RENDER_MODE);

    ViewFrustum* getViewFrustum() const { return _viewFrustum; }
    void setViewFrustum(ViewFrustum* viewFrustum) { _viewFrustum = viewFrustum; }

    static bool renderOperation(OctreeElement* element, void* extraData);

    /// clears the tree
    virtual void clear();
    
    int getElementsTouched() const { return _elementsTouched; }
    int getItemsRendered() const { return _itemsRendered; }
    int getItemsOutOfView() const { return _itemsOutOfView; }
    int getItemsTooSmall() const { return _itemsTooSmall; }

    int getMeshesConsidered() const { return _meshesConsidered; }
    int getMeshesRendered() const { return _meshesRendered; }
    int getMeshesOutOfView() const { return _meshesOutOfView; }
    int getMeshesTooSmall() const { return _meshesTooSmall; }

    int getMaterialSwitches() const { return _materialSwitches; }
    int getTrianglesRendered() const { return _trianglesRendered; }
    int getQuadsRendered() const { return _quadsRendered; }

    int getTranslucentMeshPartsRendered() const { return _translucentMeshPartsRendered; }
    int getOpaqueMeshPartsRendered() const { return _opaqueMeshPartsRendered; }

protected:
    Octree* _tree;
    bool _managedTree;
    ViewFrustum* _viewFrustum;

    int _elementsTouched;
    int _itemsRendered;
    int _itemsOutOfView;
    int _itemsTooSmall;

    int _meshesConsidered;
    int _meshesRendered;
    int _meshesOutOfView;
    int _meshesTooSmall;

    int _materialSwitches;
    int _trianglesRendered;
    int _quadsRendered;

    int _translucentMeshPartsRendered;
    int _opaqueMeshPartsRendered;
};

#endif // hifi_OctreeRenderer_h
