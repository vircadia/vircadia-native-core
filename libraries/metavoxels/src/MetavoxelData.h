//
//  MetavoxelData.h
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MetavoxelData_h
#define hifi_MetavoxelData_h

#include <QHash>
#include <QMutex>
#include <QSharedData>
#include <QSharedPointer>
#include <QVector>

#include <glm/glm.hpp>

#include "AttributeRegistry.h"
#include "MetavoxelUtil.h"

class MetavoxelInfo;
class MetavoxelNode;
class MetavoxelRendererImplementation;
class MetavoxelVisitation;
class MetavoxelVisitor;
class Spanner;

/// Determines whether to subdivide each node when traversing.  Contains the position (presumed to be of the viewer) and a
/// threshold value, where lower thresholds cause smaller/more distant voxels to be subdivided.
class MetavoxelLOD {
    STREAMABLE
    
public:
    STREAM glm::vec3 position;
    STREAM float threshold;
    
    MetavoxelLOD(const glm::vec3& position = glm::vec3(), float threshold = 0.0f);
    
    bool isValid() const { return threshold > 0.0f; }
    
    /// Checks whether, according to this LOD, we should subdivide the described voxel.
    bool shouldSubdivide(const glm::vec3& minimum, float size, float multiplier = 1.0f) const;
    
    /// Checks whether the node or any of the nodes underneath it have had subdivision enabled as compared to the reference.
    bool becameSubdivided(const glm::vec3& minimum, float size, const MetavoxelLOD& reference, float multiplier = 1.0f) const;
    
    /// Checks whether the node or any of the nodes underneath it have had subdivision
    /// enabled or disabled as compared to the reference.
    bool becameSubdividedOrCollapsed(const glm::vec3& minimum, float size,
        const MetavoxelLOD& reference, float multiplier = 1.0f) const;
};

DECLARE_STREAMABLE_METATYPE(MetavoxelLOD)

/// The base metavoxel representation shared between server and client.  Contains a size (for all dimensions) and a set of
/// octrees for different attributes.  
class MetavoxelData {
public:

    MetavoxelData();
    MetavoxelData(const MetavoxelData& other);
    ~MetavoxelData();

    MetavoxelData& operator=(const MetavoxelData& other);

    /// Sets the size in all dimensions.
    void setSize(float size) { _size = size; }
    float getSize() const { return _size; }

    /// Returns the minimum extent of the octrees (which are centered about the origin).
    glm::vec3 getMinimum() const { return glm::vec3(_size, _size, _size) * -0.5f; }

    /// Returns the bounds of the octrees.
    Box getBounds() const;

    /// Applies the specified visitor to the contained voxels.
    void guide(MetavoxelVisitor& visitor);
   
    /// Guides the specified visitor to the voxels that differ from those of the specified other.
    void guideToDifferent(const MetavoxelData& other, MetavoxelVisitor& visitor);
    
    /// Inserts a spanner into the specified attribute layer.
    void insert(const AttributePointer& attribute, const SharedObjectPointer& object);
    void insert(const AttributePointer& attribute, const Box& bounds, float granularity, const SharedObjectPointer& object);
    
    /// Removes a spanner from the specified attribute layer.
    void remove(const AttributePointer& attribute, const SharedObjectPointer& object);
    void remove(const AttributePointer& attribute, const Box& bounds, float granularity, const SharedObjectPointer& object);

    /// Toggles the existence of a spanner in the specified attribute layer (removes if present, adds if not).
    void toggle(const AttributePointer& attribute, const SharedObjectPointer& object);
    void toggle(const AttributePointer& attribute, const Box& bounds, float granularity, const SharedObjectPointer& object);

    /// Replaces a spanner in the specified attribute layer.
    void replace(const AttributePointer& attribute, const SharedObjectPointer& oldObject,
        const SharedObjectPointer& newObject);
    void replace(const AttributePointer& attribute, const Box& bounds, float granularity, const SharedObjectPointer& oldObject,
        const SharedObjectPointer& newObject);
    
    /// Clears all data in the specified attribute layer.
    void clear(const AttributePointer& attribute);

    /// "Touches" all data in the specified attribute layer, making it look as if it has changed.
    void touch(const AttributePointer& attribute);

    /// Convenience function that finds the first spanner intersecting the provided ray.    
    SharedObjectPointer findFirstRaySpannerIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const AttributePointer& attribute, float& distance, const MetavoxelLOD& lod = MetavoxelLOD());

    /// Sets part of the data.
    void set(const glm::vec3& minimum, const MetavoxelData& data, bool blend = false);

    /// Expands the tree, doubling its size in all dimensions (that is, increasing its volume eightfold).
    void expand();

    void read(Bitstream& in, const MetavoxelLOD& lod = MetavoxelLOD());
    void write(Bitstream& out, const MetavoxelLOD& lod = MetavoxelLOD()) const;

    void readDelta(const MetavoxelData& reference, const MetavoxelLOD& referenceLOD, Bitstream& in, const MetavoxelLOD& lod);
    void writeDelta(const MetavoxelData& reference, const MetavoxelLOD& referenceLOD,
        Bitstream& out, const MetavoxelLOD& lod) const;

    void setRoot(const AttributePointer& attribute, MetavoxelNode* root);
    MetavoxelNode* getRoot(const AttributePointer& attribute) const { return _roots.value(attribute); }    
    MetavoxelNode* createRoot(const AttributePointer& attribute);

    /// Performs a deep comparison between this data and the specified other (as opposed to the == operator, which does a
    /// shallow comparison).
    bool deepEquals(const MetavoxelData& other, const MetavoxelLOD& lod = MetavoxelLOD()) const;

    /// Counts the nodes in the data.
    void countNodes(int& internalNodes, int& leaves, const MetavoxelLOD& lod = MetavoxelLOD()) const;

    void dumpStats(QDebug debug = QDebug(QtDebugMsg)) const;

    bool operator==(const MetavoxelData& other) const;
    bool operator!=(const MetavoxelData& other) const;

private:

    friend class MetavoxelVisitation;
   
    void incrementRootReferenceCounts();
    void decrementRootReferenceCounts();
    
    float _size;
    QHash<AttributePointer, MetavoxelNode*> _roots;
};

Bitstream& operator<<(Bitstream& out, const MetavoxelData& data);

Bitstream& operator>>(Bitstream& in, MetavoxelData& data);

template<> void Bitstream::writeDelta(const MetavoxelData& value, const MetavoxelData& reference);

template<> void Bitstream::readDelta(MetavoxelData& value, const MetavoxelData& reference);

Q_DECLARE_METATYPE(MetavoxelData)

/// Holds the base state used in streaming metavoxel data.
class MetavoxelStreamBase {
public:
    const AttributePointer& attribute;
    Bitstream& stream;
    const MetavoxelLOD& lod;
    const MetavoxelLOD& referenceLOD;
    int visit;
};

/// Holds the state used in streaming a metavoxel node.
class MetavoxelStreamState {
public:
    MetavoxelStreamBase& base;
    glm::vec3 minimum;
    float size;
    
    bool shouldSubdivide() const;
    bool shouldSubdivideReference() const;
    bool becameSubdivided() const;
    bool becameSubdividedOrCollapsed() const;
    
    void setMinimum(const glm::vec3& lastMinimum, int index);
};

/// A single node within a metavoxel layer.
class MetavoxelNode {
public:

    static const int CHILD_COUNT = 8;

    static int getOppositeChildIndex(int index);

    MetavoxelNode(const AttributeValue& attributeValue, const MetavoxelNode* copyChildren = NULL);
    MetavoxelNode(const AttributePointer& attribute, const MetavoxelNode* copy);
    
    void setAttributeValue(const AttributeValue& attributeValue);

    void blendAttributeValues(const AttributeValue& source, const AttributeValue& dest);

    AttributeValue getAttributeValue(const AttributePointer& attribute) const;
    void* getAttributeValue() const { return _attributeValue; }

    void mergeChildren(const AttributePointer& attribute, bool postRead = false);

    MetavoxelNode* getChild(int index) const { return _children[index]; }
    void setChild(int index, MetavoxelNode* child) { _children[index] = child; }

    bool isLeaf() const;

    void read(MetavoxelStreamState& state);
    void write(MetavoxelStreamState& state) const;

    void readDelta(const MetavoxelNode& reference, MetavoxelStreamState& state);
    void writeDelta(const MetavoxelNode& reference, MetavoxelStreamState& state) const;

    MetavoxelNode* readSubdivision(MetavoxelStreamState& state);
    void writeSubdivision(MetavoxelStreamState& state) const;

    void readSubdivided(MetavoxelStreamState& state, const MetavoxelStreamState& ancestorState, void* ancestorValue);
    void writeSubdivided(MetavoxelStreamState& state, const MetavoxelStreamState& ancestorState, void* ancestorValue) const;
    
    void writeSpanners(MetavoxelStreamState& state) const;
    void writeSpannerDelta(const MetavoxelNode& reference, MetavoxelStreamState& state) const;
    void writeSpannerSubdivision(MetavoxelStreamState& state) const;

    /// Increments the node's reference count.
    void incrementReferenceCount() { _referenceCount.ref(); }

    /// Decrements the node's reference count.  If the resulting reference count is zero, destroys the node
    /// and calls delete this.
    void decrementReferenceCount(const AttributePointer& attribute);

    void destroy(const AttributePointer& attribute);

    bool clearChildren(const AttributePointer& attribute);
    
    /// Performs a deep comparison between this and the specified other node.
    bool deepEquals(const AttributePointer& attribute, const MetavoxelNode& other,
        const glm::vec3& minimum, float size, const MetavoxelLOD& lod) const;

    /// Retrieves all spanners satisfying the LOD constraint, placing them in the provided set.
    void getSpanners(const AttributePointer& attribute, const glm::vec3& minimum,
        float size, const MetavoxelLOD& lod, SharedObjectSet& results) const;
    
    void countNodes(const AttributePointer& attribute, const glm::vec3& minimum,
        float size, const MetavoxelLOD& lod, int& internalNodes, int& leaves) const;
    
    MetavoxelNode* touch(const AttributePointer& attribute) const;
    
private:
    Q_DISABLE_COPY(MetavoxelNode)
    
    friend class MetavoxelVisitation;
    
    QAtomicInt _referenceCount;
    void* _attributeValue;
    MetavoxelNode* _children[CHILD_COUNT];
};

/// Contains information about a metavoxel (explicit or procedural).
class MetavoxelInfo {
public:
    
    MetavoxelInfo* parentInfo;
    glm::vec3 minimum; ///< the minimum extent of the area covered by the voxel
    float size; ///< the size of the voxel in all dimensions
    QVector<AttributeValue> inputValues;
    QVector<OwnedAttributeValue> outputValues;
    float lodBase;
    bool isLODLeaf;
    bool isLeaf;
    
    MetavoxelInfo(MetavoxelInfo* parentInfo, int inputValuesSize, int outputValuesSize);
    MetavoxelInfo();
    
    Box getBounds() const { return Box(minimum, minimum + glm::vec3(size, size, size)); }
    glm::vec3 getCenter() const { return minimum + glm::vec3(size, size, size) * 0.5f; }
};

/// Base class for visitors to metavoxels.
class MetavoxelVisitor {
public:
    
    /// Encodes a visitation order sequence for the children of a metavoxel.
    static int encodeOrder(int first, int second, int third, int fourth, int fifth, int sixth, int seventh, int eighth);
    
    /// Encodes a visitation order sequence that visits each child as sorted along the specified direction.
    static int encodeOrder(const glm::vec3& direction);
    
    /// Returns a random visitation order sequence.
    static int encodeRandomOrder();
    
    /// The default visitation order.
    static const int DEFAULT_ORDER;
    
    /// A special "order" that instructs the guide to stop recursion.
    static const int STOP_RECURSION;
    
    /// A special "order" that short-circuits the tour.
    static const int SHORT_CIRCUIT;
    
    /// A flag combined with an order that instructs us to return to visiting all nodes (rather than the different ones) for
    /// just this level.
    static const int ALL_NODES;
    
    /// A flag combined with an order that instructs us to return to visiting all nodes (rather than the different ones) for
    /// this level and all beneath it.
    static const int ALL_NODES_REST;
    
    MetavoxelVisitor(const QVector<AttributePointer>& inputs,
        const QVector<AttributePointer>& outputs = QVector<AttributePointer>(),
        const MetavoxelLOD& lod = MetavoxelLOD());
    virtual ~MetavoxelVisitor();
    
    /// Returns a reference to the list of input attributes desired.
    const QVector<AttributePointer>& getInputs() const { return _inputs; }
    
    /// Returns a reference to the list of output attributes provided.
    const QVector<AttributePointer>& getOutputs() const { return _outputs; }
    
    /// Returns a reference to the level of detail that will determine subdivision levels.
    const MetavoxelLOD& getLOD() const { return _lod; }
    
    void setLOD(const MetavoxelLOD& lod) { _lod = lod; }
    
    float getMinimumLODThresholdMultiplier() const { return _minimumLODThresholdMultiplier; }
    
    /// Prepares for a new tour of the metavoxel data.
    virtual void prepare(MetavoxelData* data);
    
    /// Visits a metavoxel.
    /// \param info the metavoxel data
    /// \return the encoded order in which to traverse the children, zero to stop recursion, or -1 to short-circuit the tour.
    /// If child traversal is requested, postVisit will be called after we return from traversing the children and have merged
    /// their values
    virtual int visit(MetavoxelInfo& info) = 0;

    /// Called after we have visited all of a metavoxel's children.
    /// \return whether or not any outputs were set in the info
    virtual bool postVisit(MetavoxelInfo& info);

    /// Acquires the next visitation, incrementing the depth.
    MetavoxelVisitation& acquireVisitation();

    /// Releases the current visitation, decrementing the depth.
    void releaseVisitation() { _depth--; }

protected:

    QVector<AttributePointer> _inputs;
    QVector<AttributePointer> _outputs;
    MetavoxelLOD _lod;
    float _minimumLODThresholdMultiplier;
    MetavoxelData* _data;
    QList<MetavoxelVisitation> _visitations;
    int _depth;
};

/// Base class for visitors to spanners.
class SpannerVisitor : public MetavoxelVisitor {
public:
    
    SpannerVisitor(const QVector<AttributePointer>& spannerInputs,
        const QVector<AttributePointer>& inputs = QVector<AttributePointer>(),
        const QVector<AttributePointer>& outputs = QVector<AttributePointer>(),
        const MetavoxelLOD& lod = MetavoxelLOD(),
        int order = DEFAULT_ORDER);
    
    /// Visits a spanner.
    /// \return true to continue, false to short-circuit the tour
    virtual bool visit(Spanner* spanner) = 0;
    
    virtual void prepare(MetavoxelData* data);
    virtual int visit(MetavoxelInfo& info);

protected:
    
    int _spannerInputCount;
    int _order;
    int _visit;
};

/// Base class for ray intersection visitors.
class RayIntersectionVisitor : public MetavoxelVisitor {
public:
    
    RayIntersectionVisitor(const glm::vec3& origin, const glm::vec3& direction,
        const QVector<AttributePointer>& inputs,
        const QVector<AttributePointer>& outputs = QVector<AttributePointer>(),
        const MetavoxelLOD& lod = MetavoxelLOD());
    
    /// Visits a metavoxel that the ray intersects.
    virtual int visit(MetavoxelInfo& info, float distance) = 0;
    
    virtual int visit(MetavoxelInfo& info);
    
protected:
    
    glm::vec3 _origin;
    glm::vec3 _direction;
    int _order;
};

/// Base class for ray intersection spanner visitors.
class RaySpannerIntersectionVisitor : public RayIntersectionVisitor {
public:
    
    RaySpannerIntersectionVisitor(const glm::vec3& origin, const glm::vec3& direction,
        const QVector<AttributePointer>& spannerInputs,
        const QVector<AttributePointer>& inputs = QVector<AttributePointer>(),
        const QVector<AttributePointer>& outputs = QVector<AttributePointer>(),
        const MetavoxelLOD& lod = MetavoxelLOD());

    /// Visits a spanner that the ray intersects.
    /// \return true to continue, false to short-circuit the tour
    virtual bool visitSpanner(Spanner* spanner, float distance) = 0;
    
    virtual void prepare(MetavoxelData* data);
    virtual int visit(MetavoxelInfo& info, float distance);
    
protected:
    
    int _spannerInputCount;
    int _visit;
};

/// Interface for objects that guide metavoxel visitors.
class MetavoxelGuide : public SharedObject {
    Q_OBJECT
    
public:
    
    /// Guides the specified visitor to the contained voxels.
    /// \return true to keep going, false to short circuit the tour
    virtual bool guide(MetavoxelVisitation& visitation) = 0;
    
    /// Guides the specified visitor to the voxels that differ from a reference.
    /// \return true to keep going, false to short circuit the tour
    virtual bool guideToDifferent(MetavoxelVisitation& visitation);
};

/// Guides visitors through the explicit content of the system.
class DefaultMetavoxelGuide : public MetavoxelGuide {
    Q_OBJECT    
    
public:
    
    Q_INVOKABLE DefaultMetavoxelGuide();
    
    virtual bool guide(MetavoxelVisitation& visitation);
    virtual bool guideToDifferent(MetavoxelVisitation& visitation);
};

/// Contains the state associated with a visit to a metavoxel system.
class MetavoxelVisitation {
public:

    MetavoxelVisitation* previous;
    MetavoxelVisitor* visitor;
    QVector<MetavoxelNode*> inputNodes;
    QVector<MetavoxelNode*> outputNodes;
    QVector<MetavoxelNode*> compareNodes;
    MetavoxelInfo info;
    
    MetavoxelVisitation(MetavoxelVisitation* previous, MetavoxelVisitor* visitor, int inputNodesSize, int outputNodesSize);
    MetavoxelVisitation();
    
    bool isInputLeaf(int index) const;
    bool allInputNodesLeaves() const;
    AttributeValue getInheritedOutputValue(int index) const;
};

/// Base class for objects that render metavoxels.
class MetavoxelRenderer : public SharedObject {
    Q_OBJECT

public:

    MetavoxelRenderer();

    /// Returns a pointer to the implementation, creating it if necessary.
    MetavoxelRendererImplementation* getImplementation();

protected:

    MetavoxelRendererImplementation* _implementation;
    QMutex _implementationMutex;
    
    /// Returns the name of the class to instantiate for the implementation.
    virtual QByteArray getImplementationClassName() const;
};

/// Base class for renderer implementations.
class MetavoxelRendererImplementation : public SharedObject {
    Q_OBJECT

public:
    
    Q_INVOKABLE MetavoxelRendererImplementation();
    
    virtual void init(MetavoxelRenderer* renderer);
    virtual void augment(MetavoxelData& data, const MetavoxelData& previous, MetavoxelInfo& info, const MetavoxelLOD& lod);
    virtual void simulate(MetavoxelData& data, float deltaTime, MetavoxelInfo& info, const MetavoxelLOD& lod);
    virtual void render(MetavoxelData& data, MetavoxelInfo& info, const MetavoxelLOD& lod);

protected:
    
    MetavoxelRenderer* _renderer;
};

/// The standard, usual renderer.
class DefaultMetavoxelRenderer : public MetavoxelRenderer {
    Q_OBJECT

public:
    
    Q_INVOKABLE DefaultMetavoxelRenderer();
    
    virtual QByteArray getImplementationClassName() const;
};

#endif // hifi_MetavoxelData_h
