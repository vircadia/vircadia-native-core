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

#include <QtCore/QSharedPointer>

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

    /*@jsdoc
     * Updates the entities currently in view.
     * @function EntityViewer.queryOctree
     */
    void queryOctree();


    // setters for camera attributes

    /*@jsdoc
     * Sets the position of the view frustum.
     * @function EntityViewer.setPosition
     * @param {Vec3} position - The position of the view frustum.
     */
    void setPosition(const glm::vec3& position) { _hasViewFrustum = true; _viewFrustum.setPosition(position); }

    /*@jsdoc
     * Sets the orientation of the view frustum.
     * @function EntityViewer.setOrientation
     * @param {Quat} orientation - The orientation of the view frustum.
     */
    void setOrientation(const glm::quat& orientation) { _hasViewFrustum = true; _viewFrustum.setOrientation(orientation); }

    /*@jsdoc
     * Sets the radius of the center "keyhole" in the view frustum.
     * @function EntityViewer.setCenterRadius
     * @param {number} radius - The radius of the center "keyhole" in the view frustum.
     */
    void setCenterRadius(float radius) { _hasViewFrustum = true; _viewFrustum.setCenterRadius(radius); }

    /*@jsdoc
     * Sets the radius of the center "keyhole" in the view frustum.
     * @function EntityViewer.setKeyholeRadius
     * @param {number} radius - The radius of the center "keyhole" in the view frustum.
     * @deprecated This function is deprecated and will be removed. Use {@link EntityViewer.setCenterRadius|setCenterRadius} 
     *     instead.
     */
    void setKeyholeRadius(float radius) { _hasViewFrustum = true; _viewFrustum.setCenterRadius(radius); } // TODO: remove this legacy support


    // setters for LOD and PPS

    /*@jsdoc
     * @function EntityViewer.setVoxelSizeScale
     * @param {number} sizeScale - The voxel size scale.
     * @deprecated This function is deprecated and will be removed.
     */
    void setVoxelSizeScale(float sizeScale) { _octreeQuery.setOctreeSizeScale(sizeScale) ; }

    /*@jsdoc
     * @function EntityViewer.setBoundaryLevelAdjust
     * @param {number} boundaryLevelAdjust - The boundary level adjust factor.
     * @deprecated This function is deprecated and will be removed.
     */
    void setBoundaryLevelAdjust(int boundaryLevelAdjust) { _octreeQuery.setBoundaryLevelAdjust(boundaryLevelAdjust); }

    /*@jsdoc
     * Sets the maximum number of entity packets to receive from the domain server per second.
     * @function EntityViewer.setMaxPacketsPerSecond
     * @param {number} maxPacketsPerSecond - The maximum number of entity packets to receive per second.
     */
    void setMaxPacketsPerSecond(int maxPacketsPerSecond) { _octreeQuery.setMaxQueryPacketsPerSecond(maxPacketsPerSecond); }

    // getters for camera attributes

    /*@jsdoc
     * Gets the position of the view frustum.
     * @function EntityViewer.getPosition
     * @returns {Vec3} The position of the view frustum.
     */
    const glm::vec3& getPosition() const { return _viewFrustum.getPosition(); }

    /*@jsdoc
     * Gets the orientation of the view frustum.
     * @function EntityViewer.getOrientation
     * @returns {Quat} The orientation of the view frustum.
     */
    const glm::quat& getOrientation() const { return _viewFrustum.getOrientation(); }


    // getters for LOD and PPS

    /*@jsdoc
     * @function EntityViewer.getVoxelSizeScale
     * @returns {number} The voxel size scale.
     * @deprecated This function is deprecated and will be removed.
     */
    float getVoxelSizeScale() const { return _octreeQuery.getOctreeSizeScale(); }

    /*@jsdoc
     * @function EntityViewer.getBoundaryLevelAdjust
     * @returns {number} The boundary level adjust factor.
     * @deprecated This function is deprecated and will be removed.
     */
    int getBoundaryLevelAdjust() const { return _octreeQuery.getBoundaryLevelAdjust(); }

    /*@jsdoc
     * Gets the maximum number of entity packets to receive from the domain server per second.
     * @function EntityViewer.getMaxPacketsPerSecond
     * @returns {number} The maximum number of entity packets to receive per second.
     */
    int getMaxPacketsPerSecond() const { return _octreeQuery.getMaxQueryPacketsPerSecond(); }


    /*@jsdoc
     * Gets the number of nodes in the octree.
     * @function EntityViewer.getOctreeElementsCount
     * @returns {number} The number of nodes in the octree.
     */
    unsigned getOctreeElementsCount() const { return _tree->getOctreeElementsCount(); }

private:
    OctreeQuery _octreeQuery;

    bool _hasViewFrustum { false };
    ViewFrustum _viewFrustum;
};

#endif // hifi_OctreeHeadlessViewer_h
