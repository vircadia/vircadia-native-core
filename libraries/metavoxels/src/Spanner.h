//
//  Spanner.h
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 11/10/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Spanner_h
#define hifi_Spanner_h

#include <glm/glm.hpp>

#include "AttributeRegistry.h"
#include "MetavoxelUtil.h"

class AbstractHeightfieldNodeRenderer;
class DataBlock;
class Heightfield;
class HeightfieldColor;
class HeightfieldHeight;
class HeightfieldMaterial;
class HeightfieldNode;
class HeightfieldStack;
class SpannerRenderer;

/// An object that spans multiple octree cells.
class Spanner : public SharedObject {
    Q_OBJECT
    Q_PROPERTY(Box bounds MEMBER _bounds WRITE setBounds NOTIFY boundsChanged DESIGNABLE false)
    Q_PROPERTY(float placementGranularity MEMBER _placementGranularity DESIGNABLE false)
    Q_PROPERTY(float voxelizationGranularity MEMBER _voxelizationGranularity DESIGNABLE false)
    Q_PROPERTY(bool willBeVoxelized MEMBER _willBeVoxelized DESIGNABLE false)
    
public:
    
    /// Returns the value of the global visit counter and increments it.
    static int getAndIncrementNextVisit() { return _nextVisit.fetchAndAddOrdered(1); }
    
    Spanner();
    
    void setBounds(const Box& bounds);
    const Box& getBounds() const { return _bounds; }
    
    void setPlacementGranularity(float granularity) { _placementGranularity = granularity; }
    float getPlacementGranularity() const { return _placementGranularity; }
    
    void setVoxelizationGranularity(float granularity) { _voxelizationGranularity = granularity; }
    float getVoxelizationGranularity() const { return _voxelizationGranularity; }
    
    void setMerged(bool merged) { _merged = merged; }
    bool isMerged() const { return _merged; }
    
    void setWillBeVoxelized(bool willBeVoxelized) { _willBeVoxelized = willBeVoxelized; }
    bool getWillBeVoxelized() const { return _willBeVoxelized; }
    
    /// Checks whether we've visited this object on the current traversal.  If we have, returns false.
    /// If we haven't, sets the last visit identifier and returns true.
    bool testAndSetVisited(int visit);

    /// Returns a pointer to the renderer, creating it if necessary.
    SpannerRenderer* getRenderer();

    /// Checks whether this is a heightfield.
    virtual bool isHeightfield() const;

    /// Finds the height at the specified location, or returns -FLT_MAX for none.
    virtual float getHeight(const glm::vec3& location) const;
    
    /// Finds the intersection between the described ray and this spanner.
    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const;

    /// Attempts to modify the spanner's height.
    /// \param set whether to set the height as opposed to raising/lowering it
    /// \param erase whether to erase height values
    /// \return the modified spanner, or this if no modification was performed
    virtual Spanner* paintHeight(const glm::vec3& position, float radius, float height,
        bool set, bool erase, float granularity);

    /// Attempts to fill the spanner's height (adding removing volumetric information).
    /// \return the modified spanner, or this if no modification was performed
    virtual Spanner* fillHeight(const glm::vec3& position, float radius, float granularity);

    /// Attempts to "sculpt" or "paint," etc., with the supplied spanner.
    /// \return the modified spanner, or this if no modification was performed
    virtual Spanner* setMaterial(const SharedObjectPointer& spanner, const SharedObjectPointer& material,
        const QColor& color, bool paint, bool voxelize, float granularity);

    /// Checks whether this spanner has its own colors.
    virtual bool hasOwnColors() const;

    /// Checks whether this spanner has its own materials.
    virtual bool hasOwnMaterials() const;

    /// Checks whether the spanner contains the specified point.
    virtual bool contains(const glm::vec3& point);

    /// Retrieves the color at the specified point.
    virtual QRgb getColorAt(const glm::vec3& point);
    
    /// Retrieves the material at the specified point.
    virtual int getMaterialAt(const glm::vec3& point);

    /// Retrieves a reference to the list of materials.
    virtual QVector<SharedObjectPointer>& getMaterials();

    /// Finds the intersection, if any, between the specified line segment and the spanner.
    virtual bool intersects(const glm::vec3& start, const glm::vec3& end, float& distance, glm::vec3& normal);

signals:

    void boundsWillChange();
    void boundsChanged(const Box& bounds);

protected:
    
    SpannerRenderer* _renderer;
    
    /// Returns the name of the class to instantiate in order to render this spanner.
    virtual QByteArray getRendererClassName() const;

private:
    
    Box _bounds;
    float _placementGranularity;
    float _voxelizationGranularity;
    bool _merged;
    bool _willBeVoxelized;
    QHash<QThread*, int> _lastVisits; ///< last visit identifiers for each thread
    QMutex _lastVisitsMutex;
    
    static QAtomicInt _nextVisit; ///< the global visit counter
};

/// Base class for objects that can render spanners.
class SpannerRenderer : public QObject {
    Q_OBJECT
    
public:
    
    Q_INVOKABLE SpannerRenderer();
    
    virtual void init(Spanner* spanner);
    virtual void simulate(float deltaTime);
    virtual void render(const MetavoxelLOD& lod = MetavoxelLOD(), bool contained = false, bool cursor = false);
    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const;

protected:
    
    Spanner* _spanner;
};

/// An object with a 3D transform.
class Transformable : public Spanner {
    Q_OBJECT
    Q_PROPERTY(glm::vec3 translation MEMBER _translation WRITE setTranslation NOTIFY translationChanged)
    Q_PROPERTY(glm::quat rotation MEMBER _rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(float scale MEMBER _scale WRITE setScale NOTIFY scaleChanged)

public:

    Transformable();

    void setTranslation(const glm::vec3& translation);
    const glm::vec3& getTranslation() const { return _translation; }
    
    void setRotation(const glm::quat& rotation);
    const glm::quat& getRotation() const { return _rotation; }
    
    void setScale(float scale);
    float getScale() const { return _scale; }

signals:

    void translationChanged(const glm::vec3& translation);
    void rotationChanged(const glm::quat& rotation);
    void scaleChanged(float scale);

private:
    
    glm::vec3 _translation;
    glm::quat _rotation;
    float _scale;
};

/// A transformable object with a color.
class ColorTransformable : public Transformable {
    Q_OBJECT
    Q_PROPERTY(QColor color MEMBER _color WRITE setColor NOTIFY colorChanged DESIGNABLE false)
    
public:
   
    ColorTransformable();
    
    void setColor(const QColor& color);
    const QColor& getColor() const { return _color; }

signals:

    void colorChanged(const QColor& color);
    
protected:
    
    QColor _color;
};

/// A sphere.
class Sphere : public ColorTransformable {
    Q_OBJECT
    
public:
    
    Q_INVOKABLE Sphere();

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const;
    virtual bool contains(const glm::vec3& point);
    virtual bool intersects(const glm::vec3& start, const glm::vec3& end, float& distance, glm::vec3& normal);
    
protected:
    
    virtual QByteArray getRendererClassName() const;

private slots:
    
    void updateBounds();
};

/// A cuboid.
class Cuboid : public ColorTransformable {
    Q_OBJECT
    Q_PROPERTY(float aspectY MEMBER _aspectY WRITE setAspectY NOTIFY aspectYChanged)
    Q_PROPERTY(float aspectZ MEMBER _aspectZ WRITE setAspectZ NOTIFY aspectZChanged)
    
public:
    
    Q_INVOKABLE Cuboid();

    void setAspectY(float aspectY);
    float getAspectY() const { return _aspectY; }
    
    void setAspectZ(float aspectZ);
    float getAspectZ() const { return _aspectZ; }

    virtual bool contains(const glm::vec3& point);
    virtual bool intersects(const glm::vec3& start, const glm::vec3& end, float& distance, glm::vec3& normal);

signals:

    void aspectYChanged(float aspectY);
    void aspectZChanged(float aspectZ);

protected:
    
    virtual QByteArray getRendererClassName() const;

private slots:
    
    void updateBoundsAndPlanes();
    
private:

    float _aspectY;
    float _aspectZ;
    
    static const int PLANE_COUNT = 6;
    glm::vec4 _planes[PLANE_COUNT];
};

/// A static 3D model loaded from the network.
class StaticModel : public Transformable {
    Q_OBJECT
    Q_PROPERTY(QUrl url MEMBER _url WRITE setURL NOTIFY urlChanged)

public:
    
    Q_INVOKABLE StaticModel();

    void setURL(const QUrl& url);
    const QUrl& getURL() const { return _url; }

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const;
    
signals:

    void urlChanged(const QUrl& url);

protected:
    
    virtual QByteArray getRendererClassName() const;
    
private:
    
    QUrl _url;
};

typedef QExplicitlySharedDataPointer<DataBlock> DataBlockPointer;

/// Base class for blocks of data.
class DataBlock : public QSharedData {
public:

    static const int COLOR_BYTES = 3;
    
    virtual ~DataBlock();
    
    void setDeltaData(const DataBlockPointer& deltaData) { _deltaData = deltaData; }
    const DataBlockPointer& getDeltaData() const { return _deltaData; }
    
    void setEncodedDelta(const QByteArray& encodedDelta) { _encodedDelta = encodedDelta; }
    const QByteArray& getEncodedDelta() const { return _encodedDelta; }
    
    QMutex& getEncodedDeltaMutex() { return _encodedDeltaMutex; }

protected:
    
    QByteArray _encoded;
    QMutex _encodedMutex;
    
    DataBlockPointer _deltaData;
    QByteArray _encodedDelta;
    QMutex _encodedDeltaMutex;

    class EncodedSubdivision {
    public:
        DataBlockPointer ancestor;
        QByteArray data;
    };
    QVector<EncodedSubdivision> _encodedSubdivisions;
    QMutex _encodedSubdivisionsMutex;
};

/// Base class for heightfield data blocks.
class HeightfieldData : public DataBlock {
public:
    
    static const int SHARED_EDGE;
    
    HeightfieldData(int width = 0);
    
    int getWidth() const { return _width; }
    
protected:
    
    int _width;
};

typedef QExplicitlySharedDataPointer<HeightfieldHeight> HeightfieldHeightPointer;

/// A block of height data associated with a heightfield.
class HeightfieldHeight : public HeightfieldData {
public:

    static const int HEIGHT_BORDER;
    static const int HEIGHT_EXTENSION;
    
    HeightfieldHeight(int width, const QVector<quint16>& contents);
    HeightfieldHeight(Bitstream& in, int bytes);
    HeightfieldHeight(Bitstream& in, int bytes, const HeightfieldHeightPointer& reference);
    
    QVector<quint16>& getContents() { return _contents; }

    void write(Bitstream& out);
    void writeDelta(Bitstream& out, const HeightfieldHeightPointer& reference);
    
private:
    
    void read(Bitstream& in, int bytes);
    
    QVector<quint16> _contents;
};

Q_DECLARE_METATYPE(HeightfieldHeightPointer)

Bitstream& operator<<(Bitstream& out, const HeightfieldHeightPointer& value);
Bitstream& operator>>(Bitstream& in, HeightfieldHeightPointer& value);

template<> void Bitstream::writeRawDelta(const HeightfieldHeightPointer& value, const HeightfieldHeightPointer& reference);
template<> void Bitstream::readRawDelta(HeightfieldHeightPointer& value, const HeightfieldHeightPointer& reference);

/// Allows editing heightfield height blocks.
class HeightfieldHeightEditor : public QWidget {
    Q_OBJECT
    Q_PROPERTY(HeightfieldHeightPointer height MEMBER _height WRITE setHeight NOTIFY heightChanged USER true)
    
public:
    
    HeightfieldHeightEditor(QWidget* parent = NULL);
    
    const HeightfieldHeightPointer& getHeight() const { return _height; }
    
signals:

    void heightChanged(const HeightfieldHeightPointer& height);

public slots:

    void setHeight(const HeightfieldHeightPointer& height);

private slots:
    
    void select();
    void clear();    
    
private:
    
    HeightfieldHeightPointer _height;
    
    QPushButton* _select;
    QPushButton* _clear;
};

typedef QExplicitlySharedDataPointer<HeightfieldColor> HeightfieldColorPointer;

/// A block of color data associated with a heightfield.
class HeightfieldColor : public HeightfieldData {
public:
    
    HeightfieldColor(int width, const QByteArray& contents);
    HeightfieldColor(Bitstream& in, int bytes);
    HeightfieldColor(Bitstream& in, int bytes, const HeightfieldColorPointer& reference);
    
    QByteArray& getContents() { return _contents; }
    
    void write(Bitstream& out);
    void writeDelta(Bitstream& out, const HeightfieldColorPointer& reference);
    
private:
    
    void read(Bitstream& in, int bytes);
    
    QByteArray _contents;
};

Q_DECLARE_METATYPE(HeightfieldColorPointer)

Bitstream& operator<<(Bitstream& out, const HeightfieldColorPointer& value);
Bitstream& operator>>(Bitstream& in, HeightfieldColorPointer& value);

template<> void Bitstream::writeRawDelta(const HeightfieldColorPointer& value, const HeightfieldColorPointer& reference);
template<> void Bitstream::readRawDelta(HeightfieldColorPointer& value, const HeightfieldColorPointer& reference);

/// Allows editing heightfield color blocks.
class HeightfieldColorEditor : public QWidget {
    Q_OBJECT
    Q_PROPERTY(HeightfieldColorPointer color MEMBER _color WRITE setColor NOTIFY colorChanged USER true)
    
public:
    
    HeightfieldColorEditor(QWidget* parent = NULL);
    
    const HeightfieldColorPointer& getColor() const { return _color; }
    
signals:

    void colorChanged(const HeightfieldColorPointer& color);

public slots:

    void setColor(const HeightfieldColorPointer& color);

private slots:
    
    void select();
    void clear();
    
private:
    
    HeightfieldColorPointer _color;
    
    QPushButton* _select;
    QPushButton* _clear;
};

typedef QExplicitlySharedDataPointer<HeightfieldMaterial> HeightfieldMaterialPointer;

/// A block of material data associated with a heightfield.
class HeightfieldMaterial : public HeightfieldData {
public:
    
    HeightfieldMaterial(int width, const QByteArray& contents, const QVector<SharedObjectPointer>& materials);
    HeightfieldMaterial(Bitstream& in, int bytes);
    HeightfieldMaterial(Bitstream& in, int bytes, const HeightfieldMaterialPointer& reference);
    
    QByteArray& getContents() { return _contents; }
    QVector<SharedObjectPointer>& getMaterials() { return _materials; }
    
    void write(Bitstream& out);
    void writeDelta(Bitstream& out, const HeightfieldMaterialPointer& reference);
    
private:
    
    void read(Bitstream& in, int bytes);
    
    QByteArray _contents;
    QVector<SharedObjectPointer> _materials;
};

Q_DECLARE_METATYPE(HeightfieldMaterialPointer)

Bitstream& operator<<(Bitstream& out, const HeightfieldMaterialPointer& value);
Bitstream& operator>>(Bitstream& in, HeightfieldMaterialPointer& value);

template<> void Bitstream::writeRawDelta(const HeightfieldMaterialPointer& value, const HeightfieldMaterialPointer& reference);
template<> void Bitstream::readRawDelta(HeightfieldMaterialPointer& value, const HeightfieldMaterialPointer& reference);

/// Contains the description of a material.
class MaterialObject : public SharedObject {
    Q_OBJECT
    Q_PROPERTY(QUrl diffuse MEMBER _diffuse)
    Q_PROPERTY(float scaleS MEMBER _scaleS)
    Q_PROPERTY(float scaleT MEMBER _scaleT)
    
public:
    
    Q_INVOKABLE MaterialObject();

    const QUrl& getDiffuse() const { return _diffuse; }

    float getScaleS() const { return _scaleS; }
    float getScaleT() const { return _scaleT; }

private:
    
    QUrl _diffuse;
    float _scaleS;
    float _scaleT;
};

/// Finds a material index for the supplied material in the provided list, adding an entry if necessary.  Returns -1
/// on failure (no room to add new material).
int getMaterialIndex(const SharedObjectPointer& material, QVector<SharedObjectPointer>& materials);

typedef QExplicitlySharedDataPointer<HeightfieldStack> HeightfieldStackPointer;

/// A single column within a stack block.
class StackArray : public QByteArray {
public:
    
#pragma pack(push, 1)
    /// A single entry within the array.
    class Entry {
    public:
        quint32 color;
        uchar material;
        quint32 hermiteX;
        quint32 hermiteY;
        quint32 hermiteZ;
        
        Entry();
        
        bool isSet() const { return qAlpha(color) != 0; }
        
        bool isZero() const;
        bool isMergeable(const Entry& other) const;
        
        void setHermiteX(const glm::vec3& normal, float position);
        float getHermiteX(glm::vec3& normal) const;
        
        void setHermiteY(const glm::vec3& normal, float position);
        float getHermiteY(glm::vec3& normal) const;
        
        void setHermiteZ(const glm::vec3& normal, float position);
        float getHermiteZ(glm::vec3& normal) const;
    };
#pragma pack(pop)

    static int getSize(int entries) { return (entries == 0) ? 0 : sizeof(quint16) + sizeof(Entry) * entries; }

    StackArray() : QByteArray() { }
    StackArray(int entries) : QByteArray(getSize(entries), 0) { }
    StackArray(const QByteArray& other) : QByteArray(other) { }
    StackArray(const char* src, int bytes) : QByteArray(src, bytes) { }
    
    int getPosition() const { return *(const quint16*)constData(); }
    void setPosition(int position) { *(quint16*)data() = position; }
    
    quint16& getPositionRef() { return *(quint16*)data(); }
    
    int getEntryCount() const { return isEmpty() ? 0 : (size() - sizeof(quint16)) / sizeof(Entry); }
    
    Entry* getEntryData() { return (Entry*)(data() + sizeof(quint16)); }
    const Entry* getEntryData() const { return (const Entry*)(constData() + sizeof(quint16)); }
    
    int getEntryAlpha(int y, float heightfieldHeight = 0.0f) const;
    
    Entry& getEntry(int y, float heightfieldHeight = 0.0f);
    const Entry& getEntry(int y, float heightfieldHeight = 0.0f) const;
    
    void getExtents(int& minimumY, int& maximumY) const;
    
    bool hasSetEntries() const;
    
    void removeEntries(int position, int count) { remove(sizeof(quint16) + position * sizeof(Entry), count * sizeof(Entry)); }
};

/// A block of stack data associated with a heightfield.
class HeightfieldStack : public HeightfieldData {
public:

    HeightfieldStack(int width, const QVector<StackArray>& contents, const QVector<SharedObjectPointer>& materials);
    HeightfieldStack(Bitstream& in, int bytes);
    HeightfieldStack(Bitstream& in, int bytes, const HeightfieldStackPointer& reference);
    
    QVector<StackArray>& getContents() { return _contents; }
    QVector<SharedObjectPointer>& getMaterials() { return _materials; }
    
    void write(Bitstream& out);
    void writeDelta(Bitstream& out, const HeightfieldStackPointer& reference);
    
private:
    
    void read(Bitstream& in, int bytes);
    
    QVector<StackArray> _contents;
    QVector<SharedObjectPointer> _materials;
};

Q_DECLARE_METATYPE(HeightfieldStackPointer)

Bitstream& operator<<(Bitstream& out, const HeightfieldStackPointer& value);
Bitstream& operator>>(Bitstream& in, HeightfieldStackPointer& value);

template<> void Bitstream::writeRawDelta(const HeightfieldStackPointer& value, const HeightfieldStackPointer& reference);
template<> void Bitstream::readRawDelta(HeightfieldStackPointer& value, const HeightfieldStackPointer& reference);

typedef QExplicitlySharedDataPointer<HeightfieldNode> HeightfieldNodePointer;

/// Holds the base state used in streaming heightfield data.
class HeightfieldStreamBase {
public:
    Bitstream& stream;
    const MetavoxelLOD& lod;
    const MetavoxelLOD& referenceLOD;
};

/// Holds the state used in streaming a heightfield node.
class HeightfieldStreamState {
public:
    HeightfieldStreamBase& base;
    glm::vec2 minimum;
    float size;
    
    bool shouldSubdivide() const;
    bool shouldSubdivideReference() const;
    bool becameSubdivided() const;
    bool becameSubdividedOrCollapsed() const;
    
    void setMinimum(const glm::vec2& lastMinimum, int index);
};

/// A node in a heightfield quadtree.
class HeightfieldNode : public QSharedData {
public:
    
    static const int CHILD_COUNT = 4;
    
    HeightfieldNode(const HeightfieldHeightPointer& height = HeightfieldHeightPointer(),
        const HeightfieldColorPointer& color = HeightfieldColorPointer(),
        const HeightfieldMaterialPointer& material = HeightfieldMaterialPointer(),
        const HeightfieldStackPointer& stack = HeightfieldStackPointer());
    
    HeightfieldNode(const HeightfieldNode& other);
    
    ~HeightfieldNode();
    
    void setContents(const HeightfieldHeightPointer& height, const HeightfieldColorPointer& color,
        const HeightfieldMaterialPointer& material, const HeightfieldStackPointer& stack);
    
    void setHeight(const HeightfieldHeightPointer& height) { _height = height; }
    const HeightfieldHeightPointer& getHeight() const { return _height; }
    
    void setColor(const HeightfieldColorPointer& color) { _color = color; }
    const HeightfieldColorPointer& getColor() const { return _color; }
    
    void setMaterial(const HeightfieldMaterialPointer& material) { _material = material; }
    const HeightfieldMaterialPointer& getMaterial() const { return _material; }
    
    void setStack(const HeightfieldStackPointer& stack) { _stack = stack; }
    const HeightfieldStackPointer& getStack() const { return _stack; }
    
    void setRenderer(AbstractHeightfieldNodeRenderer* renderer) { _renderer = renderer; }
    AbstractHeightfieldNodeRenderer* getRenderer() const { return _renderer; }
    
    bool isLeaf() const;
    
    void setChild(int index, const HeightfieldNodePointer& child) { _children[index] = child; }
    const HeightfieldNodePointer& getChild(int index) const { return _children[index]; }
    
    bool findRayIntersection(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale,
        const glm::vec3& origin, const glm::vec3& direction, float& distance) const;
    
    void getRangeAfterHeightPaint(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale,
        const glm::vec3& position, float radius, float height, float& minimum, float& maximum) const;
    
    HeightfieldNode* paintHeight(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale,
        const glm::vec3& position, float radius, float height, bool set, bool erase,
        float normalizeScale, float normalizeOffset, float granularity);
    
    HeightfieldNode* fillHeight(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale,
        const glm::vec3& position, float radius, float granularity);
    
    void getRangeAfterEdit(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale,
        const Box& editBounds, float& minimum, float& maximum) const;
    
    HeightfieldNode* setMaterial(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale,
        Spanner* spanner, const SharedObjectPointer& material, const QColor& color, bool paint, bool voxelize,
        float normalizeScale, float normalizeOffset, float granularity);
        
    void read(HeightfieldStreamState& state);
    void write(HeightfieldStreamState& state) const;
    
    void readDelta(const HeightfieldNodePointer& reference, HeightfieldStreamState& state);
    void writeDelta(const HeightfieldNodePointer& reference, HeightfieldStreamState& state) const;
    
    HeightfieldNode* readSubdivision(HeightfieldStreamState& state);
    void writeSubdivision(HeightfieldStreamState& state) const;
    
    void readSubdivided(HeightfieldStreamState& state, const HeightfieldStreamState& ancestorState,
        const HeightfieldNode* ancestor);
    void writeSubdivided(HeightfieldStreamState& state, const HeightfieldStreamState& ancestorState,
        const HeightfieldNode* ancestor) const;
    
private:
    
    void clearChildren();
    void mergeChildren(bool height = true, bool colorMaterial = true);
    
    QRgb getColorAt(const glm::vec3& location) const;
    int getMaterialAt(const glm::vec3& location) const;
    
    void maybeRenormalize(const glm::vec3& scale, float normalizeScale, float normalizeOffset, int innerStackWidth,
        QVector<quint16>& heightContents, QVector<StackArray>& stackContents);
    
    bool findHeightfieldRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        float boundsDistance, float& distance) const;
    
    HeightfieldNode* subdivide(const QVector<quint16>& heightContents, const QVector<StackArray>& stackContents) const;
    
    HeightfieldHeightPointer _height;
    HeightfieldColorPointer _color;
    HeightfieldMaterialPointer _material;
    HeightfieldStackPointer _stack;
    
    HeightfieldNodePointer _children[CHILD_COUNT];
    
    AbstractHeightfieldNodeRenderer* _renderer;
};

/// Base class for heightfield node rendering.
class AbstractHeightfieldNodeRenderer {
public:
    
    virtual ~AbstractHeightfieldNodeRenderer();
    
    virtual bool findRayIntersection(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale,
        const glm::vec3& origin, const glm::vec3& direction, float boundsDistance, float& distance) const; 
};

/// A heightfield represented as a spanner.
class Heightfield : public Transformable {
    Q_OBJECT
    Q_PROPERTY(float aspectY MEMBER _aspectY WRITE setAspectY NOTIFY aspectYChanged)
    Q_PROPERTY(float aspectZ MEMBER _aspectZ WRITE setAspectZ NOTIFY aspectZChanged)
    Q_PROPERTY(HeightfieldHeightPointer height MEMBER _height WRITE setHeight NOTIFY heightChanged STORED false)
    Q_PROPERTY(HeightfieldColorPointer color MEMBER _color WRITE setColor NOTIFY colorChanged STORED false)
    Q_PROPERTY(HeightfieldMaterialPointer material MEMBER _material WRITE setMaterial NOTIFY materialChanged STORED false
        DESIGNABLE false)
    Q_PROPERTY(HeightfieldStackPointer stack MEMBER _stack WRITE setStack NOTIFY stackChanged STORED false
        DESIGNABLE false)
        
public:
    
    Q_INVOKABLE Heightfield();

    void setAspectY(float aspectY);
    float getAspectY() const { return _aspectY; }
    
    void setAspectZ(float aspectZ);
    float getAspectZ() const { return _aspectZ; }

    void setHeight(const HeightfieldHeightPointer& height);
    const HeightfieldHeightPointer& getHeight() const { return _height; }
    
    void setColor(const HeightfieldColorPointer& color);
    const HeightfieldColorPointer& getColor() const { return _color; }
    
    void setMaterial(const HeightfieldMaterialPointer& material);
    const HeightfieldMaterialPointer& getMaterial() const { return _material; }

    void setStack(const HeightfieldStackPointer& stack);
    const HeightfieldStackPointer& getStack() const { return _stack; }
    
    void setRoot(const HeightfieldNodePointer& root) { _root = root; }
    const HeightfieldNodePointer& getRoot() const { return _root; }

    MetavoxelLOD transformLOD(const MetavoxelLOD& lod) const;

    virtual SharedObject* clone(bool withID = false, SharedObject* target = NULL) const;
    
    virtual bool isHeightfield() const;
    
    virtual float getHeight(const glm::vec3& location) const;

    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const;

    virtual Spanner* paintHeight(const glm::vec3& position, float radius, float height,
        bool set, bool erase, float granularity);
    
    virtual Spanner* fillHeight(const glm::vec3& position, float radius, float granularity);
    
    virtual Spanner* setMaterial(const SharedObjectPointer& spanner, const SharedObjectPointer& material,
        const QColor& color, bool paint, bool voxelize, float granularity);
        
    virtual bool hasOwnColors() const;
    virtual bool hasOwnMaterials() const;
    virtual QRgb getColorAt(const glm::vec3& point);
    virtual int getMaterialAt(const glm::vec3& point);
    virtual QVector<SharedObjectPointer>& getMaterials();
    
    virtual bool contains(const glm::vec3& point);
    virtual bool intersects(const glm::vec3& start, const glm::vec3& end, float& distance, glm::vec3& normal);
    
    virtual void writeExtra(Bitstream& out) const;
    virtual void readExtra(Bitstream& in, bool reread);
    virtual void writeExtraDelta(Bitstream& out, const SharedObject* reference) const;
    virtual void readExtraDelta(Bitstream& in, const SharedObject* reference, bool reread);
    virtual void maybeWriteSubdivision(Bitstream& out);
    virtual SharedObject* readSubdivision(Bitstream& in);
    
signals:

    void aspectYChanged(float aspectY);
    void aspectZChanged(float aspectZ);
    void heightChanged(const HeightfieldHeightPointer& height);
    void colorChanged(const HeightfieldColorPointer& color);
    void materialChanged(const HeightfieldMaterialPointer& material);
    void stackChanged(const HeightfieldStackPointer& stack);
    
protected:
    
    virtual QByteArray getRendererClassName() const;

private slots:
    
    void updateBounds();
    void updateRoot();
    
private:

    Heightfield* prepareEdit(float minimumValue, float maximumValue, float& normalizeScale, float& normalizeOffset);

    float _aspectY;
    float _aspectZ;
    
    HeightfieldHeightPointer _height;
    HeightfieldColorPointer _color;
    HeightfieldMaterialPointer _material;
    HeightfieldStackPointer _stack;
    
    HeightfieldNodePointer _root;
};

#endif // hifi_Spanner_h
