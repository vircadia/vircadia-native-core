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
#include <QHash>
#include <QSharedPointer>
#include <QString>

#include "Bitstream.h"

class Attribute;

typedef QSharedPointer<Attribute> AttributePointer;

/// Maintains information about metavoxel attribute types.
class AttributeRegistry {
public:
    
    /// Returns a pointer to the singleton registry instance.
    static AttributeRegistry* getInstance() { return &_instance; }
    
    AttributeRegistry();
    
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
    
    /// Returns a reference to the standard QRgb "color" attribute.
    const AttributePointer& getColorAttribute() const { return _colorAttribute; }
    
    /// Returns a reference to the standard QRgb "normal" attribute.
    const AttributePointer& getNormalAttribute() const { return _normalAttribute; }
    
private:

    static AttributeRegistry _instance;

    QHash<QString, AttributePointer> _attributes;
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
    
    AttributeValue(const AttributePointer& attribute = AttributePointer(), void* const* value = NULL);
    AttributeValue(const AttributeValue& other);
    ~AttributeValue();
    
    AttributeValue& operator=(const AttributeValue& other);
    
    AttributePointer getAttribute() const { return _attribute; }
    void* getValue() const { return _value; }
    
    template<class T> void setInlineValue(T value) { _value = encodeInline(value); }
    template<class T> T getInlineValue() const { return decodeInline<T>(_value); }
    
    void* copy() const;

    bool isDefault() const;

    bool operator==(const AttributeValue& other) const;
    bool operator==(void* other) const;
    
private:
    
    AttributePointer _attribute;
    void* _value;
};

/// Represents a registered attribute.
class Attribute {
public:

    static const int AVERAGE_COUNT = 8;
    
    Attribute(const QString& name);
    virtual ~Attribute();

    const QString& getName() const { return _name; }

    virtual void* create(void* const* copy = NULL) const = 0;
    virtual void destroy(void* value) const = 0;

    virtual bool read(Bitstream& in, void*& value) const = 0;
    virtual bool write(Bitstream& out, void* value) const = 0;

    virtual bool equal(void* first, void* second) const = 0;

    virtual void* createAveraged(void* values[]) const = 0;

    virtual void* getDefaultValue() const = 0;

private:

    QString _name;
};

/// A simple attribute class that stores its values inline.
template<class T, int bits> class InlineAttribute : public Attribute {
public:
    
    InlineAttribute(const QString& name, T defaultValue = T()) : Attribute(name), _defaultValue(encodeInline(defaultValue)) { }
    
    virtual void* create(void* const* copy = NULL) const { return (copy == NULL) ? _defaultValue : *copy; }
    virtual void destroy(void* value) const { /* no-op */ }
    
    virtual bool read(Bitstream& in, void*& value) const { value = getDefaultValue(); in.read(&value, bits); return false; }
    virtual bool write(Bitstream& out, void* value) const { out.write(&value, bits); return false; }

    virtual bool equal(void* first, void* second) const { return first == second; }

    virtual void* createAveraged(void* values[]) const;

    virtual void* getDefaultValue() const { return _defaultValue; }

private:
    
    void* _defaultValue;
};

template<class T, int bits> inline void* InlineAttribute<T, bits>::createAveraged(void* values[]) const {
    T total = T();
    for (int i = 0; i < AVERAGE_COUNT; i++) {
        total += decodeInline<T>(values[i]);
    }
    total /= AVERAGE_COUNT;
    return encodeInline(total);
}

/// Provides appropriate averaging for RGBA values.
class QRgbAttribute : public InlineAttribute<QRgb, 32> {
public:
    
    QRgbAttribute(const QString& name, QRgb defaultValue = QRgb());
    
    virtual void* createAveraged(void* values[]) const;
};

#endif /* defined(__interface__AttributeRegistry__) */
