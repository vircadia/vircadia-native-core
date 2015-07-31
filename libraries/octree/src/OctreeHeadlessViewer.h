//
//  OctreeHeadlessViewer.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 2/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeHeadlessViewer_h
#define hifi_OctreeHeadlessViewer_h

#include <udt/PacketHeaders.h>
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
    virtual void renderElement(OctreeElement* element, RenderArgs* args) { /* swallow these */ }

    virtual void init();
    virtual void render(RenderArgs* renderArgs) override { /* swallow these */ }

    void setJurisdictionListener(JurisdictionListener* jurisdictionListener) { _jurisdictionListener = jurisdictionListener; }

    static int parseOctreeStats(QSharedPointer<NLPacket> packet, SharedNodePointer sourceNode);
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

    unsigned getOctreeElementsCount() const { return _tree->getOctreeElementsCount(); }

private:
    ViewFrustum _viewFrustum;
    JurisdictionListener* _jurisdictionListener;
    OctreeQuery _octreeQuery;
    float _voxelSizeScale;
    int _boundaryLevelAdjust;
    int _maxPacketsPerSecond;
};

#endif // hifi_OctreeHeadlessViewer_h
