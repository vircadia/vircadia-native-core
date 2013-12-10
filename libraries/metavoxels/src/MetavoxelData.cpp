//
//  MetavoxelData.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "MetavoxelData.h"

MetavoxelData::~MetavoxelData() {
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = _roots.constBegin(); it != _roots.constEnd(); it++) {
        it.value()->destroy(it.key());
        delete it.value();
    }
}

void MetavoxelData::visitVoxels(const QVector<Attribute*>& attributes, VoxelVisitor* visitor) {
    // map attributes to layers, indices
    
}

void MetavoxelData::setAttributeValue(const MetavoxelPath& path, const AttributeValue& attributeValue) {
    MetavoxelNode*& node = _roots[attributeValue.getAttribute()];
    if (node == NULL) {
        node = new MetavoxelNode(attributeValue.getAttribute());
    }
    if (node->setAttributeValue(path, 0, attributeValue)) {
        node->destroy(attributeValue.getAttribute());
        delete node;
        _roots.remove(attributeValue.getAttribute());
    }
}

AttributeValue MetavoxelData::getAttributeValue(const MetavoxelPath& path, const AttributePointer& attribute) const {
    MetavoxelNode* node = _roots.value(attribute);
    if (node == NULL) {
        return AttributeValue(attribute);
    }
    for (int i = 0, n = path.getSize(); i < n; i++) {
        int index = path[i];
        MetavoxelNode* child = node->getChild(i);
        if (child == NULL) {
            return AttributeValue(attribute);
        }
        node = child;
    }
    return node->getAttributeValue(attribute);
}

MetavoxelNode::MetavoxelNode(const AttributeValue& attributeValue) {
    _attributeValue = attributeValue.copy();
    for (int i = 0; i < CHILD_COUNT; i++) {
        _children[i] = NULL;
    }
}

bool MetavoxelNode::setAttributeValue(const MetavoxelPath& path, int index, const AttributeValue& attributeValue) {
    if (index == path.getSize()) {
        setAttributeValue(attributeValue);
        return attributeValue.isDefault();
    }
    int element = path[index];
    if (_children[element] == NULL) {
        _children[element] = new MetavoxelNode(attributeValue.getAttribute());
    }
    if (_children[element]->setAttributeValue(path, index + 1, attributeValue)) {
        _children[element]->destroy(attributeValue.getAttribute());
        delete _children[element];
        _children[element] = NULL;
        if (allChildrenNull()) {
            return true;
        }
    }
    void* childValues[CHILD_COUNT];
    for (int i = 0; i < CHILD_COUNT; i++) {
        childValues[i] = (_children[i] == NULL) ? attributeValue.getAttribute()->getDefaultValue() :
            _children[i]->_attributeValue;
    }
    attributeValue.getAttribute()->destroy(_attributeValue);
    _attributeValue = attributeValue.getAttribute()->createAveraged(childValues);
    return false;
}

void MetavoxelNode::setAttributeValue(const AttributeValue& attributeValue) {
    attributeValue.getAttribute()->destroy(_attributeValue);
    _attributeValue = attributeValue.copy();
}

AttributeValue MetavoxelNode::getAttributeValue(const AttributePointer& attribute) const {
    return AttributeValue(attribute, &_attributeValue);
}

void MetavoxelNode::destroy(const AttributePointer& attribute) {
    attribute->destroy(_attributeValue);
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (_children[i]) {
            _children[i]->destroy(attribute);
            delete _children[i];
        }
    }
}

bool MetavoxelNode::allChildrenNull() const {
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (_children[i] != NULL) {
            return false;
        }
    }
    return true;
}

int MetavoxelPath::operator[](int index) const {
    return _array.at(index * BITS_PER_ELEMENT) | (_array.at(index * BITS_PER_ELEMENT + 1) << 1) |
        (_array.at(index * BITS_PER_ELEMENT + 2) << 2);
}

MetavoxelPath& MetavoxelPath::operator+=(int element) {
    int offset = _array.size();
    _array.resize(offset + BITS_PER_ELEMENT);
    _array.setBit(offset, element & 0x01);
    _array.setBit(offset + 1, (element >> 1) & 0x01);
    _array.setBit(offset + 2, element >> 2);
    return *this;
}

