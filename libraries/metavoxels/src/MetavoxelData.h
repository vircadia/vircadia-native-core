//
//  MetavoxelData.h
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__MetavoxelData__
#define __interface__MetavoxelData__

#include <QScopedPointer>
#include <QVector>

#include "AttributeRegistry.h"

class MetavoxelNode;
class VoxelVisitor;

/// The base metavoxel representation shared between server and client.
class MetavoxelData {
public:

    /// Applies the specified function to the contained voxels.
    /// \param attributes the list of attributes desired
    void visitVoxels(const QVector<AttributeID>& attributes, VoxelVisitor* visitor);

private:
   
    
};

/// A single layer of metavoxel data (a tree containing nodes of the same type).
class MetavoxelLayer {
public:

private:

    QScopedPointer<MetavoxelNode> _root;
};

/// A single node within a metavoxel layer.
class MetavoxelNode {
public:

    static const int CHILD_COUNT = 8;

    

private:
    
    QScopedPointer<MetavoxelNode> _children[CHILD_COUNT];
};

/// Interface for visitors to voxels.
class VoxelVisitor {
public:
    
    /// Visits a voxel.
    /// \param attributeValues the values of the desired attributes
    virtual void visit(const QVector<void*>& attributeValues) = 0;
};

#endif /* defined(__interface__MetavoxelData__) */
