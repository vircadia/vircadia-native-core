//
//  OctreeHeadlessViewer.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/26/14
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__OctreeHeadlessViewer__
#define __hifi__OctreeHeadlessViewer__

#include <PacketHeaders.h>
#include <SharedUtil.h>
#include "JurisdictionListener.h"
#include "Octree.h"
#include "OctreeQuery.h"
#include "OctreeRenderer.h"
#include "Octree.h"
#include "ViewFrustum.h"

// Generic client side Octree renderer class.
class OctreeHeadlessViewer : public OctreeRenderer {
    Q_OBJECT
public:
    OctreeHeadlessViewer();
    virtual ~OctreeHeadlessViewer();
    virtual void renderElement(OctreeElement* element, RenderArgs* args) { /* swallow these */ };

    virtual void init();
    virtual void render() { /* swallow these */ };

    void setJurisdictionListener(JurisdictionListener* jurisdictionListener) { _jurisdictionListener = jurisdictionListener; }
    void queryOctree();

    void setVoxelSizeScale(float sizeScale);
    float getVoxelSizeScale() const { return _voxelSizeScale; }
    void setBoundaryLevelAdjust(int boundaryLevelAdjust);
    int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }
    int getMaxPacketsPerSecond() const { return _maxTotalPPS; }

private:
    ViewFrustum _viewFrustum;
    JurisdictionListener* _jurisdictionListener;
    OctreeQuery _octreeQuery;
    float _voxelSizeScale;
    int _boundaryLevelAdjust;
    int _maxTotalPPS;
};

#endif /* defined(__hifi__OctreeHeadlessViewer__) */