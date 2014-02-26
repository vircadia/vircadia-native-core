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
#include "Octree.h"
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
private:
    ViewFrustum _viewFrustum;
};

#endif /* defined(__hifi__OctreeHeadlessViewer__) */