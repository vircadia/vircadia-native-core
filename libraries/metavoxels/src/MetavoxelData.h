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
#include <QScriptString>
#include <QScriptValue>
#include <QVector>

#include <glm/glm.hpp>

#include "AttributeRegistry.h"

class QScriptContext;

class MetavoxelNode;
class MetavoxelPath;
class MetavoxelVisitation;
class MetavoxelVisitor;

/// The base metavoxel representation shared between server and client.
class MetavoxelData {
public:

    ~MetavoxelData();

    /// Applies the specified visitor to the contained voxels.
    void guide(MetavoxelVisitor& visitor);

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
    /// \return whether or not the node is entirely equal to the value
    bool setAttributeValue(const MetavoxelPath& path, int index, const AttributeValue& attributeValue);

    void setAttributeValue(const AttributeValue& attributeValue);

    AttributeValue getAttributeValue(const AttributePointer& attribute) const;

    MetavoxelNode* getChild(int index) const { return _children[index]; }
    void setChild(int index, MetavoxelNode* child) { _children[index] = child; }

    bool isLeaf() const;

    void destroy(const AttributePointer& attribute);

private:
    Q_DISABLE_COPY(MetavoxelNode)
    
    void clearChildren(const AttributePointer& attribute);
    
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

/// Contains information about a metavoxel (explicit or procedural).
class MetavoxelInfo {
public:
    
    glm::vec3 minimum; ///< the minimum extent of the area covered by the voxel
    float size; ///< the size of the voxel in all dimensions
    QVector<AttributeValue> attributeValues;
    bool isLeaf;
};

/// Interface for visitors to metavoxels.
class MetavoxelVisitor {
public:
    
    MetavoxelVisitor(const QVector<AttributePointer>& attributes) : _attributes(attributes) { }
        
    /// Returns a reference to the list of attributes desired.
    const QVector<AttributePointer>& getAttributes() const { return _attributes; }
    
    /// Visits a metavoxel.
    /// \param info the metavoxel ata
    /// \param if true, continue descending; if false, stop
    virtual bool visit(const MetavoxelInfo& info) = 0;

protected:

    QVector<AttributePointer> _attributes;
};

/// Interface for objects that guide metavoxel visitors.
class MetavoxelGuide : public PolymorphicData {
public:
    
    /// Guides the specified visitor to the contained voxels.
    virtual void guide(MetavoxelVisitation& visitation) = 0;
};

/// Guides visitors through the explicit content of the system.
class DefaultMetavoxelGuide : public MetavoxelGuide {
public:
    
    virtual PolymorphicData* clone() const;
    
    virtual void guide(MetavoxelVisitation& visitation);
};

/// Represents a guide implemented in Javascript.
class ScriptedMetavoxelGuide : public MetavoxelGuide {
public:

    ScriptedMetavoxelGuide(const QScriptValue& guideFunction);

    virtual PolymorphicData* clone() const;
    
    virtual void guide(MetavoxelVisitation& visitation);

private:

    static QScriptValue getAttributes(QScriptContext* context, QScriptEngine* engine);
    static QScriptValue visit(QScriptContext* context, QScriptEngine* engine);

    QScriptValue _guideFunction;
    QScriptString _minimumHandle;
    QScriptString _sizeHandle;
    QScriptString _attributeValuesHandle;
    QScriptString _isLeafHandle;
    QScriptValueList _arguments;
    QScriptValue _getAttributesFunction;
    QScriptValue _visitFunction;
    QScriptValue _info;
    QScriptValue _minimum;
    
    MetavoxelVisitation* _visitation;
};

/// Contains the state associated with a visit to a metavoxel system.
class MetavoxelVisitation {
public:

    MetavoxelVisitor& visitor;
    QVector<MetavoxelNode*> nodes;
    MetavoxelInfo info;
    
    bool allNodesLeaves() const;
};

#endif /* defined(__interface__MetavoxelData__) */
