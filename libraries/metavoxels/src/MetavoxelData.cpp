//
//  MetavoxelData.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QDateTime>
#include <QScriptEngine>
#include <QtDebug>

#include "MetavoxelData.h"
#include "MetavoxelUtil.h"
#include "ScriptCache.h"

REGISTER_META_OBJECT(MetavoxelGuide)
REGISTER_META_OBJECT(DefaultMetavoxelGuide)
REGISTER_META_OBJECT(ScriptedMetavoxelGuide)
REGISTER_META_OBJECT(ThrobbingMetavoxelGuide)
REGISTER_META_OBJECT(Spanner)
REGISTER_META_OBJECT(StaticModel)

MetavoxelData::MetavoxelData() : _size(1.0f) {
}

MetavoxelData::MetavoxelData(const MetavoxelData& other) :
    _size(other._size),
    _roots(other._roots) {
    
    incrementRootReferenceCounts();
}

MetavoxelData::~MetavoxelData() {
    decrementRootReferenceCounts();
}

MetavoxelData& MetavoxelData::operator=(const MetavoxelData& other) {
    decrementRootReferenceCounts();
    _size = other._size;
    _roots = other._roots;
    incrementRootReferenceCounts();
    return *this;
}

Box MetavoxelData::getBounds() const {
    float halfSize = _size * 0.5f;
    return Box(glm::vec3(-halfSize, -halfSize, -halfSize), glm::vec3(halfSize, halfSize, halfSize));
}

void MetavoxelData::guide(MetavoxelVisitor& visitor) {
    // let the visitor know we're about to begin a tour
    visitor.prepare();

    // start with the root values/defaults (plus the guide attribute)
    const QVector<AttributePointer>& inputs = visitor.getInputs();
    const QVector<AttributePointer>& outputs = visitor.getOutputs();
    MetavoxelVisitation firstVisitation = { NULL, visitor, QVector<MetavoxelNode*>(inputs.size() + 1),
        QVector<MetavoxelNode*>(outputs.size()), { glm::vec3(_size, _size, _size) * -0.5f, _size,
            QVector<AttributeValue>(inputs.size() + 1), QVector<OwnedAttributeValue>(outputs.size()) } };
    for (int i = 0; i < inputs.size(); i++) {
        MetavoxelNode* node = _roots.value(inputs.at(i));
        firstVisitation.inputNodes[i] = node;
        firstVisitation.info.inputValues[i] = node ? node->getAttributeValue(inputs[i]) : inputs[i];
    }
    AttributePointer guideAttribute = AttributeRegistry::getInstance()->getGuideAttribute();
    MetavoxelNode* node = _roots.value(guideAttribute);
    firstVisitation.inputNodes.last() = node;
    firstVisitation.info.inputValues.last() = node ? node->getAttributeValue(guideAttribute) : guideAttribute;
    for (int i = 0; i < outputs.size(); i++) {
        MetavoxelNode* node = _roots.value(outputs.at(i));
        firstVisitation.outputNodes[i] = node;
    }
    static_cast<MetavoxelGuide*>(firstVisitation.info.inputValues.last().getInlineValue<
        SharedObjectPointer>().data())->guide(firstVisitation);
    for (int i = 0; i < outputs.size(); i++) {
        OwnedAttributeValue& value = firstVisitation.info.outputValues[i];
        if (!value.getAttribute()) {
            continue;
        }
        // replace the old node with the new
        MetavoxelNode*& node = _roots[value.getAttribute()];
        if (node) {
            node->decrementReferenceCount(value.getAttribute());
        }
        node = firstVisitation.outputNodes.at(i);
        if (node->isLeaf() && value.isDefault()) {
            // immediately remove the new node if redundant
            node->decrementReferenceCount(value.getAttribute());
            _roots.remove(value.getAttribute());
        }
    }
}

class InsertVisitor : public MetavoxelVisitor {
public:
    
    InsertVisitor(const AttributePointer& attribute, const Box& bounds, float granularity, const SharedObjectPointer& object);
    
    virtual bool visit(MetavoxelInfo& info);

private:
    
    const AttributePointer& _attribute;
    const Box& _bounds;
    float _longestSide;
    const SharedObjectPointer& _object;
};

InsertVisitor::InsertVisitor(const AttributePointer& attribute, const Box& bounds,
        float granularity, const SharedObjectPointer& object) :
    MetavoxelVisitor(QVector<AttributePointer>() << attribute, QVector<AttributePointer>() << attribute),
    _attribute(attribute),
    _bounds(bounds),
    _longestSide(qMax(bounds.getLongestSide(), granularity)),
    _object(object) {
}

bool InsertVisitor::visit(MetavoxelInfo& info) {
    if (!info.getBounds().intersects(_bounds)) {
        return false;
    }
    if (info.size > _longestSide) {
        return true;
    }
    SharedObjectSet set = info.inputValues.at(0).getInlineValue<SharedObjectSet>();
    set.insert(_object);
    info.outputValues[0] = AttributeValue(_attribute, encodeInline(set));
    return false;
}

void MetavoxelData::insert(const AttributePointer& attribute, const Box& bounds,
        float granularity, const SharedObjectPointer& object) {
    // expand to fit the entire bounds
    while (!getBounds().contains(bounds)) {
        expand();
    }
    InsertVisitor visitor(attribute, bounds, granularity, object);
    guide(visitor);
}

class RemoveVisitor : public MetavoxelVisitor {
public:
    
    RemoveVisitor(const AttributePointer& attribute, const Box& bounds, float granularity, const SharedObjectPointer& object);
    
    virtual bool visit(MetavoxelInfo& info);

private:
    
    const AttributePointer& _attribute;
    const Box& _bounds;
    float _longestSide;
    const SharedObjectPointer& _object;
};

RemoveVisitor::RemoveVisitor(const AttributePointer& attribute, const Box& bounds,
        float granularity, const SharedObjectPointer& object) :
    MetavoxelVisitor(QVector<AttributePointer>() << attribute, QVector<AttributePointer>() << attribute),
    _attribute(attribute),
    _bounds(bounds),
    _longestSide(qMax(bounds.getLongestSide(), granularity)),
    _object(object) {
}

bool RemoveVisitor::visit(MetavoxelInfo& info) {
    if (!info.getBounds().intersects(_bounds)) {
        return false;
    }
    if (info.size > _longestSide) {
        return true;
    }
    SharedObjectSet set = info.inputValues.at(0).getInlineValue<SharedObjectSet>();
    set.remove(_object);
    info.outputValues[0] = AttributeValue(_attribute, encodeInline(set));
    return false;
}

void MetavoxelData::remove(const AttributePointer& attribute, const Box& bounds,
        float granularity, const SharedObjectPointer& object) {
    RemoveVisitor visitor(attribute, bounds, granularity, object);
    guide(visitor);
}

void MetavoxelData::clear(const AttributePointer& attribute) {
    MetavoxelNode* node = _roots.take(attribute);
    if (node) {
        node->decrementReferenceCount(attribute);
    }
}

const int X_MAXIMUM_FLAG = 1;
const int Y_MAXIMUM_FLAG = 2;
const int Z_MAXIMUM_FLAG = 4;
const int MAXIMUM_FLAG_MASK = X_MAXIMUM_FLAG | Y_MAXIMUM_FLAG | Z_MAXIMUM_FLAG;

static int getOppositeIndex(int index) {
    return index ^ MAXIMUM_FLAG_MASK;
}

void MetavoxelData::expand() {
    for (QHash<AttributePointer, MetavoxelNode*>::iterator it = _roots.begin(); it != _roots.end(); it++) {
        MetavoxelNode* newParent = new MetavoxelNode(it.key());
        for (int i = 0; i < MetavoxelNode::CHILD_COUNT; i++) {
            MetavoxelNode* newChild = new MetavoxelNode(it.key());
            newParent->setChild(i, newChild);
            int index = getOppositeIndex(i);
            if (it.value()->isLeaf()) {
                newChild->setChild(index, new MetavoxelNode(it.value()->getAttributeValue(it.key())));               
            } else {
                MetavoxelNode* grandchild = it.value()->getChild(i);
                grandchild->incrementReferenceCount();
                newChild->setChild(index, grandchild);
            }
            for (int j = 1; j < MetavoxelNode::CHILD_COUNT; j++) {
                MetavoxelNode* newGrandchild = new MetavoxelNode(it.key());
                newChild->setChild((index + j) % MetavoxelNode::CHILD_COUNT, newGrandchild);
            }
            newChild->mergeChildren(it.key());
        }
        newParent->mergeChildren(it.key());
        it.value()->decrementReferenceCount(it.key());
        it.value() = newParent;
    }
    _size *= 2.0f;
}

void MetavoxelData::read(Bitstream& in) {
    // clear out any existing roots
    decrementRootReferenceCounts();
    _roots.clear();

    in >> _size;
    
    // read in the new roots
    int rootCount;
    in >> rootCount;
    for (int i = 0; i < rootCount; i++) {
        AttributePointer attribute;
        in >> attribute;
        MetavoxelNode*& root = _roots[attribute];
        root = new MetavoxelNode(attribute);
        root->read(attribute, in);
    }
}

void MetavoxelData::write(Bitstream& out) const {
    out << _size;
    out << _roots.size();
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = _roots.constBegin(); it != _roots.constEnd(); it++) {
        out << it.key();
        it.value()->write(it.key(), out);
    }
}

void MetavoxelData::readDelta(const MetavoxelData& reference, Bitstream& in) {
    // shallow copy the reference
    *this = reference;

    bool changed;
    in >> changed;
    if (!changed) {
        return;
    }

    bool sizeChanged;
    in >> sizeChanged;
    if (sizeChanged) {
        float size;
        in >> size;
        while (_size < size) {
            expand();
        }
    }

    int changedCount;
    in >> changedCount;
    for (int i = 0; i < changedCount; i++) {
        AttributePointer attribute;
        in >> attribute;
        MetavoxelNode*& root = _roots[attribute];
        if (root) {
            MetavoxelNode* oldRoot = root;
            root = new MetavoxelNode(attribute);
            root->readDelta(attribute, *oldRoot, in);
            oldRoot->decrementReferenceCount(attribute);
            
        } else {
            root = new MetavoxelNode(attribute);
            root->read(attribute, in);
        } 
    }
    
    int removedCount;
    in >> removedCount;
    for (int i = 0; i < removedCount; i++) {
        AttributePointer attribute;
        in >> attribute;
        _roots.take(attribute)->decrementReferenceCount(attribute);
    }
}

void MetavoxelData::writeDelta(const MetavoxelData& reference, Bitstream& out) const {
    // first things first: there might be no change whatsoever
    if (_size == reference._size && _roots == reference._roots) {
        out << false;
        return;
    }
    out << true;
    
    // compare the size; if changed (rare), we must compare to the expanded reference
    const MetavoxelData* expandedReference = &reference;
    if (_size == reference._size) {
        out << false;
    } else {
        out << true;
        out << _size;
        
        MetavoxelData* expanded = new MetavoxelData(reference);
        while (expanded->_size < _size) {
            expanded->expand();
        }
        expandedReference = expanded;
    }

    // count the number of roots added/changed, then write
    int changedCount = 0;
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = _roots.constBegin(); it != _roots.constEnd(); it++) {
        MetavoxelNode* referenceRoot = expandedReference->_roots.value(it.key());
        if (it.value() != referenceRoot) {
            changedCount++;
        }
    }
    out << changedCount;
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = _roots.constBegin(); it != _roots.constEnd(); it++) {
        MetavoxelNode* referenceRoot = expandedReference->_roots.value(it.key());
        if (it.value() != referenceRoot) {
            out << it.key();
            if (referenceRoot) {
                it.value()->writeDelta(it.key(), *referenceRoot, out);
            } else {
                it.value()->write(it.key(), out);
            }
        }
    }
    
    // same with nodes removed
    int removedCount = 0;
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = expandedReference->_roots.constBegin();
            it != expandedReference->_roots.constEnd(); it++) {
        if (!_roots.contains(it.key())) {
            removedCount++;
        }
    }
    out << removedCount;
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = expandedReference->_roots.constBegin();
            it != expandedReference->_roots.constEnd(); it++) {
        if (!_roots.contains(it.key())) {
            out << it.key();
        }
    }
    
    // delete the expanded reference if we had to expand
    if (expandedReference != &reference) {
        delete expandedReference;
    }
}

void MetavoxelData::incrementRootReferenceCounts() {
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = _roots.constBegin(); it != _roots.constEnd(); it++) {
        it.value()->incrementReferenceCount();
    }
}

void MetavoxelData::decrementRootReferenceCounts() {
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = _roots.constBegin(); it != _roots.constEnd(); it++) {
        it.value()->decrementReferenceCount(it.key());
    }
}

MetavoxelNode::MetavoxelNode(const AttributeValue& attributeValue) : _referenceCount(1) {
    _attributeValue = attributeValue.copy();
    for (int i = 0; i < CHILD_COUNT; i++) {
        _children[i] = NULL;
    }
}

MetavoxelNode::MetavoxelNode(const AttributePointer& attribute, const MetavoxelNode* copy) : _referenceCount(1) {
    _attributeValue = attribute->create(copy->_attributeValue);
    for (int i = 0; i < CHILD_COUNT; i++) {
        if ((_children[i] = copy->_children[i])) {
            _children[i]->incrementReferenceCount();
        }
    }
}

void MetavoxelNode::setAttributeValue(const AttributeValue& attributeValue) {
    attributeValue.getAttribute()->destroy(_attributeValue);
    _attributeValue = attributeValue.copy();
    clearChildren(attributeValue.getAttribute());
}

AttributeValue MetavoxelNode::getAttributeValue(const AttributePointer& attribute) const {
    return AttributeValue(attribute, _attributeValue);
}

void MetavoxelNode::mergeChildren(const AttributePointer& attribute) {
    void* childValues[CHILD_COUNT];
    bool allLeaves = true;
    for (int i = 0; i < CHILD_COUNT; i++) {
        childValues[i] = _children[i]->_attributeValue;
        allLeaves &= _children[i]->isLeaf();
    }
    if (attribute->merge(_attributeValue, childValues) && allLeaves) {
        clearChildren(attribute);
    }
}

bool MetavoxelNode::isLeaf() const {
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (_children[i]) {
            return false;
        }    
    }
    return true;
}

void MetavoxelNode::read(const AttributePointer& attribute, Bitstream& in) {
    clearChildren(attribute);
    
    bool leaf;
    in >> leaf;
    attribute->read(in, _attributeValue, leaf);
    if (!leaf) {
        for (int i = 0; i < CHILD_COUNT; i++) {
            _children[i] = new MetavoxelNode(attribute);
            _children[i]->read(attribute, in);
        }
    }
}

void MetavoxelNode::write(const AttributePointer& attribute, Bitstream& out) const {
    bool leaf = isLeaf();
    out << leaf;
    attribute->write(out, _attributeValue, leaf);
    if (!leaf) {
        for (int i = 0; i < CHILD_COUNT; i++) {
            _children[i]->write(attribute, out);
        }
    }
}

void MetavoxelNode::readDelta(const AttributePointer& attribute, const MetavoxelNode& reference, Bitstream& in) {
    clearChildren(attribute);

    bool leaf;
    in >> leaf;
    attribute->readDelta(in, _attributeValue, reference._attributeValue, leaf);
    if (!leaf) {
        if (reference.isLeaf()) {
            for (int i = 0; i < CHILD_COUNT; i++) {
                _children[i] = new MetavoxelNode(attribute);
                _children[i]->read(attribute, in);
            }
        } else {
            for (int i = 0; i < CHILD_COUNT; i++) {
                bool changed;
                in >> changed;
                if (changed) {
                    _children[i] = new MetavoxelNode(attribute);
                    _children[i]->readDelta(attribute, *reference._children[i], in);
                } else {
                    _children[i] = reference._children[i];
                    _children[i]->incrementReferenceCount();
                }
            }
        }  
    }
}

void MetavoxelNode::writeDelta(const AttributePointer& attribute, const MetavoxelNode& reference, Bitstream& out) const {
    bool leaf = isLeaf();
    out << leaf;
    attribute->writeDelta(out, _attributeValue, reference._attributeValue, leaf);
    if (!leaf) {
        if (reference.isLeaf()) {
            for (int i = 0; i < CHILD_COUNT; i++) {
                _children[i]->write(attribute, out);
            }
        } else {
            for (int i = 0; i < CHILD_COUNT; i++) {
                if (_children[i] == reference._children[i]) {
                    out << false;
                } else {
                    out << true;
                    _children[i]->writeDelta(attribute, *reference._children[i], out);
                }
            }
        }
    }
}

void MetavoxelNode::decrementReferenceCount(const AttributePointer& attribute) {
    if (--_referenceCount == 0) {
        destroy(attribute);
        delete this;
    }
}

void MetavoxelNode::destroy(const AttributePointer& attribute) {
    attribute->destroy(_attributeValue);
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (_children[i]) {
            _children[i]->decrementReferenceCount(attribute);
        }
    }
}

void MetavoxelNode::clearChildren(const AttributePointer& attribute) {
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (_children[i]) {
            _children[i]->decrementReferenceCount(attribute);
            _children[i] = NULL;
        }
    }
}

MetavoxelVisitor::MetavoxelVisitor(const QVector<AttributePointer>& inputs, const QVector<AttributePointer>& outputs) :
    _inputs(inputs),
    _outputs(outputs) {
}

MetavoxelVisitor::~MetavoxelVisitor() {
}

void MetavoxelVisitor::prepare() {
    // nothing by default
}

SpannerVisitor::SpannerVisitor(const QVector<AttributePointer>& spannerInputs, const QVector<AttributePointer>& inputs,
        const QVector<AttributePointer>& outputs) :
    MetavoxelVisitor(inputs + spannerInputs, outputs),
    _spannerInputCount(spannerInputs.size()) {
}

void SpannerVisitor::prepare() {
    Spanner::incrementVisit();
}

bool SpannerVisitor::visit(MetavoxelInfo& info) {
    for (int i = _inputs.size() - _spannerInputCount; i < _inputs.size(); i++) {
        foreach (const SharedObjectPointer& object, info.inputValues.at(i).getInlineValue<SharedObjectSet>()) {
            Spanner* spanner = static_cast<Spanner*>(object.data());
            if (spanner->testAndSetVisited()) {
                visit(spanner);
            }
        }
    }
    return !info.isLeaf;
}

DefaultMetavoxelGuide::DefaultMetavoxelGuide() {
}

void DefaultMetavoxelGuide::guide(MetavoxelVisitation& visitation) {
    visitation.info.isLeaf = visitation.allInputNodesLeaves();
    bool keepGoing = visitation.visitor.visit(visitation.info);
    for (int i = 0; i < visitation.outputNodes.size(); i++) {
        OwnedAttributeValue& value = visitation.info.outputValues[i];
        if (!value.getAttribute()) {
            continue;
        }
        MetavoxelNode*& node = visitation.outputNodes[i];
        if (node && node->isLeaf() && value.getAttribute()->equal(value.getValue(), node->getAttributeValue())) {
            // "set" to same value; disregard
            value = AttributeValue();
        } else {
            node = new MetavoxelNode(value);
        }
    }
    if (!keepGoing) {
        return;
    }
    MetavoxelVisitation nextVisitation = { &visitation, visitation.visitor,
        QVector<MetavoxelNode*>(visitation.inputNodes.size()), QVector<MetavoxelNode*>(visitation.outputNodes.size()),
        { glm::vec3(), visitation.info.size * 0.5f, QVector<AttributeValue>(visitation.inputNodes.size()),
            QVector<OwnedAttributeValue>(visitation.outputNodes.size()) } };
    for (int i = 0; i < MetavoxelNode::CHILD_COUNT; i++) {
        for (int j = 0; j < visitation.inputNodes.size(); j++) {
            MetavoxelNode* node = visitation.inputNodes.at(j);
            MetavoxelNode* child = node ? node->getChild(i) : NULL;
            nextVisitation.info.inputValues[j] = ((nextVisitation.inputNodes[j] = child)) ?
                child->getAttributeValue(visitation.info.inputValues[j].getAttribute()) :
                    visitation.info.inputValues[j];
        }
        for (int j = 0; j < visitation.outputNodes.size(); j++) {
            MetavoxelNode* node = visitation.outputNodes.at(j);
            MetavoxelNode* child = node ? node->getChild(i) : NULL;
            nextVisitation.outputNodes[j] = child;
        }
        nextVisitation.info.minimum = visitation.info.minimum + glm::vec3(
            (i & X_MAXIMUM_FLAG) ? nextVisitation.info.size : 0.0f,
            (i & Y_MAXIMUM_FLAG) ? nextVisitation.info.size : 0.0f,
            (i & Z_MAXIMUM_FLAG) ? nextVisitation.info.size : 0.0f);
        static_cast<MetavoxelGuide*>(nextVisitation.info.inputValues.last().getInlineValue<
            SharedObjectPointer>().data())->guide(nextVisitation);
        for (int j = 0; j < nextVisitation.outputNodes.size(); j++) {
            OwnedAttributeValue& value = nextVisitation.info.outputValues[j];
            if (!value.getAttribute()) {
                continue;
            }
            // replace the child
            OwnedAttributeValue& parentValue = visitation.info.outputValues[j];
            if (!parentValue.getAttribute()) {
                // shallow-copy the parent node on first change
                parentValue = value;
                MetavoxelNode*& node = visitation.outputNodes[j];
                if (node) {
                    node = new MetavoxelNode(value.getAttribute(), node);
                } else {
                    // create leaf with inherited value
                    node = new MetavoxelNode(visitation.getInheritedOutputValue(j));
                }
            }
            MetavoxelNode* node = visitation.outputNodes.at(j);
            MetavoxelNode* child = node->getChild(i);
            if (child) {
                child->decrementReferenceCount(value.getAttribute());
            } else {
                // it's a leaf; we need to split it up
                AttributeValue nodeValue = node->getAttributeValue(value.getAttribute());
                for (int k = 1; k < MetavoxelNode::CHILD_COUNT; k++) {
                    node->setChild((i + k) % MetavoxelNode::CHILD_COUNT, new MetavoxelNode(nodeValue));
                }
            }
            node->setChild(i, nextVisitation.outputNodes.at(j));
            value = AttributeValue();
        }
    }
    for (int i = 0; i < visitation.outputNodes.size(); i++) {
        OwnedAttributeValue& value = visitation.info.outputValues[i];
        if (value.getAttribute()) {
            MetavoxelNode* node = visitation.outputNodes.at(i);
            node->mergeChildren(value.getAttribute());
            value = node->getAttributeValue(value.getAttribute()); 
        }
    }
}

ThrobbingMetavoxelGuide::ThrobbingMetavoxelGuide() : _rate(10.0) {
}

void ThrobbingMetavoxelGuide::guide(MetavoxelVisitation& visitation) {
    AttributePointer colorAttribute = AttributeRegistry::getInstance()->getColorAttribute();
    for (int i = 0; i < visitation.info.inputValues.size(); i++) {
        AttributeValue& attributeValue = visitation.info.inputValues[i]; 
        if (attributeValue.getAttribute() == colorAttribute) {
            QRgb base = attributeValue.getInlineValue<QRgb>();
            double seconds = QDateTime::currentMSecsSinceEpoch() / 1000.0;
            double amplitude = sin(_rate * seconds) * 0.5 + 0.5;
            attributeValue.setInlineValue<QRgb>(qRgba(qRed(base) * amplitude, qGreen(base) * amplitude,
                qBlue(base) * amplitude, qAlpha(base)));
        }
    }
    
    DefaultMetavoxelGuide::guide(visitation);
}

static QScriptValue getAttributes(QScriptEngine* engine, ScriptedMetavoxelGuide* guide,
        const QVector<AttributePointer>& attributes) {
    
    QScriptValue attributesValue = engine->newArray(attributes.size());
    for (int i = 0; i < attributes.size(); i++) {
        attributesValue.setProperty(i, engine->newQObject(attributes.at(i).data(), QScriptEngine::QtOwnership,
            QScriptEngine::PreferExistingWrapperObject));
    }
    return attributesValue;
}

QScriptValue ScriptedMetavoxelGuide::getInputs(QScriptContext* context, QScriptEngine* engine) {
    ScriptedMetavoxelGuide* guide = static_cast<ScriptedMetavoxelGuide*>(context->callee().data().toVariant().value<void*>());
    return getAttributes(engine, guide, guide->_visitation->visitor.getInputs());
}

QScriptValue ScriptedMetavoxelGuide::getOutputs(QScriptContext* context, QScriptEngine* engine) {
    ScriptedMetavoxelGuide* guide = static_cast<ScriptedMetavoxelGuide*>(context->callee().data().toVariant().value<void*>());
    return getAttributes(engine, guide, guide->_visitation->visitor.getOutputs());
}

QScriptValue ScriptedMetavoxelGuide::visit(QScriptContext* context, QScriptEngine* engine) {
    ScriptedMetavoxelGuide* guide = static_cast<ScriptedMetavoxelGuide*>(context->callee().data().toVariant().value<void*>());

    // start with the basics, including inherited attribute values
    QScriptValue infoValue = context->argument(0);
    QScriptValue minimum = infoValue.property(guide->_minimumHandle);
    MetavoxelInfo info = {
        glm::vec3(minimum.property(0).toNumber(), minimum.property(1).toNumber(), minimum.property(2).toNumber()),
        infoValue.property(guide->_sizeHandle).toNumber(), guide->_visitation->info.inputValues,
        guide->_visitation->info.outputValues, infoValue.property(guide->_isLeafHandle).toBool() };
    
    // extract and convert the values provided by the script
    QScriptValue inputValues = infoValue.property(guide->_inputValuesHandle);
    const QVector<AttributePointer>& inputs = guide->_visitation->visitor.getInputs();
    for (int i = 0; i < inputs.size(); i++) {
        QScriptValue attributeValue = inputValues.property(i);
        if (attributeValue.isValid()) {
            info.inputValues[i] = AttributeValue(inputs.at(i),
                inputs.at(i)->createFromScript(attributeValue, engine));
        }
    }
    
    QScriptValue result = guide->_visitation->visitor.visit(info);
    
    // destroy any created values
    for (int i = 0; i < inputs.size(); i++) {
        if (inputValues.property(i).isValid()) {
            info.inputValues[i].getAttribute()->destroy(info.inputValues[i].getValue());
        }
    }
    
    return result;
}

ScriptedMetavoxelGuide::ScriptedMetavoxelGuide() {
}

void ScriptedMetavoxelGuide::guide(MetavoxelVisitation& visitation) {
    QScriptValue guideFunction;
    if (_guideFunction) {
        guideFunction = _guideFunction->getValue();    
        
    } else if (_url.isValid()) {
        _guideFunction = ScriptCache::getInstance()->getValue(_url);
        guideFunction = _guideFunction->getValue();
    }
    if (!guideFunction.isValid()) {
        // before we load, just use the default behavior
        DefaultMetavoxelGuide::guide(visitation);
        return;
    }
    QScriptEngine* engine = guideFunction.engine();
    if (!_minimumHandle.isValid()) {
        _minimumHandle = engine->toStringHandle("minimum");
        _sizeHandle = engine->toStringHandle("size");
        _inputValuesHandle = engine->toStringHandle("inputValues");
        _outputValuesHandle = engine->toStringHandle("outputValues");
        _isLeafHandle = engine->toStringHandle("isLeaf");
        _getInputsFunction = engine->newFunction(getInputs, 0);
        _getOutputsFunction = engine->newFunction(getOutputs, 0);
        _visitFunction = engine->newFunction(visit, 1);
        _info = engine->newObject();
        _minimum = engine->newArray(3);
        
        _arguments.clear();
        _arguments.append(engine->newObject());
        QScriptValue visitor = engine->newObject();
        visitor.setProperty("getInputs", _getInputsFunction);
        visitor.setProperty("getOutputs", _getOutputsFunction);
        visitor.setProperty("visit", _visitFunction);
        _arguments[0].setProperty("visitor", visitor);
        _arguments[0].setProperty("info", _info);
        _info.setProperty(_minimumHandle, _minimum);
    }
    QScriptValue data = engine->newVariant(QVariant::fromValue<void*>(this));
    _getInputsFunction.setData(data);
    _visitFunction.setData(data);
    _minimum.setProperty(0, visitation.info.minimum.x);
    _minimum.setProperty(1, visitation.info.minimum.y);
    _minimum.setProperty(2, visitation.info.minimum.z);
    _info.setProperty(_sizeHandle, visitation.info.size);
    _info.setProperty(_isLeafHandle, visitation.info.isLeaf);
    _visitation = &visitation;
    guideFunction.call(QScriptValue(), _arguments);
    if (engine->hasUncaughtException()) {
        qDebug() << "Script error: " << engine->uncaughtException().toString();
    }
}

void ScriptedMetavoxelGuide::setURL(const ParameterizedURL& url) {
    _url = url;
    _guideFunction.reset();
    _minimumHandle = QScriptString();
}

bool MetavoxelVisitation::allInputNodesLeaves() const {
    foreach (MetavoxelNode* node, inputNodes) {
        if (node != NULL && !node->isLeaf()) {
            return false;
        }
    }
    return true;
}

AttributeValue MetavoxelVisitation::getInheritedOutputValue(int index) const {
    for (const MetavoxelVisitation* visitation = previous; visitation != NULL; visitation = visitation->previous) {
        MetavoxelNode* node = visitation->outputNodes.at(index);
        if (node) {
            return node->getAttributeValue(visitor.getOutputs().at(index));
        }
    }
    return AttributeValue(visitor.getOutputs().at(index));
}

const float DEFAULT_GRANULARITY = 0.01f;

Spanner::Spanner() :
    _granularity(DEFAULT_GRANULARITY),
    _lastVisit(0),
    _renderer(NULL) {
}

void Spanner::setBounds(const Box& bounds) {
    if (_bounds == bounds) {
        return;
    }
    emit boundsWillChange();
    emit boundsChanged(_bounds = bounds);
}

bool Spanner::testAndSetVisited() {
    if (_lastVisit == _visit) {
        return false;
    }
    _lastVisit = _visit;
    return true;
}

SpannerRenderer* Spanner::getRenderer() {
    if (!_renderer) {
        QByteArray className = getRendererClassName();
        const QMetaObject* metaObject = Bitstream::getMetaObject(className);
        if (!metaObject) {
            qDebug() << "Unknown class name:" << className;
            metaObject = &SpannerRenderer::staticMetaObject;
        }
        _renderer = static_cast<SpannerRenderer*>(metaObject->newInstance());
        _renderer->setParent(this);
        _renderer->init(this);
    }
    return _renderer;
}

QByteArray Spanner::getRendererClassName() const {
    return "SpannerRendererer";
}

int Spanner::_visit = 0;

SpannerRenderer::SpannerRenderer() {
}

void SpannerRenderer::init(Spanner* spanner) {
    // nothing by default
}

void SpannerRenderer::simulate(float deltaTime) {
    // nothing by default
}

void SpannerRenderer::render(float alpha) {
    // nothing by default
}

Transformable::Transformable() : _scale(1.0f) {
}

void Transformable::setTranslation(const glm::vec3& translation) {
    if (_translation != translation) {
        emit translationChanged(_translation = translation);
    }
}

void Transformable::setRotation(const glm::vec3& rotation) {
    if (_rotation != rotation) {
        emit rotationChanged(_rotation = rotation);
    }
}

void Transformable::setScale(float scale) {
    if (_scale != scale) {
        emit scaleChanged(_scale = scale);
    }
}

StaticModel::StaticModel() {
}

void StaticModel::setURL(const QUrl& url) {
    if (_url != url) {
        emit urlChanged(_url = url);
    }
}

QByteArray StaticModel::getRendererClassName() const {
    return "StaticModelRenderer";
}
