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
#include <QSharedData>
#include <QSharedPointer>
#include <QScriptString>
#include <QScriptValue>
#include <QVector>

#include <glm/glm.hpp>

#include "AttributeRegistry.h"
#include "MetavoxelUtil.h"

class QScriptContext;

class MetavoxelNode;
class MetavoxelVisitation;
class MetavoxelVisitor;
class NetworkValue;
class Spanner;
class SpannerRenderer;

/// Determines whether to subdivide each node when traversing.
class MetavoxelLOD {
    STREAMABLE
    
public:
    STREAM glm::vec3 position;
    STREAM float threshold;
    
    MetavoxelLOD(const glm::vec3& position = glm::vec3(), float threshold = 0.0f);
    
    bool isValid() const { return threshold > 0.0f; }
    
    bool shouldSubdivide(const glm::vec3& minimum, float size) const;
};

DECLARE_STREAMABLE_METATYPE(MetavoxelLOD)

/// The base metavoxel representation shared between server and client.
class MetavoxelData {
public:

    MetavoxelData();
    MetavoxelData(const MetavoxelData& other);
    ~MetavoxelData();

    MetavoxelData& operator=(const MetavoxelData& other);

    float getSize() const { return _size; }

    glm::vec3 getMinimum() const { return glm::vec3(_size, _size, _size) * -0.5f; }

    Box getBounds() const;

    /// Applies the specified visitor to the contained voxels.
    void guide(MetavoxelVisitor& visitor);
   
    void insert(const AttributePointer& attribute, const SharedObjectPointer& object);
    void insert(const AttributePointer& attribute, const Box& bounds, float granularity, const SharedObjectPointer& object);
    
    void remove(const AttributePointer& attribute, const SharedObjectPointer& object);
    void remove(const AttributePointer& attribute, const Box& bounds, float granularity, const SharedObjectPointer& object);

    void toggle(const AttributePointer& attribute, const SharedObjectPointer& object);
    void toggle(const AttributePointer& attribute, const Box& bounds, float granularity, const SharedObjectPointer& object);

    void clear(const AttributePointer& attribute);

    /// Expands the tree, increasing its capacity in all dimensions.
    void expand();

    void read(Bitstream& in, const MetavoxelLOD& lod = MetavoxelLOD());
    void write(Bitstream& out, const MetavoxelLOD& lod = MetavoxelLOD()) const;

    void readDelta(const MetavoxelData& reference, const MetavoxelLOD& referenceLOD, Bitstream& in, const MetavoxelLOD& lod);
    void writeDelta(const MetavoxelData& reference, const MetavoxelLOD& referenceLOD,
        Bitstream& out, const MetavoxelLOD& lod) const;

    MetavoxelNode* createRoot(const AttributePointer& attribute);

private:

    friend class MetavoxelVisitation;
   
    void incrementRootReferenceCounts();
    void decrementRootReferenceCounts();
    
    float _size;
    QHash<AttributePointer, MetavoxelNode*> _roots;
};

/// Holds the state used in streaming metavoxel data.
class MetavoxelStreamState {
public:
    glm::vec3 minimum;
    float size;
    const AttributePointer& attribute;
    Bitstream& stream;
    const MetavoxelLOD& lod;
    const MetavoxelLOD& referenceLOD;
    
    bool shouldSubdivide() const { return lod.shouldSubdivide(minimum, size); }
    bool shouldSubdivideReference() const { return referenceLOD.shouldSubdivide(minimum, size); }
    
    void setMinimum(const glm::vec3& lastMinimum, int index);
};

/// A single node within a metavoxel layer.
class MetavoxelNode {
public:

    static const int CHILD_COUNT = 8;

    MetavoxelNode(const AttributeValue& attributeValue);
    MetavoxelNode(const AttributePointer& attribute, const MetavoxelNode* copy);
    
    void setAttributeValue(const AttributeValue& attributeValue);

    AttributeValue getAttributeValue(const AttributePointer& attribute) const;
    void* getAttributeValue() const { return _attributeValue; }

    void mergeChildren(const AttributePointer& attribute);

    MetavoxelNode* getChild(int index) const { return _children[index]; }
    void setChild(int index, MetavoxelNode* child) { _children[index] = child; }

    bool isLeaf() const;

    void read(MetavoxelStreamState& state);
    void write(MetavoxelStreamState& state) const;

    void readDelta(const MetavoxelNode& reference, MetavoxelStreamState& state);
    void writeDelta(const MetavoxelNode& reference, MetavoxelStreamState& state) const;

    void writeSpanners(MetavoxelStreamState& state) const;
    void writeSpannerDelta(const MetavoxelNode& reference, MetavoxelStreamState& state) const;

    /// Increments the node's reference count.
    void incrementReferenceCount() { _referenceCount++; }

    /// Decrements the node's reference count.  If the resulting reference count is zero, destroys the node
    /// and calls delete this.
    void decrementReferenceCount(const AttributePointer& attribute);

    void destroy(const AttributePointer& attribute);

private:
    Q_DISABLE_COPY(MetavoxelNode)
    
    friend class MetavoxelVisitation;
    
    void clearChildren(const AttributePointer& attribute);
    
    int _referenceCount;
    void* _attributeValue;
    MetavoxelNode* _children[CHILD_COUNT];
};

/// Contains information about a metavoxel (explicit or procedural).
class MetavoxelInfo {
public:
    
    glm::vec3 minimum; ///< the minimum extent of the area covered by the voxel
    float size; ///< the size of the voxel in all dimensions
    QVector<AttributeValue> inputValues;
    QVector<OwnedAttributeValue> outputValues;
    bool isLeaf;
    
    Box getBounds() const { return Box(minimum, minimum + glm::vec3(size, size, size)); }
};

/// Interface for visitors to metavoxels.
class MetavoxelVisitor {
public:
    
    MetavoxelVisitor(const QVector<AttributePointer>& inputs,
        const QVector<AttributePointer>& outputs = QVector<AttributePointer>());
    virtual ~MetavoxelVisitor();
    
    /// Returns a reference to the list of input attributes desired.
    const QVector<AttributePointer>& getInputs() const { return _inputs; }
    
    /// Returns a reference to the list of output attributes provided.
    const QVector<AttributePointer>& getOutputs() const { return _outputs; }
    
    /// Prepares for a new tour of the metavoxel data.
    virtual void prepare();
    
    /// Visits a metavoxel.
    /// \param info the metavoxel data
    /// \return if true, continue descending; if false, stop
    virtual bool visit(MetavoxelInfo& info) = 0;

protected:

    QVector<AttributePointer> _inputs;
    QVector<AttributePointer> _outputs;
};

typedef QSharedPointer<MetavoxelVisitor> MetavoxelVisitorPointer;

/// Interface for visitors to spanners.
class SpannerVisitor : public MetavoxelVisitor {
public:
    
    SpannerVisitor(const QVector<AttributePointer>& spannerInputs,
        const QVector<AttributePointer>& inputs = QVector<AttributePointer>(),
        const QVector<AttributePointer>& outputs = QVector<AttributePointer>());
    
    /// Visits a spanner.
    virtual void visit(Spanner* spanner) = 0;
    
    virtual void prepare();
    virtual bool visit(MetavoxelInfo& info);

protected:
    
    int _spannerInputCount;
};

/// Interface for objects that guide metavoxel visitors.
class MetavoxelGuide : public SharedObject {
    Q_OBJECT
    
public:
    
    /// Guides the specified visitor to the contained voxels.
    virtual void guide(MetavoxelVisitation& visitation) = 0;
};

/// Guides visitors through the explicit content of the system.
class DefaultMetavoxelGuide : public MetavoxelGuide {
    Q_OBJECT    
    
public:
    
    Q_INVOKABLE DefaultMetavoxelGuide();
    
    virtual void guide(MetavoxelVisitation& visitation);
};

/// A temporary test guide that just makes the existing voxels throb with delight.
class ThrobbingMetavoxelGuide : public DefaultMetavoxelGuide {
    Q_OBJECT
    Q_PROPERTY(float rate MEMBER _rate)

public:
    
    Q_INVOKABLE ThrobbingMetavoxelGuide();
    
    virtual void guide(MetavoxelVisitation& visitation);
    
private:
    
    float _rate;
};

/// Represents a guide implemented in Javascript.
class ScriptedMetavoxelGuide : public DefaultMetavoxelGuide {
    Q_OBJECT
    Q_PROPERTY(ParameterizedURL url MEMBER _url WRITE setURL)
    
public:

    Q_INVOKABLE ScriptedMetavoxelGuide();

    virtual void guide(MetavoxelVisitation& visitation);

public slots:

    void setURL(const ParameterizedURL& url);
    
private:

    static QScriptValue getInputs(QScriptContext* context, QScriptEngine* engine);
    static QScriptValue getOutputs(QScriptContext* context, QScriptEngine* engine);
    static QScriptValue visit(QScriptContext* context, QScriptEngine* engine);

    ParameterizedURL _url;

    QSharedPointer<NetworkValue> _guideFunction;

    QScriptString _minimumHandle;
    QScriptString _sizeHandle;
    QScriptString _inputValuesHandle;
    QScriptString _outputValuesHandle;
    QScriptString _isLeafHandle;
    QScriptValueList _arguments;
    QScriptValue _getInputsFunction;
    QScriptValue _getOutputsFunction;
    QScriptValue _visitFunction;
    QScriptValue _info;
    QScriptValue _minimum;
    
    MetavoxelVisitation* _visitation;
};

/// Contains the state associated with a visit to a metavoxel system.
class MetavoxelVisitation {
public:

    MetavoxelVisitation* previous;
    MetavoxelVisitor& visitor;
    QVector<MetavoxelNode*> inputNodes;
    QVector<MetavoxelNode*> outputNodes;
    MetavoxelInfo info;
    
    bool allInputNodesLeaves() const;
    AttributeValue getInheritedOutputValue(int index) const;
};

/// An object that spans multiple octree cells.
class Spanner : public SharedObject {
    Q_OBJECT
    Q_PROPERTY(Box bounds MEMBER _bounds WRITE setBounds NOTIFY boundsChanged DESIGNABLE false)
    Q_PROPERTY(float granularity MEMBER _granularity DESIGNABLE false)

public:
    
    /// Increments the value of the global visit counter.
    static void incrementVisit() { _visit++; }
    
    Spanner();
    
    void setBounds(const Box& bounds);
    const Box& getBounds() const { return _bounds; }
    
    void setGranularity(float granularity) { _granularity = granularity; }
    float getGranularity() const { return _granularity; }
    
    /// Checks whether we've visited this object on the current traversal.  If we have, returns false.
    /// If we haven't, sets the last visit identifier and returns true.
    bool testAndSetVisited();

    /// Returns a pointer to the renderer, creating it if necessary.
    SpannerRenderer* getRenderer();

signals:

    void boundsWillChange();
    void boundsChanged(const Box& bounds);

protected:
    
    /// Returns the name of the class to instantiate in order to render this spanner.
    virtual QByteArray getRendererClassName() const;

private:
    
    Box _bounds;
    float _granularity;
    int _lastVisit; ///< the identifier of the last visit
    SpannerRenderer* _renderer;
    
    static int _visit; ///< the global visit counter
};

/// Base class for objects that can render spanners.
class SpannerRenderer : public QObject {
    Q_OBJECT
    
public:
    
    Q_INVOKABLE SpannerRenderer();
    
    virtual void init(Spanner* spanner);
    virtual void simulate(float deltaTime);
    virtual void render(float alpha);
};

/// An object with a 3D transform.
class Transformable : public Spanner {
    Q_OBJECT
    Q_PROPERTY(glm::vec3 translation MEMBER _translation WRITE setTranslation NOTIFY translationChanged)
    Q_PROPERTY(glm::vec3 rotation MEMBER _rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(float scale MEMBER _scale WRITE setScale NOTIFY scaleChanged)

public:

    Transformable();

    void setTranslation(const glm::vec3& translation);
    const glm::vec3& getTranslation() const { return _translation; }
    
    void setRotation(const glm::vec3& rotation);
    const glm::vec3& getRotation() const { return _rotation; }
    
    void setScale(float scale);
    float getScale() const { return _scale; }

signals:

    void translationChanged(const glm::vec3& translation);
    void rotationChanged(const glm::vec3& rotation);
    void scaleChanged(float scale);

private:
    
    glm::vec3 _translation;
    glm::vec3 _rotation;
    float _scale;
};

/// A sphere.
class Sphere : public Transformable {
    Q_OBJECT
    Q_PROPERTY(QColor color MEMBER _color WRITE setColor NOTIFY colorChanged)

public:
    
    Q_INVOKABLE Sphere();

    void setColor(const QColor& color);
    const QColor& getColor() const { return _color; }

signals:

    void colorChanged(const QColor& color);

protected:
    
    virtual QByteArray getRendererClassName() const;

private slots:
    
    void updateBounds();
    
private:
    
    QColor _color;
};

/// A static 3D model loaded from the network.
class StaticModel : public Transformable {
    Q_OBJECT
    Q_PROPERTY(QUrl url MEMBER _url WRITE setURL NOTIFY urlChanged)

public:
    
    Q_INVOKABLE StaticModel();

    void setURL(const QUrl& url);
    const QUrl& getURL() const { return _url; }

signals:

    void urlChanged(const QUrl& url);

protected:
    
    virtual QByteArray getRendererClassName() const;
    
private:
    
    QUrl _url;
};

#endif /* defined(__interface__MetavoxelData__) */
