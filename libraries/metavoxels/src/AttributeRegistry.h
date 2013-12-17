//
//  AttributeRegistry.h
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__AttributeRegistry__
#define __interface__AttributeRegistry__

#include <QColor>
#include <QExplicitlySharedDataPointer>
#include <QHash>
#include <QObject>
#include <QSharedData>
#include <QSharedPointer>
#include <QString>

#include "Bitstream.h"

class QScriptContext;
class QScriptEngine;
class QScriptValue;

class Attribute;

typedef QSharedPointer<Attribute> AttributePointer;

/// Maintains information about metavoxel attribute types.
class AttributeRegistry {
public:
    
    /// Returns a pointer to the singleton registry instance.
    static AttributeRegistry* getInstance() { return &_instance; }
    
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
    
    /// Retrieves an attribute by name.
    AttributePointer getAttribute(const QString& name) const { return _attributes.value(name); }
    
    /// Returns a reference to the standard PolymorphicDataPointer "guide" attribute.
    const AttributePointer& getGuideAttribute() const { return _guideAttribute; }
    
    /// Returns a reference to the standard QRgb "color" attribute.
    const AttributePointer& getColorAttribute() const { return _colorAttribute; }
    
    /// Returns a reference to the standard QRgb "normal" attribute.
    const AttributePointer& getNormalAttribute() const { return _normalAttribute; }
    
private:

    static QScriptValue getAttribute(QScriptContext* context, QScriptEngine* engine);

    static AttributeRegistry _instance;

    QHash<QString, AttributePointer> _attributes;
    AttributePointer _guideAttribute;
    AttributePointer _colorAttribute;
    AttributePointer _normalAttribute;
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
    
    template<class T> T* getPointerValue() const { return static_cast<T*>(_value); }
    
    void* copy() const;

    bool isDefault() const;

    bool operator==(const AttributeValue& other) const;
    bool operator==(void* other) const;
    
protected:
    
    AttributePointer _attribute;
    void* _value;
};

// Assumes ownership of an attribute value.
class OwnedAttributeValue : public AttributeValue {
public:
    
    OwnedAttributeValue(const AttributePointer& attribute = AttributePointer());
    OwnedAttributeValue(const AttributePointer& attribute, void* value);
    OwnedAttributeValue(const AttributeValue& other);
    ~OwnedAttributeValue();
    
    OwnedAttributeValue& operator=(const AttributeValue& other);
};

/// Represents a registered attribute.
class Attribute : public QObject {
    Q_OBJECT
    
public:
    
    static const int MERGE_COUNT = 8;
    
    Attribute(const QString& name);
    virtual ~Attribute();

    Q_INVOKABLE QString getName() const { return objectName(); }

    void* create() const { return create(getDefaultValue()); }
    virtual void* create(void* copy) const = 0;
    virtual void destroy(void* value) const = 0;

    virtual bool read(Bitstream& in, void*& value) const = 0;
    virtual bool write(Bitstream& out, void* value) const = 0;

    virtual bool equal(void* first, void* second) const = 0;

    /// Merges the value of a parent and its children.
    /// \return whether or not the children and parent values are all equal
    virtual bool merge(void*& parent, void* children[]) const = 0;

    virtual void* getDefaultValue() const = 0;

    virtual void* createFromScript(const QScriptValue& value, QScriptEngine* engine) const { return create(); }
};

/// A simple attribute class that stores its values inline.
template<class T, int bits = 32> class InlineAttribute : public Attribute {
public:
    
    InlineAttribute(const QString& name, const T& defaultValue = T()) : Attribute(name), _defaultValue(defaultValue) { }
    
    virtual void* create(void* copy) const { void* value; new (&value) T(*(T*)&copy); return value; }
    virtual void destroy(void* value) const { ((T*)&value)->~T(); }
    
    virtual bool read(Bitstream& in, void*& value) const { value = getDefaultValue(); in.read(&value, bits); return false; }
    virtual bool write(Bitstream& out, void* value) const { out.write(&value, bits); return false; }

    virtual bool equal(void* first, void* second) const { return decodeInline<T>(first) == decodeInline<T>(second); }

    virtual void* getDefaultValue() const { return encodeInline(_defaultValue); }

private:
    
    T _defaultValue;
};

/// Provides merging using the =, ==, += and /= operators.
template<class T, int bits = 32> class SimpleInlineAttribute : public InlineAttribute<T, bits> {
public:
    
    SimpleInlineAttribute(const QString& name, T defaultValue = T()) : InlineAttribute<T, bits>(name, defaultValue) { }
    
    virtual bool merge(void*& parent, void* children[]) const;
};

template<class T, int bits> inline bool SimpleInlineAttribute<T, bits>::merge(void*& parent, void* children[]) const {
    T& merged = *(T*)&parent;
    merged = decodeInline<T>(children[0]);
    bool allChildrenEqual = true;
    for (int i = 1; i < Attribute::MERGE_COUNT; i++) {
        merged += decodeInline<T>(children[i]);
        allChildrenEqual &= (decodeInline<T>(children[0]) == decodeInline<T>(children[i]));
    }
    merged /= Attribute::MERGE_COUNT;
    return allChildrenEqual;
}

/// Provides appropriate averaging for RGBA values.
class QRgbAttribute : public InlineAttribute<QRgb> {
public:
    
    QRgbAttribute(const QString& name, QRgb defaultValue = QRgb());
    
    virtual bool merge(void*& parent, void* children[]) const;
    
    virtual void* createFromScript(const QScriptValue& value, QScriptEngine* engine) const;
};

/// An attribute class that stores pointers to its values.
template<class T> class PointerAttribute : public Attribute {
public:
    
    PointerAttribute(const QString& name, T defaultValue = T()) : Attribute(name), _defaultValue(defaultValue) { }
    
    virtual void* create(void* copy) const { new T(*static_cast<T*>(copy)); }
    virtual void destroy(void* value) const { delete static_cast<T*>(value); }
    
    virtual bool read(Bitstream& in, void*& value) const { in >> *static_cast<T*>(value); return true; }
    virtual bool write(Bitstream& out, void* value) const { out << *static_cast<T*>(value); return true; }

    virtual bool equal(void* first, void* second) const { return *static_cast<T*>(first) == *static_cast<T*>(second); }

    virtual void* getDefaultValue() const { return const_cast<void*>((void*)&_defaultValue); }

private:
    
    T _defaultValue;
}; 

/// Provides merging using the =, ==, += and /= operators.
template<class T> class SimplePointerAttribute : public PointerAttribute<T> {
public:
    
    SimplePointerAttribute(const QString& name, T defaultValue = T()) : PointerAttribute<T>(name, defaultValue) { }
    
    virtual bool merge(void*& parent, void* children[]) const;
};

template<class T> inline bool SimplePointerAttribute<T>::merge(void*& parent, void* children[]) const {
    T& merged = *static_cast<T*>(parent);
    merged = *static_cast<T*>(children[0]);
    bool allChildrenEqual = true;
    for (int i = 1; i < Attribute::MERGE_COUNT; i++) {
        merged += *static_cast<T*>(children[i]);
        allChildrenEqual &= (*static_cast<T*>(children[0]) == *static_cast<T*>(children[i]));
    }
    merged /= Attribute::MERGE_COUNT;
    return allChildrenEqual;
}

/// Base class for polymorphic attribute data.  
class PolymorphicData : public QSharedData {
public:
    
    virtual ~PolymorphicData();
    
    /// Creates a new clone of this object.
    virtual PolymorphicData* clone() const = 0;
};

template<> PolymorphicData* QExplicitlySharedDataPointer<PolymorphicData>::clone();

typedef QExplicitlySharedDataPointer<PolymorphicData> PolymorphicDataPointer;

/// Provides polymorphic streaming and averaging.
class PolymorphicAttribute : public InlineAttribute<PolymorphicDataPointer> {
public:

    PolymorphicAttribute(const QString& name, const PolymorphicDataPointer& defaultValue = PolymorphicDataPointer());
    
    virtual bool merge(void*& parent, void* children[]) const;
};

#endif /* defined(__interface__AttributeRegistry__) */
