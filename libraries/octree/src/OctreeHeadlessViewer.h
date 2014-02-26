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
#include "OctreeConstants.h"
#include "OctreeQuery.h"
#include "OctreeRenderer.h"
#include "OctreeSceneStats.h"
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

    static int parseOctreeStats(const QByteArray& packet, const SharedNodePointer& sourceNode);
    static void trackIncomingOctreePacket(const QByteArray& packet, const SharedNodePointer& sendingNode, bool wasStatsPacket);

public slots:
    void queryOctree();

    // setters for camera attributes
    void setPosition(const glm::vec3& position) { _viewFrustum.setPosition(position); }
    void setOrientation(const glm::quat& orientation) { _viewFrustum.setOrientation(orientation); }

    // setters for LOD and PPS
    void setVoxelSizeScale(float sizeScale) { _voxelSizeScale = sizeScale; }
    void setBoundaryLevelAdjust(int boundaryLevelAdjust) { _boundaryLevelAdjust = boundaryLevelAdjust; }
    void setMaxPacketsPerSecond(int maxPacketsPerSecond) { _maxPacketsPerSecond = maxPacketsPerSecond; }

    // getters for camera attributes
    const glm::vec3& getPosition() const { return _viewFrustum.getPosition(); }
    const glm::quat& getOrientation() const { return _viewFrustum.getOrientation(); }

    // getters for LOD and PPS
    float getVoxelSizeScale() const { return _voxelSizeScale; }
    int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }
    int getMaxPacketsPerSecond() const { return _maxPacketsPerSecond; }

private:
    ViewFrustum _viewFrustum;
    JurisdictionListener* _jurisdictionListener;
    OctreeQuery _octreeQuery;
    float _voxelSizeScale;
    int _boundaryLevelAdjust;
    int _maxPacketsPerSecond;
};

#endif /* defined(__hifi__OctreeHeadlessViewer__) */