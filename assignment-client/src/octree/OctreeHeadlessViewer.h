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
#include <JurisdictionListener.h>
#include <OctreeQuery.h>


// Generic client side Octree renderer class.
class OctreeHeadlessViewer : public OctreeProcessor {
    Q_OBJECT
public:
    OctreeHeadlessViewer();
    virtual ~OctreeHeadlessViewer() {};

    void setJurisdictionListener(JurisdictionListener* jurisdictionListener) { _jurisdictionListener = jurisdictionListener; }
    
    OctreeQuery& getOctreeQuery() { return _octreeQuery; }

    static int parseOctreeStats(QSharedPointer<ReceivedMessage> message, SharedNodePointer sourceNode);
    static void trackIncomingOctreePacket(const QByteArray& packet, const SharedNodePointer& sendingNode, bool wasStatsPacket);

public slots:
    void queryOctree();

    // setters for camera attributes
    void setPosition(const glm::vec3& position) { _viewFrustum.setPosition(position); }
    void setOrientation(const glm::quat& orientation) { _viewFrustum.setOrientation(orientation); }
    void setCenterRadius(float radius) { _viewFrustum.setCenterRadius(radius); }
    void setKeyholeRadius(float radius) { _viewFrustum.setCenterRadius(radius); } // TODO: remove this legacy support

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
    JurisdictionListener* _jurisdictionListener = nullptr;
    OctreeQuery _octreeQuery;

    ViewFrustum _viewFrustum;
    float _voxelSizeScale { DEFAULT_OCTREE_SIZE_SCALE };
    int _boundaryLevelAdjust { 0 };
    int _maxPacketsPerSecond { DEFAULT_MAX_OCTREE_PPS };
};

#endif // hifi_OctreeHeadlessViewer_h
