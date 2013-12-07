//
//  AttributeRegistry.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "AttributeRegistry.h"

AttributeRegistry AttributeRegistry::_instance;

AttributeRegistry::AttributeRegistry() : _lastAttributeID(0) {
}

AttributeID AttributeRegistry::getAttributeID(const QString& name) {
    AttributeID& id = _attributeIDs[name];
    if (id == 0) {
        id = ++_lastAttributeID;
        _attributes.insert(id, Attribute(id, name));
    }
    return id;
}

Attribute AttributeRegistry::getAttribute(AttributeID id) const {
    return _attributes.value(id);
} 

Attribute::Attribute(AttributeID id, const QString& name) :
    _id(id), _name(name) {
}
