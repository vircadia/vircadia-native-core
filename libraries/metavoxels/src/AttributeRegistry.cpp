//
//  AttributeRegistry.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "AttributeRegistry.h"

AttributeRegistry AttributeRegistry::_instance;

AttributeRegistry::AttributeRegistry() :
    _colorAttribute(registerAttribute(new QRgbAttribute("color"))),
    _normalAttribute(registerAttribute(new QRgbAttribute("normal"))) {
}

AttributePointer AttributeRegistry::registerAttribute(AttributePointer attribute) {
    AttributePointer& pointer = _attributes[attribute->getName()];
    if (!pointer) {
        pointer = attribute;
    }
    return pointer;
}

AttributeValue::AttributeValue(const AttributePointer& attribute, void* const* value) :
        _attribute(attribute) {
    
    if (_attribute) {
        _value = _attribute->create(value);
    }
}

AttributeValue::AttributeValue(const AttributeValue& other) :
        _attribute(other._attribute) {
    
    if (_attribute) {
        _value = _attribute->create(&other._value);
    }
}

AttributeValue::~AttributeValue() {
    if (_attribute) {
        _attribute->destroy(_value);
    }
}

AttributeValue& AttributeValue::operator=(const AttributeValue& other) {
    if (_attribute) {
        _attribute->destroy(_value);
    }
    if ((_attribute = other._attribute)) {
        _value = _attribute->create(&other._value);
    }
}

void* AttributeValue::copy() const {
    return _attribute->create(&_value);
}

bool AttributeValue::isDefault() const {
    return !_attribute || _attribute->equal(_value, _attribute->getDefaultValue());
}

bool AttributeValue::operator==(const AttributeValue& other) const {
    return _attribute == other._attribute && (!_attribute || _attribute->equal(_value, other._value));
}

bool AttributeValue::operator==(void* other) const {
    return _attribute && _attribute->equal(_value, other);
}

Attribute::Attribute(const QString& name) : _name(name) {
}

Attribute::~Attribute() {
}

QRgbAttribute::QRgbAttribute(const QString& name, QRgb defaultValue) :
    InlineAttribute<QRgb, 32>(name, defaultValue) {
}
 
void* QRgbAttribute::createAveraged(void* values[]) const {
    int totalRed = 0;
    int totalGreen = 0;
    int totalBlue = 0;
    int totalAlpha = 0;
    for (int i = 0; i < AVERAGE_COUNT; i++) {
        QRgb value = decodeInline<QRgb>(values[i]);
        totalRed += qRed(value);
        totalGreen += qGreen(value);
        totalBlue += qBlue(value);
        totalAlpha += qAlpha(value);
    }
    return encodeInline(qRgba(totalRed / AVERAGE_COUNT, totalGreen / AVERAGE_COUNT,
        totalBlue / AVERAGE_COUNT, totalAlpha / AVERAGE_COUNT));
} 

