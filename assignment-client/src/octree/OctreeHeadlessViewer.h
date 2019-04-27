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

    /**jsdoc
     * @function EntityViewer.queryOctree
     */
    void queryOctree();


    // setters for camera attributes

    /**jsdoc
     * @function EntityViewer.setPosition
     * @param {Vec3} position
     */
    void setPosition(const glm::vec3& position) { _hasViewFrustum = true; _viewFrustum.setPosition(position); }

    /**jsdoc
     * @function EntityViewer.setOrientation
     * @param {Quat} orientation
     */
    void setOrientation(const glm::quat& orientation) { _hasViewFrustum = true; _viewFrustum.setOrientation(orientation); }

    /**jsdoc
     * @function EntityViewer.setCenterRadius
     * @param {number} radius
     */
    void setCenterRadius(float radius) { _hasViewFrustum = true; _viewFrustum.setCenterRadius(radius); }

    /**jsdoc
     * @function EntityViewer.setKeyholeRadius
     * @param {number} radius
     * @deprecated This function is deprecated and will be removed. Use {@link EntityViewer.setCenterRadius|setCenterRadius} 
     *     instead.
     */
    void setKeyholeRadius(float radius) { _hasViewFrustum = true; _viewFrustum.setCenterRadius(radius); } // TODO: remove this legacy support


    // setters for LOD and PPS

    /**jsdoc
     * @function EntityViewer.setVoxelSizeScale
     * @param {number} sizeScale
     */
    void setVoxelSizeScale(float sizeScale) { _octreeQuery.setOctreeSizeScale(sizeScale) ; }

    /**jsdoc
     * @function EntityViewer.setBoundaryLevelAdjust
     * @param {number} boundaryLevelAdjust
     */
    void setBoundaryLevelAdjust(int boundaryLevelAdjust) { _octreeQuery.setBoundaryLevelAdjust(boundaryLevelAdjust); }

    /**jsdoc
     * @function EntityViewer.setMaxPacketsPerSecond
     * @param {number} maxPacketsPerSecond
     */
    void setMaxPacketsPerSecond(int maxPacketsPerSecond) { _octreeQuery.setMaxQueryPacketsPerSecond(maxPacketsPerSecond); }

    // getters for camera attributes

    /**jsdoc
     * @function EntityViewer.getPosition
     * @returns {Vec3}
     */
    const glm::vec3& getPosition() const { return _viewFrustum.getPosition(); }

    /**jsdoc
     * @function EntityViewer.getOrientation
     * @returns {Quat}
     */
    const glm::quat& getOrientation() const { return _viewFrustum.getOrientation(); }


    // getters for LOD and PPS

    /**jsdoc
     * @function EntityViewer.getVoxelSizeScale
     * @returns {number}
     */
    float getVoxelSizeScale() const { return _octreeQuery.getOctreeSizeScale(); }

    /**jsdoc
     * @function EntityViewer.getBoundaryLevelAdjust
     * @returns {number}
     */
    int getBoundaryLevelAdjust() const { return _octreeQuery.getBoundaryLevelAdjust(); }

    /**jsdoc
     * @function EntityViewer.getMaxPacketsPerSecond
     * @returns {number}
     */
    int getMaxPacketsPerSecond() const { return _octreeQuery.getMaxQueryPacketsPerSecond(); }


    /**jsdoc
     * @function EntityViewer.getOctreeElementsCount
     * @returns {number}
     */
    unsigned getOctreeElementsCount() const { return _tree->getOctreeElementsCount(); }

private:
    OctreeQuery _octreeQuery;

    bool _hasViewFrustum { false };
    ViewFrustum _viewFrustum;
};

#endif // hifi_OctreeHeadlessViewer_h
