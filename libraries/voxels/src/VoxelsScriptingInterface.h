//
//  VoxelsScriptingInterface.h
//  libraries/voxels/src
//
//  Created by Stephen Birarda on 9/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VoxelsScriptingInterface_h
#define hifi_VoxelsScriptingInterface_h

#include <QtCore/QObject>

#include <OctreeScriptingInterface.h>
#include <RegisteredMetaTypes.h>

#include "VoxelConstants.h"
#include "VoxelEditPacketSender.h"
#include "VoxelTree.h"

class QUndoStack;

/// handles scripting of voxel commands from JS passed to assigned clients
class VoxelsScriptingInterface : public OctreeScriptingInterface {
    Q_OBJECT
public:
    VoxelsScriptingInterface() : _tree(NULL), _undoStack(NULL) {};
    VoxelEditPacketSender* getVoxelPacketSender() { return (VoxelEditPacketSender*)getPacketSender(); }

    virtual NodeType_t getServerNodeType() const { return NodeType::VoxelServer; }
    virtual OctreeEditPacketSender* createPacketSender() { return new VoxelEditPacketSender(); }
    void setVoxelTree(VoxelTree* tree) { _tree = tree; }
    void setUndoStack(QUndoStack* undoStack) { _undoStack = undoStack; }

public:
    /// provide the world scale
    Q_INVOKABLE const int getTreeScale() const { return TREE_SCALE; }

    /// checks the local voxel tree for a voxel at the specified location and scale
    /// \param x the x-coordinate of the voxel (in meter units)
    /// \param y the y-coordinate of the voxel (in meter units)
    /// \param z the z-coordinate of the voxel (in meter units)
    /// \param scale the scale of the voxel (in meter units)
    Q_INVOKABLE VoxelDetail getVoxelAt(float x, float y, float z, float scale);

    /// queues the creation of a voxel which will be sent by calling process on the PacketSender
    /// \param x the x-coordinate of the voxel (in meter units)
    /// \param y the y-coordinate of the voxel (in meter units)
    /// \param z the z-coordinate of the voxel (in meter units)
    /// \param scale the scale of the voxel (in meter units)
    /// \param red the R value for RGB color of voxel
    /// \param green the G value for RGB color of voxel
    /// \param blue the B value for RGB color of voxel
    Q_INVOKABLE void setVoxelNonDestructive(float x, float y, float z, float scale, uchar red, uchar green, uchar blue);

    /// queues the destructive creation of a voxel which will be sent by calling process on the PacketSender
    /// \param x the x-coordinate of the voxel (in meter units)
    /// \param y the y-coordinate of the voxel (in meter units)
    /// \param z the z-coordinate of the voxel (in meter units)
    /// \param scale the scale of the voxel (in meter units)
    /// \param red the R value for RGB color of voxel
    /// \param green the G value for RGB color of voxel
    /// \param blue the B value for RGB color of voxel
    Q_INVOKABLE void setVoxel(float x, float y, float z, float scale, uchar red, uchar green, uchar blue);

    /// queues the deletion of a voxel, sent by calling process on the PacketSender
    /// \param x the x-coordinate of the voxel (in meter units)
    /// \param y the y-coordinate of the voxel (in meter units)
    /// \param z the z-coordinate of the voxel (in meter units)
    /// \param scale the scale of the voxel (in meter units)
    Q_INVOKABLE void eraseVoxel(float x, float y, float z, float scale);

    /// If the scripting context has visible voxels, this will determine a ray intersection, the results
    /// may be inaccurate if the engine is unable to access the visible voxels, in which case result.accurate
    /// will be false.
    Q_INVOKABLE RayToVoxelIntersectionResult findRayIntersection(const PickRay& ray);

    /// If the scripting context has visible voxels, this will determine a ray intersection, and will block in
    /// order to return an accurate result
    Q_INVOKABLE RayToVoxelIntersectionResult findRayIntersectionBlocking(const PickRay& ray);

    /// returns a voxel space axis aligned vector for the face, useful in doing voxel math
    Q_INVOKABLE glm::vec3 getFaceVector(const QString& face);
    
    /// checks the local voxel tree for the smallest voxel enclosing the point
    /// \param point the x,y,z coordinates of the point (in meter units)
    /// \return VoxelDetail - if no voxel encloses the point then VoxelDetail items will be 0
    Q_INVOKABLE VoxelDetail getVoxelEnclosingPoint(const glm::vec3& point);

    /// checks the local voxel tree for the smallest voxel enclosing the point and uses a blocking lock
    /// \param point the x,y,z coordinates of the point (in meter units)
    /// \return VoxelDetail - if no voxel encloses the point then VoxelDetail items will be 0
    Q_INVOKABLE VoxelDetail getVoxelEnclosingPointBlocking(const glm::vec3& point);

private:
    /// actually does the work of finding the ray intersection, can be called in locking mode or tryLock mode
    RayToVoxelIntersectionResult findRayIntersectionWorker(const PickRay& ray, Octree::lockType lockType);

    void queueVoxelAdd(PacketType addPacketType, VoxelDetail& addVoxelDetails);
    VoxelTree* _tree;
    QUndoStack* _undoStack;
};

#endif // hifi_VoxelsScriptingInterface_h
