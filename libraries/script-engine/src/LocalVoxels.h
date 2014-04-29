//
//  LocalVoxels.h
//  libraries/script-engine/src
//
//  Created by Clément Brisset on 2/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LocalVoxels_h
#define hifi_LocalVoxels_h

#include <QObject>

#include <RegisteredMetaTypes.h>
#include <LocalVoxelsList.h>


/// object allowing JS scripters to use their own local trees
class LocalVoxels : public QObject {
    Q_OBJECT
    
public:
    LocalVoxels(QString name);
    ~LocalVoxels();
    
    /// checks the local voxel tree for a voxel at the specified location and scale
    /// \param x the x-coordinate of the voxel (in meter units)
    /// \param y the y-coordinate of the voxel (in meter units)
    /// \param z the z-coordinate of the voxel (in meter units)
    /// \param scale the scale of the voxel (in meter units)
    Q_INVOKABLE VoxelDetail getVoxelAt(float x, float y, float z, float scale);
    
    /// creates a non destructive voxel in the local tree
    /// \param x the x-coordinate of the voxel (in meter units)
    /// \param y the y-coordinate of the voxel (in meter units)
    /// \param z the z-coordinate of the voxel (in meter units)
    /// \param scale the scale of the voxel (in meter units)
    /// \param red the R value for RGB color of voxel
    /// \param green the G value for RGB color of voxel
    /// \param blue the B value for RGB color of voxel
    Q_INVOKABLE void setVoxelNonDestructive(float x, float y, float z, float scale, uchar red, uchar green, uchar blue);
    
    /// creates a voxel in the local tree
    /// \param x the x-coordinate of the voxel (in meter units)
    /// \param y the y-coordinate of the voxel (in meter units)
    /// \param z the z-coordinate of the voxel (in meter units)
    /// \param scale the scale of the voxel (in meter units)
    /// \param red the R value for RGB color of voxel
    /// \param green the G value for RGB color of voxel
    /// \param blue the B value for RGB color of voxel
    Q_INVOKABLE void setVoxel(float x, float y, float z, float scale, uchar red, uchar green, uchar blue);
    
    /// erase the voxel and its children at the given coordinate
    /// \param x the x-coordinate of the voxel (in meter units)
    /// \param y the y-coordinate of the voxel (in meter units)
    /// \param z the z-coordinate of the voxel (in meter units)
    /// \param scale the scale of the voxel (in meter units)
    Q_INVOKABLE void eraseVoxel(float x, float y, float z, float scale);
    
    /// copy the given subtree onto destination's root node
    /// \param x the x-coordinate of the subtree (in meter units)
    /// \param y the y-coordinate of the subtree (in meter units)
    /// \param z the z-coordinate of the subtree (in meter units)
    /// \param scale the scale of the subtree (in meter units)
    /// \param destination LocalVoxels' destination tree
    Q_INVOKABLE void copyTo(float x, float y, float z, float scale, const QString destination);
    
    ///copy source in the given subtree
    /// \param x the x-coordinate of the subtree (in meter units)
    /// \param y the y-coordinate of the subtree (in meter units)
    /// \param z the z-coordinate of the subtree (in meter units)
    /// \param scale the scale of the subtree (in meter units)
    /// \param source LocalVoxels' source tree
    Q_INVOKABLE void pasteFrom(float x, float y, float z, float scale, const QString source);
    
    /// If the scripting context has visible voxels, this will determine a ray intersection, the results
    /// may be inaccurate if the engine is unable to access the visible voxels, in which case result.accurate
    /// will be false.
    Q_INVOKABLE RayToVoxelIntersectionResult findRayIntersection(const PickRay& ray);

    /// If the scripting context has visible voxels, this will determine a ray intersection, and will block in
    /// order to return an accurate result
    Q_INVOKABLE RayToVoxelIntersectionResult findRayIntersectionBlocking(const PickRay& ray);
    
    /// returns a voxel space axis aligned vector for the face, useful in doing voxel math
    Q_INVOKABLE glm::vec3 getFaceVector(const QString& face);
    
private:
    /// actually does the work of finding the ray intersection, can be called in locking mode or tryLock mode
    RayToVoxelIntersectionResult findRayIntersectionWorker(const PickRay& ray, Octree::lockType lockType);

    QString _name;
    StrongVoxelTreePointer _tree;
};




#endif // hifi_LocalVoxels_h
