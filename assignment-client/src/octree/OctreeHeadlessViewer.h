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

#include <OctreeProcessor.h>
#include <OctreeQuery.h>


// Generic client side Octree renderer class.
class OctreeHeadlessViewer : public OctreeProcessor {
    Q_OBJECT
public:
    OctreeQuery& getOctreeQuery() { return _octreeQuery; }

    static int parseOctreeStats(QSharedPointer<ReceivedMessage> message, SharedNodePointer sourceNode);
    static void trackIncomingOctreePacket(const QByteArray& packet, const SharedNodePointer& sendingNode, bool wasStatsPacket);

public slots:
    void queryOctree();

    // setters for camera attributes
    void setPosition(const glm::vec3& position) { _hasViewFrustum = true; _viewFrustum.setPosition(position); }
    void setOrientation(const glm::quat& orientation) { _hasViewFrustum = true; _viewFrustum.setOrientation(orientation); }
    void setCenterRadius(float radius) { _hasViewFrustum = true; _viewFrustum.setCenterRadius(radius); }
    void setKeyholeRadius(float radius) { _hasViewFrustum = true; _viewFrustum.setCenterRadius(radius); } // TODO: remove this legacy support

    // setters for LOD and PPS
    void setVoxelSizeScale(float sizeScale) { _octreeQuery.setOctreeSizeScale(sizeScale) ; }
    void setBoundaryLevelAdjust(int boundaryLevelAdjust) { _octreeQuery.setBoundaryLevelAdjust(boundaryLevelAdjust); }
    void setMaxPacketsPerSecond(int maxPacketsPerSecond) { _octreeQuery.setMaxQueryPacketsPerSecond(maxPacketsPerSecond); }

    // getters for camera attributes
    const glm::vec3& getPosition() const { return _viewFrustum.getPosition(); }
    const glm::quat& getOrientation() const { return _viewFrustum.getOrientation(); }

    // getters for LOD and PPS
    float getVoxelSizeScale() const { return _octreeQuery.getOctreeSizeScale(); }
    int getBoundaryLevelAdjust() const { return _octreeQuery.getBoundaryLevelAdjust(); }
    int getMaxPacketsPerSecond() const { return _octreeQuery.getMaxQueryPacketsPerSecond(); }

    unsigned getOctreeElementsCount() const { return _tree->getOctreeElementsCount(); }

private:
    OctreeQuery _octreeQuery;

    bool _hasViewFrustum { false };
    ViewFrustum _viewFrustum;
};

#endif // hifi_OctreeHeadlessViewer_h
