//
//  AttributeRegistry.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "AttributeRegistry.h"

AttributeRegistry AttributeRegistry::_instance;

AttributeRegistry::AttributeRegistry() {
    registerAttribute(AttributePointer(new InlineAttribute<float, 32>("blerp")));
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

