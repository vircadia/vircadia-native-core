//
//  MetavoxelData.h
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__MetavoxelData__
#define __interface__MetavoxelData__

#include <QBitArray>
#include <QHash>
#include <QScopedPointer>

#include "AttributeRegistry.h"

class MetavoxelNode;
class MetavoxelPath;
class VoxelVisitor;

/// The base metavoxel representation shared between server and client.
class MetavoxelData {
public:

    ~MetavoxelData();

    /// Applies the specified function to the contained voxels.
    /// \param attributes the list of attributes desired
    void visitVoxels(const QVector<Attribute*>& attributes, VoxelVisitor* visitor);

    /// Sets the attribute value corresponding to the specified path.
    void setAttributeValue(const MetavoxelPath& path, const AttributeValue& attributeValue);

    /// Retrieves the attribute value corresponding to the specified path.
    AttributeValue getAttributeValue(const MetavoxelPath& path, const AttributePointer& attribute) const;

private:
   
    QHash<AttributePointer, MetavoxelNode*> _roots;
};

/// A single node within a metavoxel layer.
class MetavoxelNode {
public:

    static const int CHILD_COUNT = 8;

    MetavoxelNode(const AttributeValue& attributeValue);

    /// Descends the voxel tree in order to set the value of a node.
    /// \param path the path to follow
    /// \param index the position in the path
    /// \return whether or not the node is entirely equal to the default attribute, and can thus be collapsed
    bool setAttributeValue(const MetavoxelPath& path, int index, const AttributeValue& attributeValue);

    void setAttributeValue(const AttributeValue& attributeValue);

    AttributeValue getAttributeValue(const AttributePointer& attribute) const;

    MetavoxelNode* getChild(int index) const { return _children[index]; }
    void setChild(int index, MetavoxelNode* child) { _children[index] = child; }

    void destroy(const AttributePointer& attribute);

private:
    Q_DISABLE_COPY(MetavoxelNode)
    
    bool allChildrenNull() const;
    
    void* _attributeValue;
    MetavoxelNode* _children[CHILD_COUNT];
};

/// A path down an octree.
class MetavoxelPath {
public:
    
    int getSize() const { return _array.size() / BITS_PER_ELEMENT; }
    bool isEmpty() const { return _array.isEmpty(); }
    
    int operator[](int index) const;
    
    MetavoxelPath& operator+=(int element);
    
private:
    
    static const int BITS_PER_ELEMENT = 3;
    
    QBitArray _array;    
};

/// Interface for visitors to voxels.
class VoxelVisitor {
public:
    
    /// Visits a voxel.
    /// \param attributeValues the values of the desired attributes
    virtual void visit(const QVector<void*>& attributeValues) = 0;
};

#endif /* defined(__interface__MetavoxelData__) */
