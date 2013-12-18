//
//  MetavoxelData.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QScriptEngine>
#include <QtDebug>

#include "MetavoxelData.h"

MetavoxelData::~MetavoxelData() {
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = _roots.constBegin(); it != _roots.constEnd(); it++) {
        it.value()->destroy(it.key());
        delete it.value();
    }
}

void MetavoxelData::guide(MetavoxelVisitor& visitor) {
    // start with the root values/defaults (plus the guide attribute)
    const float TOP_LEVEL_SIZE = 1.0f;
    const QVector<AttributePointer>& attributes = visitor.getAttributes();
    MetavoxelVisitation firstVisitation = { visitor, QVector<MetavoxelNode*>(attributes.size() + 1),
        { glm::vec3(), TOP_LEVEL_SIZE, QVector<AttributeValue>(attributes.size() + 1) } };
    for (int i = 0; i < attributes.size(); i++) {
        MetavoxelNode* node = _roots.value(attributes[i]);
        firstVisitation.nodes[i] = node;
        firstVisitation.info.attributeValues[i] = node ? node->getAttributeValue(attributes[i]) : attributes[i];
    }
    AttributePointer guideAttribute = AttributeRegistry::getInstance()->getGuideAttribute();
    MetavoxelNode* node = _roots.value(guideAttribute);
    firstVisitation.nodes.last() = node;
    firstVisitation.info.attributeValues.last() = node ? node->getAttributeValue(guideAttribute) : guideAttribute;
    static_cast<MetavoxelGuide*>(firstVisitation.info.attributeValues.last().getInlineValue<
        PolymorphicDataPointer>().data())->guide(firstVisitation);
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
        MetavoxelNode* child = node->getChild(path[i]);
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
    if (attributeValue.getAttribute()->merge(_attributeValue, childValues) && allLeaves) {
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
    return AttributeValue(attribute, _attributeValue);
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

PolymorphicData* DefaultMetavoxelGuide::clone() const {
    return new DefaultMetavoxelGuide();
}

const int X_MAXIMUM_FLAG = 1;
const int Y_MAXIMUM_FLAG = 2;
const int Z_MAXIMUM_FLAG = 4;

void DefaultMetavoxelGuide::guide(MetavoxelVisitation& visitation) {
    visitation.info.isLeaf = visitation.allNodesLeaves();
    if (!visitation.visitor.visit(visitation.info) || visitation.info.isLeaf) {
        return;
    }
    MetavoxelVisitation nextVisitation = { visitation.visitor, QVector<MetavoxelNode*>(visitation.nodes.size()),
        { glm::vec3(), visitation.info.size * 0.5f, QVector<AttributeValue>(visitation.nodes.size()) } };
    for (int i = 0; i < MetavoxelNode::CHILD_COUNT; i++) {
        for (int j = 0; j < visitation.nodes.size(); j++) {
            MetavoxelNode* node = visitation.nodes.at(j);
            MetavoxelNode* child = node ? node->getChild(i) : NULL;
            nextVisitation.info.attributeValues[j] = ((nextVisitation.nodes[j] = child)) ?
                child->getAttributeValue(visitation.info.attributeValues[j].getAttribute()) :
                    visitation.info.attributeValues[j];
        }
        nextVisitation.info.minimum = visitation.info.minimum + glm::vec3(
            (i & X_MAXIMUM_FLAG) ? nextVisitation.info.size : 0.0f,
            (i & Y_MAXIMUM_FLAG) ? nextVisitation.info.size : 0.0f,
            (i & Z_MAXIMUM_FLAG) ? nextVisitation.info.size : 0.0f);
        static_cast<MetavoxelGuide*>(nextVisitation.info.attributeValues.last().getInlineValue<
            PolymorphicDataPointer>().data())->guide(nextVisitation);
    }
}

QScriptValue ScriptedMetavoxelGuide::getAttributes(QScriptContext* context, QScriptEngine* engine) {
    ScriptedMetavoxelGuide* guide = static_cast<ScriptedMetavoxelGuide*>(context->callee().data().toVariant().value<void*>());
    
    const QVector<AttributePointer>& attributes = guide->_visitation->visitor.getAttributes();
    QScriptValue attributesValue = engine->newArray(attributes.size());
    for (int i = 0; i < attributes.size(); i++) {
        attributesValue.setProperty(i, engine->newQObject(attributes.at(i).data(), QScriptEngine::QtOwnership,
            QScriptEngine::PreferExistingWrapperObject));
    }
    
    return attributesValue;
}

QScriptValue ScriptedMetavoxelGuide::visit(QScriptContext* context, QScriptEngine* engine) {
    ScriptedMetavoxelGuide* guide = static_cast<ScriptedMetavoxelGuide*>(context->callee().data().toVariant().value<void*>());

    // start with the basics, including inherited attribute values
    QScriptValue infoValue = context->argument(0);
    QScriptValue minimum = infoValue.property(guide->_minimumHandle);
    MetavoxelInfo info = {
        glm::vec3(minimum.property(0).toNumber(), minimum.property(1).toNumber(), minimum.property(2).toNumber()),
        infoValue.property(guide->_sizeHandle).toNumber(), guide->_visitation->info.attributeValues,
        infoValue.property(guide->_isLeafHandle).toBool() };
    
    // extract and convert the values provided by the script
    QScriptValue attributeValues = infoValue.property(guide->_attributeValuesHandle);
    const QVector<AttributePointer>& attributes = guide->_visitation->visitor.getAttributes();
    for (int i = 0; i < attributes.size(); i++) {
        QScriptValue attributeValue = attributeValues.property(i);
        if (attributeValue.isValid()) {
            info.attributeValues[i] = AttributeValue(attributes.at(i),
                attributes.at(i)->createFromScript(attributeValue, engine));
        }
    }
    
    QScriptValue result = guide->_visitation->visitor.visit(info);
    
    // destroy any created values
    for (int i = 0; i < attributes.size(); i++) {
        if (attributeValues.property(i).isValid()) {
            info.attributeValues[i].getAttribute()->destroy(info.attributeValues[i].getValue());
        }
    }
    
    return result;
}

ScriptedMetavoxelGuide::ScriptedMetavoxelGuide(const QScriptValue& guideFunction) :
    _guideFunction(guideFunction),
    _minimumHandle(guideFunction.engine()->toStringHandle("minimum")),
    _sizeHandle(guideFunction.engine()->toStringHandle("size")),
    _attributeValuesHandle(guideFunction.engine()->toStringHandle("attributeValues")),
    _isLeafHandle(guideFunction.engine()->toStringHandle("isLeaf")),
    _getAttributesFunction(guideFunction.engine()->newFunction(getAttributes, 0)),
    _visitFunction(guideFunction.engine()->newFunction(visit, 1)),
    _info(guideFunction.engine()->newObject()),
    _minimum(guideFunction.engine()->newArray(3)) {
    
    _arguments.append(guideFunction.engine()->newObject());
    QScriptValue visitor = guideFunction.engine()->newObject();
    visitor.setProperty("getAttributes", _getAttributesFunction);
    visitor.setProperty("visit", _visitFunction);
    _arguments[0].setProperty("visitor", visitor);
    _arguments[0].setProperty("info", _info);
    _info.setProperty(_minimumHandle, _minimum);
}

PolymorphicData* ScriptedMetavoxelGuide::clone() const {
    return new ScriptedMetavoxelGuide(_guideFunction);
}

void ScriptedMetavoxelGuide::guide(MetavoxelVisitation& visitation) {
    QScriptValue data = _guideFunction.engine()->newVariant(QVariant::fromValue<void*>(this));
    _getAttributesFunction.setData(data);
    _visitFunction.setData(data);
    _minimum.setProperty(0, visitation.info.minimum.x);
    _minimum.setProperty(1, visitation.info.minimum.y);
    _minimum.setProperty(2, visitation.info.minimum.z);
    _info.setProperty(_sizeHandle, visitation.info.size);
    _info.setProperty(_isLeafHandle, visitation.info.isLeaf);
    _visitation = &visitation;
    _guideFunction.call(QScriptValue(), _arguments);
    if (_guideFunction.engine()->hasUncaughtException()) {
        qDebug() << "Script error: " << _guideFunction.engine()->uncaughtException().toString() << "\n";
    }
}

bool MetavoxelVisitation::allNodesLeaves() const {
    foreach (MetavoxelNode* node, nodes) {
        if (node != NULL && !node->isLeaf()) {
            return false;
        }
    }
    return true;
}
