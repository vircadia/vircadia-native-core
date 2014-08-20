//
//  AttributeRegistry.h
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AttributeRegistry_h
#define hifi_AttributeRegistry_h

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QReadWriteLock>
#include <QSharedPointer>
#include <QString>
#include <QUrl>
#include <QWidget>

#include "Bitstream.h"
#include "SharedObject.h"

class QScriptContext;
class QScriptEngine;
class QScriptValue;

class Attribute;
class HeightfieldColorData;
class HeightfieldData;
class HeightfieldHeightData;
class HeightfieldTextureData;
class MetavoxelData;
class MetavoxelLOD;
class MetavoxelNode;
class MetavoxelStreamState;

typedef SharedObjectPointerTemplate<Attribute> AttributePointer;

Q_DECLARE_METATYPE(AttributePointer)

/// Maintains information about metavoxel attribute types.
class AttributeRegistry {
public:
    
    /// Returns a pointer to the singleton registry instance.
    static AttributeRegistry* getInstance();
    
    AttributeRegistry();
    
    /// Configures the supplied script engine with the global AttributeRegistry property.
    void configureScriptEngine(QScriptEngine* engine);
    
    /// Registers an attribute with the system.  The registry assumes ownership of the object.
    /// \return either the pointer passed as an argument, if the attribute wasn't already registered, or the existing
    /// attribute
    AttributePointer registerAttribute(Attribute* attribute) { return registerAttribute(AttributePointer(attribute)); }
    
    /// Registers an attribute with the system.
    /// \return either the pointer passed as an argument, if the attribute wasn't already registered, or the existing
    /// attribute
    AttributePointer registerAttribute(AttributePointer attribute);
    
    /// Deregisters an attribute.
    void deregisterAttribute(const QString& name);
    
    /// Retrieves an attribute by name.
    AttributePointer getAttribute(const QString& name);
    
    /// Returns a reference to the attribute hash.
    const QHash<QString, AttributePointer>& getAttributes() const { return _attributes; }
    
    /// Returns a reference to the attributes lock.
    QReadWriteLock& getAttributesLock() { return _attributesLock; }    
    
    /// Returns a reference to the standard SharedObjectPointer "guide" attribute.
    const AttributePointer& getGuideAttribute() const { return _guideAttribute; }
    
    /// Returns a reference to the standard SharedObjectPointer "renderer" attribute.
    const AttributePointer& getRendererAttribute() const { return _rendererAttribute; }
    
    /// Returns a reference to the standard SharedObjectSet "spanners" attribute.
    const AttributePointer& getSpannersAttribute() const { return _spannersAttribute; }
    
    /// Returns a reference to the standard QRgb "color" attribute.
    const AttributePointer& getColorAttribute() const { return _colorAttribute; }
    
    /// Returns a reference to the standard packed normal "normal" attribute.
    const AttributePointer& getNormalAttribute() const { return _normalAttribute; }
    
    /// Returns a reference to the standard QRgb "spannerColor" attribute.
    const AttributePointer& getSpannerColorAttribute() const { return _spannerColorAttribute; }
    
    /// Returns a reference to the standard packed normal "spannerNormal" attribute.
    const AttributePointer& getSpannerNormalAttribute() const { return _spannerNormalAttribute; }
    
    /// Returns a reference to the standard "spannerMask" attribute.
    const AttributePointer& getSpannerMaskAttribute() const { return _spannerMaskAttribute; }
    
    /// Returns a reference to the standard HeightfieldDataPointer "heightfield" attribute.
    const AttributePointer& getHeightfieldAttribute() const { return _heightfieldAttribute; }
    
    /// Returns a reference to the standard HeightfieldDataPointer "heightfieldColor" attribute.
    const AttributePointer& getHeightfieldColorAttribute() const { return _heightfieldColorAttribute; }
    
    /// Returns a reference to the standard HeightfieldDataPointer "heightfieldTexture" attribute.
    const AttributePointer& getHeightfieldTextureAttribute() const { return _heightfieldTextureAttribute; } 
    
private:

    static QScriptValue getAttribute(QScriptContext* context, QScriptEngine* engine);

    QHash<QString, AttributePointer> _attributes;
    QReadWriteLock _attributesLock;
    
    AttributePointer _guideAttribute;
    AttributePointer _rendererAttribute;
    AttributePointer _spannersAttribute;
    AttributePointer _colorAttribute;
    AttributePointer _normalAttribute;
    AttributePointer _spannerColorAttribute;
    AttributePointer _spannerNormalAttribute;
    AttributePointer _spannerMaskAttribute;
    AttributePointer _heightfieldAttribute;
    AttributePointer _heightfieldColorAttribute;
    AttributePointer _heightfieldTextureAttribute;
};

/// Converts a value to a void pointer.
template<class T> inline void* encodeInline(T value) {
    return *(void**)&value;
}

/// Extracts a value from a void pointer.
template<class T> inline T decodeInline(void* value) {
    return *(T*)&value;
}

/// Pairs an attribute value with its type.
class AttributeValue {
public:
    
    AttributeValue(const AttributePointer& attribute = AttributePointer());
    AttributeValue(const AttributePointer& attribute, void* value);
    
    AttributePointer getAttribute() const { return _attribute; }
    void* getValue() const { return _value; }
    
    template<class T> void setInlineValue(T value) { _value = encodeInline(value); }
    template<class T> T getInlineValue() const { return decodeInline<T>(_value); }
    
    void* copy() const;

    bool isDefault() const;

    bool operator==(const AttributeValue& other) const;
    bool operator==(void* other) const;
    
    bool operator!=(const AttributeValue& other) const;
    bool operator!=(void* other) const;
    
protected:
    
    AttributePointer _attribute;
    void* _value;
};

// Assumes ownership of an attribute value.
class OwnedAttributeValue : public AttributeValue {
public:
    
    /// Assumes ownership of the specified value.  It will be destroyed when this is destroyed or reassigned.
    OwnedAttributeValue(const AttributePointer& attribute, void* value);
    
    /// Creates an owned attribute with a copy of the specified attribute's default value.
    OwnedAttributeValue(const AttributePointer& attribute = AttributePointer());
    
    /// Creates an owned attribute with a copy of the specified other value.
    OwnedAttributeValue(const AttributeValue& other);
    
    /// Creates an owned attribute with a copy of the specified other value.
    OwnedAttributeValue(const OwnedAttributeValue& other);
    
    /// Destroys the current value, if any.
    ~OwnedAttributeValue();
    
    /// Sets this attribute to a mix of the first and second provided.    
    void mix(const AttributeValue& first, const AttributeValue& second, float alpha);

    /// Sets this attribute to a blend of the source and destination.
    void blend(const AttributeValue& source, const AttributeValue& dest);
    
    /// Destroys the current value, if any, and copies the specified other value.
    OwnedAttributeValue& operator=(const AttributeValue& other);
    
    /// Destroys the current value, if any, and copies the specified other value.
    OwnedAttributeValue& operator=(const OwnedAttributeValue& other);
};

Q_DECLARE_METATYPE(OwnedAttributeValue)

/// Represents a registered attribute.
class Attribute : public SharedObject {
    Q_OBJECT
    Q_PROPERTY(float lodThresholdMultiplier MEMBER _lodThresholdMultiplier)
    
public:
    
    static const int MERGE_COUNT = 8;
    
    Attribute(const QString& name);
    virtual ~Attribute();

    Q_INVOKABLE QString getName() const { return objectName(); }

    float getLODThresholdMultiplier() const { return _lodThresholdMultiplier; }
    void setLODThresholdMultiplier(float multiplier) { _lodThresholdMultiplier = multiplier; }

    void* create() const { return create(getDefaultValue()); }
    virtual void* create(void* copy) const = 0;
    virtual void destroy(void* value) const = 0;

    virtual void read(Bitstream& in, void*& value, bool isLeaf) const = 0;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const = 0;

    virtual void readDelta(Bitstream& in, void*& value, void* reference, bool isLeaf) const { read(in, value, isLeaf); }
    virtual void writeDelta(Bitstream& out, void* value, void* reference, bool isLeaf) const { write(out, value, isLeaf); }

    virtual void readSubdivided(MetavoxelStreamState& state, void*& value,
        const MetavoxelStreamState& ancestorState, void* ancestorValue, bool isLeaf) const;
    virtual void writeSubdivided(MetavoxelStreamState& state, void* value,
        const MetavoxelStreamState& ancestorState, void* ancestorValue, bool isLeaf) const;
    
    virtual MetavoxelNode* createMetavoxelNode(const AttributeValue& value, const MetavoxelNode* original) const;

    virtual void readMetavoxelRoot(MetavoxelData& data, MetavoxelStreamState& state);
    virtual void writeMetavoxelRoot(const MetavoxelNode& root, MetavoxelStreamState& state);
    
    virtual void readMetavoxelDelta(MetavoxelData& data, const MetavoxelNode& reference, MetavoxelStreamState& state);
    virtual void writeMetavoxelDelta(const MetavoxelNode& root, const MetavoxelNode& reference, MetavoxelStreamState& state);
    
    virtual void readMetavoxelSubdivision(MetavoxelData& data, MetavoxelStreamState& state);
    virtual void writeMetavoxelSubdivision(const MetavoxelNode& root, MetavoxelStreamState& state);

    virtual bool equal(void* first, void* second) const = 0;

    virtual bool deepEqual(void* first, void* second) const { return equal(first, second); }

    virtual bool metavoxelRootsEqual(const MetavoxelNode& firstRoot, const MetavoxelNode& secondRoot,
        const glm::vec3& minimum, float size, const MetavoxelLOD& lod);

    /// Expands the specified root, doubling its size in each dimension.
    /// \return a new node representing the result
    virtual MetavoxelNode* expandMetavoxelRoot(const MetavoxelNode& root);

    /// Merges the value of a parent and its children.
    /// \param postRead whether or not the merge is happening after a read
    /// \return whether or not the children and parent values are all equal
    virtual bool merge(void*& parent, void* children[], bool postRead = false) const = 0;

    /// Given the parent value, returns the value that children should inherit (either the parent value or the default).
    virtual AttributeValue inherit(const AttributeValue& parentValue) const { return parentValue; }

    /// Mixes the first and the second, returning a new value with the result.
    virtual void* mix(void* first, void* second, float alpha) const = 0;

    /// Blends the source with the destination, returning a new value with the result.
    virtual void* blend(void* source, void* dest) const = 0;

    virtual void* getDefaultValue() const = 0;

    virtual void* createFromScript(const QScriptValue& value, QScriptEngine* engine) const { return create(); }
    
    virtual void* createFromVariant(const QVariant& value) const { return create(); }
    
    /// Creates a widget to use to edit values of this attribute, or returns NULL if the attribute isn't editable.
    /// The widget should have a single "user" property that will be used to get/set the value.
    virtual QWidget* createEditor(QWidget* parent = NULL) const { return NULL; }

private:
    
    float _lodThresholdMultiplier;
};

/// A simple attribute class that stores its values inline.
template<class T, int bits = 32> class InlineAttribute : public Attribute {
public:
    
    InlineAttribute(const QString& name, const T& defaultValue = T()) : Attribute(name), _defaultValue(defaultValue) { }
    
    virtual void* create(void* copy) const { void* value; new (&value) T(*(T*)&copy); return value; }
    virtual void destroy(void* value) const { ((T*)&value)->~T(); }
    
    virtual void read(Bitstream& in, void*& value, bool isLeaf) const;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const;

    virtual bool equal(void* first, void* second) const { return decodeInline<T>(first) == decodeInline<T>(second); }

    virtual void* mix(void* first, void* second, float alpha) const { return create(alpha < 0.5f ? first : second); }

    virtual void* blend(void* source, void* dest) const { return create(source); }

    virtual void* getDefaultValue() const { return encodeInline(_defaultValue); }

protected:
    
    T _defaultValue;
};

template<class T, int bits> inline void InlineAttribute<T, bits>::read(Bitstream& in, void*& value, bool isLeaf) const {
    if (isLeaf) {
        value = getDefaultValue();
        in.read(&value, bits);
    }
}

template<class T, int bits> inline void InlineAttribute<T, bits>::write(Bitstream& out, void* value, bool isLeaf) const {
    if (isLeaf) {
        out.write(&value, bits);
    }
}

/// Provides averaging using the +=, ==, and / operators.
template<class T, int bits = 32> class SimpleInlineAttribute : public InlineAttribute<T, bits> {
public:
    
    SimpleInlineAttribute(const QString& name, const T& defaultValue = T()) : InlineAttribute<T, bits>(name, defaultValue) { }
    
    virtual bool merge(void*& parent, void* children[], bool postRead = false) const;
};

template<class T, int bits> inline bool SimpleInlineAttribute<T, bits>::merge(
        void*& parent, void* children[], bool postRead) const {
    T firstValue = decodeInline<T>(children[0]);
    T totalValue = firstValue;
    bool allChildrenEqual = true;
    for (int i = 1; i < Attribute::MERGE_COUNT; i++) {
        T value = decodeInline<T>(children[i]);
        totalValue += value;
        allChildrenEqual &= (firstValue == value);
    }
    parent = encodeInline(totalValue / Attribute::MERGE_COUNT);
    return allChildrenEqual;
}

/// Simple float attribute.
class FloatAttribute : public SimpleInlineAttribute<float> {
    Q_OBJECT
    Q_PROPERTY(float defaultValue MEMBER _defaultValue)

public:
    
    Q_INVOKABLE FloatAttribute(const QString& name = QString(), float defaultValue = 0.0f);
};

/// Provides appropriate averaging for RGBA values.
class QRgbAttribute : public InlineAttribute<QRgb> {
    Q_OBJECT
    Q_PROPERTY(uint defaultValue MEMBER _defaultValue)

public:
    
    Q_INVOKABLE QRgbAttribute(const QString& name = QString(), QRgb defaultValue = QRgb());
    
    virtual bool merge(void*& parent, void* children[], bool postRead = false) const;
    
    virtual void* mix(void* first, void* second, float alpha) const;
    
    virtual void* blend(void* source, void* dest) const;
    
    virtual void* createFromScript(const QScriptValue& value, QScriptEngine* engine) const;
    
    virtual void* createFromVariant(const QVariant& value) const;
    
    virtual QWidget* createEditor(QWidget* parent = NULL) const;
};

/// Provides appropriate averaging for packed normals.
class PackedNormalAttribute : public QRgbAttribute {
    Q_OBJECT

public:
    
    Q_INVOKABLE PackedNormalAttribute(const QString& name = QString(), QRgb defaultValue = QRgb());
    
    virtual bool merge(void*& parent, void* children[], bool postRead = false) const;
    
    virtual void* mix(void* first, void* second, float alpha) const;
    
    virtual void* blend(void* source, void* dest) const;
};

/// Packs a normal into an RGB value.
QRgb packNormal(const glm::vec3& normal);

/// Unpacks a normal from an RGB value.
glm::vec3 unpackNormal(QRgb value);

/// RGBA values for voxelized spanners.
class SpannerQRgbAttribute : public QRgbAttribute {
    Q_OBJECT

public:
    
    Q_INVOKABLE SpannerQRgbAttribute(const QString& name = QString(), QRgb defaultValue = QRgb());
    
    virtual void read(Bitstream& in, void*& value, bool isLeaf) const;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const;
    
    virtual MetavoxelNode* createMetavoxelNode(const AttributeValue& value, const MetavoxelNode* original) const;
    
    virtual bool merge(void*& parent, void* children[], bool postRead = false) const;
    
    virtual AttributeValue inherit(const AttributeValue& parentValue) const;
};

/// Packed normals for voxelized spanners.
class SpannerPackedNormalAttribute : public PackedNormalAttribute {
    Q_OBJECT

public:
    
    Q_INVOKABLE SpannerPackedNormalAttribute(const QString& name = QString(), QRgb defaultValue = QRgb());
    
    virtual void read(Bitstream& in, void*& value, bool isLeaf) const;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const;
    
    virtual MetavoxelNode* createMetavoxelNode(const AttributeValue& value, const MetavoxelNode* original) const;
    
    virtual bool merge(void*& parent, void* children[], bool postRead = false) const;
    
    virtual AttributeValue inherit(const AttributeValue& parentValue) const;
};

typedef QExplicitlySharedDataPointer<HeightfieldData> HeightfieldDataPointer;

/// Contains a block of heightfield data.
class HeightfieldData : public QSharedData {
public:

    static const int COLOR_BYTES = 3;
    
    HeightfieldData(const QByteArray& contents = QByteArray());
    virtual ~HeightfieldData();
    
    const QByteArray& getContents() const { return _contents; }

    void setDeltaData(const HeightfieldDataPointer& deltaData) { _deltaData = deltaData; }
    const HeightfieldDataPointer& getDeltaData() const { return _deltaData; }
    
    void setEncodedDelta(const QByteArray& encodedDelta) { _encodedDelta = encodedDelta; }
    const QByteArray& getEncodedDelta() const { return _encodedDelta; }
    
    QMutex& getEncodedDeltaMutex() { return _encodedDeltaMutex; }

protected:
    
    QByteArray _contents;
    QByteArray _encoded;
    QMutex _encodedMutex;
    
    HeightfieldDataPointer _deltaData;
    QByteArray _encodedDelta;
    QMutex _encodedDeltaMutex;

    class EncodedSubdivision {
    public:
        HeightfieldDataPointer ancestor;
        QByteArray data;
    };
    QVector<EncodedSubdivision> _encodedSubdivisions;
    QMutex _encodedSubdivisionsMutex;
};

typedef QExplicitlySharedDataPointer<HeightfieldHeightData> HeightfieldHeightDataPointer;

/// Contains a block of heightfield height data.
class HeightfieldHeightData : public HeightfieldData {
public:
    
    HeightfieldHeightData(const QByteArray& contents);
    HeightfieldHeightData(Bitstream& in, int bytes);
    HeightfieldHeightData(Bitstream& in, int bytes, const HeightfieldHeightDataPointer& reference);
    HeightfieldHeightData(Bitstream& in, int bytes, const HeightfieldHeightDataPointer& ancestor,
        const glm::vec3& minimum, float size);
    
    void write(Bitstream& out);
    void writeDelta(Bitstream& out, const HeightfieldHeightDataPointer& reference);
    void writeSubdivided(Bitstream& out, const HeightfieldHeightDataPointer& ancestor,
        const glm::vec3& minimum, float size);

private:
    
    void read(Bitstream& in, int bytes);
    void set(const QImage& image);
};

typedef QExplicitlySharedDataPointer<HeightfieldColorData> HeightfieldColorDataPointer;

/// Contains a block of heightfield color data.
class HeightfieldColorData : public HeightfieldData {
public:
    
    HeightfieldColorData(const QByteArray& contents);
    HeightfieldColorData(Bitstream& in, int bytes);
    HeightfieldColorData(Bitstream& in, int bytes, const HeightfieldColorDataPointer& reference);
    HeightfieldColorData(Bitstream& in, int bytes, const HeightfieldColorDataPointer& ancestor,
        const glm::vec3& minimum, float size);
        
    void write(Bitstream& out);
    void writeDelta(Bitstream& out, const HeightfieldColorDataPointer& reference);
    void writeSubdivided(Bitstream& out, const HeightfieldColorDataPointer& ancestor,
        const glm::vec3& minimum, float size);

private:
    
    void read(Bitstream& in, int bytes);
    void set(const QImage& image);
};

typedef QExplicitlySharedDataPointer<HeightfieldTextureData> HeightfieldTextureDataPointer;

/// Contains a block of heightfield texture data.
class HeightfieldTextureData : public HeightfieldData {
public:
    
    HeightfieldTextureData(const QByteArray& contents,
        const QVector<SharedObjectPointer>& textures = QVector<SharedObjectPointer>());
    HeightfieldTextureData(Bitstream& in, int bytes);
    HeightfieldTextureData(Bitstream& in, int bytes, const HeightfieldTextureDataPointer& reference);
    
    const QVector<SharedObjectPointer>& getTextures() const { return _textures; }
    
    void write(Bitstream& out);
    void writeDelta(Bitstream& out, const HeightfieldTextureDataPointer& reference);

private:
    
    void read(Bitstream& in, int bytes);
    void set(const QImage& image);
    
    QVector<SharedObjectPointer> _textures;
};

/// Contains the description of a heightfield texture.
class HeightfieldTexture : public SharedObject {
    Q_OBJECT
    Q_PROPERTY(QUrl url MEMBER _url)

public:
    
    Q_INVOKABLE HeightfieldTexture();

    const QUrl& getURL() const { return _url; }

private:
    
    QUrl _url;
};

/// An attribute that stores heightfield data.
class HeightfieldAttribute : public InlineAttribute<HeightfieldHeightDataPointer> {
    Q_OBJECT
    
public:
    
    Q_INVOKABLE HeightfieldAttribute(const QString& name = QString());
    
    virtual void read(Bitstream& in, void*& value, bool isLeaf) const;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const;
        
    virtual void readDelta(Bitstream& in, void*& value, void* reference, bool isLeaf) const;
    virtual void writeDelta(Bitstream& out, void* value, void* reference, bool isLeaf) const;

    virtual bool merge(void*& parent, void* children[], bool postRead = false) const;
};

/// An attribute that stores heightfield colors.
class HeightfieldColorAttribute : public InlineAttribute<HeightfieldColorDataPointer> {
    Q_OBJECT
    
public:
    
    Q_INVOKABLE HeightfieldColorAttribute(const QString& name = QString());
    
    virtual void read(Bitstream& in, void*& value, bool isLeaf) const;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const;
    
    virtual void readDelta(Bitstream& in, void*& value, void* reference, bool isLeaf) const;
    virtual void writeDelta(Bitstream& out, void* value, void* reference, bool isLeaf) const;
    
    virtual bool merge(void*& parent, void* children[], bool postRead = false) const;
};

/// An attribute that stores heightfield textures.
class HeightfieldTextureAttribute : public InlineAttribute<HeightfieldTextureDataPointer> {
    Q_OBJECT
    
public:
    
    Q_INVOKABLE HeightfieldTextureAttribute(const QString& name = QString());
    
    virtual void read(Bitstream& in, void*& value, bool isLeaf) const;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const;
    
    virtual void readDelta(Bitstream& in, void*& value, void* reference, bool isLeaf) const;
    virtual void writeDelta(Bitstream& out, void* value, void* reference, bool isLeaf) const;
    
    virtual bool merge(void*& parent, void* children[], bool postRead = false) const;
};

/// An attribute that takes the form of QObjects of a given meta-type (a subclass of SharedObject).
class SharedObjectAttribute : public InlineAttribute<SharedObjectPointer> {
    Q_OBJECT
    Q_PROPERTY(const QMetaObject* metaObject MEMBER _metaObject)
    
public:
    
    Q_INVOKABLE SharedObjectAttribute(const QString& name = QString(),
        const QMetaObject* metaObject = &SharedObject::staticMetaObject,
        const SharedObjectPointer& defaultValue = SharedObjectPointer());

    virtual void read(Bitstream& in, void*& value, bool isLeaf) const;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const;

    virtual bool deepEqual(void* first, void* second) const;

    virtual bool merge(void*& parent, void* children[], bool postRead = false) const;
    
    virtual void* createFromVariant(const QVariant& value) const;
    
    virtual QWidget* createEditor(QWidget* parent = NULL) const;

private:
    
    const QMetaObject* _metaObject;
};

/// An attribute that takes the form of a set of shared objects.
class SharedObjectSetAttribute : public InlineAttribute<SharedObjectSet> {
    Q_OBJECT
    Q_PROPERTY(const QMetaObject* metaObject MEMBER _metaObject)
    
public:
    
    Q_INVOKABLE SharedObjectSetAttribute(const QString& name = QString(),
        const QMetaObject* metaObject = &SharedObject::staticMetaObject);
    
    const QMetaObject* getMetaObject() const { return _metaObject; }
    
    virtual void read(Bitstream& in, void*& value, bool isLeaf) const;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const;
    
    virtual MetavoxelNode* createMetavoxelNode(const AttributeValue& value, const MetavoxelNode* original) const;
    
    virtual bool deepEqual(void* first, void* second) const;
    
    virtual MetavoxelNode* expandMetavoxelRoot(const MetavoxelNode& root);
    
    virtual bool merge(void*& parent, void* children[], bool postRead = false) const;

    virtual AttributeValue inherit(const AttributeValue& parentValue) const;

    virtual QWidget* createEditor(QWidget* parent = NULL) const;

private:
    
    const QMetaObject* _metaObject;
};

/// An attribute that takes the form of a set of spanners.
class SpannerSetAttribute : public SharedObjectSetAttribute {
    Q_OBJECT

public:
    
    Q_INVOKABLE SpannerSetAttribute(const QString& name = QString(),
        const QMetaObject* metaObject = &SharedObject::staticMetaObject);
    
    virtual void readMetavoxelRoot(MetavoxelData& data, MetavoxelStreamState& state);
    virtual void writeMetavoxelRoot(const MetavoxelNode& root, MetavoxelStreamState& state);
    
    virtual void readMetavoxelDelta(MetavoxelData& data, const MetavoxelNode& reference, MetavoxelStreamState& state);
    virtual void writeMetavoxelDelta(const MetavoxelNode& root, const MetavoxelNode& reference, MetavoxelStreamState& state);
    
    virtual void readMetavoxelSubdivision(MetavoxelData& data, MetavoxelStreamState& state);
    virtual void writeMetavoxelSubdivision(const MetavoxelNode& root, MetavoxelStreamState& state);
    
    virtual bool metavoxelRootsEqual(const MetavoxelNode& firstRoot, const MetavoxelNode& secondRoot,
        const glm::vec3& minimum, float size, const MetavoxelLOD& lod);
};

#endif // hifi_AttributeRegistry_h
