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

class Visitation {
public:
    MetavoxelVisitor& visitor;
    QVector<MetavoxelNode*> nodes;
    QVector<AttributeValue> attributeValues;

    void apply();
};

void Visitation::apply() {
    Visitation nextVisitation = { visitor, QVector<MetavoxelNode*>(nodes.size()), attributeValues };
    for (int i = 0; i < nodes.size(); i++) {
        MetavoxelNode* node = nodes.at(i);
        if (node) {
            nextVisitation.attributeValues[i] = node->getAttributeValue(attributeValues[i].getAttribute());
        }
    }
    
    if (!visitor.visit(nextVisitation.attributeValues)) {
        return;
    }
    
    for (int i = 0; i < MetavoxelNode::CHILD_COUNT; i++) {
        bool anyChildrenPresent = false;
        for (int j = 0; j < nodes.size(); j++) {
            MetavoxelNode* node = nodes.at(j);
            if ((nextVisitation.nodes[j] = node ? node->getChild(i) : NULL)) {
                anyChildrenPresent = true;
            }            
        }
        if (anyChildrenPresent) {
            nextVisitation.apply();
        }
    }
}

void MetavoxelData::visitVoxels(const QVector<AttributePointer>& attributes, MetavoxelVisitor& visitor) {
    // start with the root values/defaults
    Visitation firstVisitation = { visitor, QVector<MetavoxelNode*>(attributes.size()),
        QVector<AttributeValue>(attributes.size()) };
    for (int i = 0; i < attributes.size(); i++) {
        firstVisitation.nodes[i] = _roots.value(attributes[i]);
        firstVisitation.attributeValues[i] = attributes[i];
    }
    firstVisitation.apply();
}

void MetavoxelData::setAttributeValue(const MetavoxelPath& path, const AttributeValue& attributeValue) {
    MetavoxelNode*& node = _roots[attributeValue.getAttribute()];
    if (node == NULL) {
        node = new MetavoxelNode(attributeValue.getAttribute());
    }
    if (node->setAttributeValue(path, 0, attributeValue) && attributeValue.isDefault()) {
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
            return node->getAttributeValue(attribute);
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
        return true;
    }
    int element = path[index];
    if (_children[element] == NULL) {
        AttributeValue ownAttributeValue = getAttributeValue(attributeValue.getAttribute());
        for (int i = 0; i < CHILD_COUNT; i++) {
            _children[i] = new MetavoxelNode(ownAttributeValue);
        }
    }
    _children[element]->setAttributeValue(path, index + 1, attributeValue);
    
    void* childValues[CHILD_COUNT];
    bool allLeaves = true;
    for (int i = 0; i < CHILD_COUNT; i++) {
        childValues[i] = _children[i]->_attributeValue;
        allLeaves &= _children[i]->isLeaf();
    }
    attributeValue.getAttribute()->destroy(_attributeValue);
    _attributeValue = attributeValue.getAttribute()->createAveraged(childValues);
    
    if (allLeaves && allChildrenEqual(attributeValue.getAttribute())) {
        clearChildren(attributeValue.getAttribute());
        return true;
    }
    
    return false;
}

void MetavoxelNode::setAttributeValue(const AttributeValue& attributeValue) {
    attributeValue.getAttribute()->destroy(_attributeValue);
    _attributeValue = attributeValue.copy();
    clearChildren(attributeValue.getAttribute());
}

AttributeValue MetavoxelNode::getAttributeValue(const AttributePointer& attribute) const {
    return AttributeValue(attribute, &_attributeValue);
}

bool MetavoxelNode::isLeaf() const {
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (_children[i]) {
            return false;
        }    
    }
    return true;
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

bool MetavoxelNode::allChildrenEqual(const AttributePointer& attribute) const {
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (!attribute->equal(_attributeValue, _children[i]->_attributeValue)) {
            return false;
        }
    }
    return true;
}

void MetavoxelNode::clearChildren(const AttributePointer& attribute) {
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (_children[i]) {
            _children[i]->destroy(attribute);
            delete _children[i];
            _children[i] = NULL;
        }
    }
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

