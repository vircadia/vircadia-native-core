//
//  MetavoxelData.cpp
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDateTime>
#include <QDebugStateSaver>
#include <QScriptEngine>
#include <QThread>
#include <QtDebug>

#include <glm/gtx/transform.hpp>

#include <GeometryUtil.h>

#include "MetavoxelData.h"
#include "MetavoxelUtil.h"
#include "ScriptCache.h"

REGISTER_META_OBJECT(MetavoxelGuide)
REGISTER_META_OBJECT(DefaultMetavoxelGuide)
REGISTER_META_OBJECT(ScriptedMetavoxelGuide)
REGISTER_META_OBJECT(ThrobbingMetavoxelGuide)
REGISTER_META_OBJECT(MetavoxelRenderer)
REGISTER_META_OBJECT(DefaultMetavoxelRenderer)
REGISTER_META_OBJECT(Spanner)
REGISTER_META_OBJECT(Sphere)
REGISTER_META_OBJECT(Cuboid)
REGISTER_META_OBJECT(StaticModel)

static int metavoxelDataTypeId = registerSimpleMetaType<MetavoxelData>();

MetavoxelLOD::MetavoxelLOD(const glm::vec3& position, float threshold) :
    position(position),
    threshold(threshold) {
}

bool MetavoxelLOD::shouldSubdivide(const glm::vec3& minimum, float size, float multiplier) const {
    return size >= glm::distance(position, minimum + glm::vec3(size, size, size) * 0.5f) * threshold * multiplier;
}

bool MetavoxelLOD::becameSubdivided(const glm::vec3& minimum, float size,
        const MetavoxelLOD& reference, float multiplier) const {
    if (position == reference.position && threshold >= reference.threshold) {
        return false; // first off, nothing becomes subdivided if it doesn't change
    }
    if (!shouldSubdivide(minimum, size, multiplier)) {
        return false; // this one must be subdivided
    }
    // TODO: find some way of culling subtrees that can't possibly contain subdivided nodes
    return true;
}

bool MetavoxelLOD::becameSubdividedOrCollapsed(const glm::vec3& minimum, float size,
        const MetavoxelLOD& reference, float multiplier) const {
    if (position == reference.position && threshold == reference.threshold) {
        return false; // first off, nothing becomes subdivided or collapsed if it doesn't change
    }
    if (!(shouldSubdivide(minimum, size, multiplier) || reference.shouldSubdivide(minimum, size, multiplier))) {
        return false; // this one or the reference must be subdivided
    }
    // TODO: find some way of culling subtrees that can't possibly contain subdivided or collapsed nodes
    return true;
}

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
    visitor.prepare(this);

    // start with the root values/defaults (plus the guide attribute)
    const QVector<AttributePointer>& inputs = visitor.getInputs();
    const QVector<AttributePointer>& outputs = visitor.getOutputs();
    MetavoxelVisitation& firstVisitation = visitor.acquireVisitation();
    firstVisitation.info.minimum = getMinimum();
    firstVisitation.info.size = _size;
    for (int i = 0; i < inputs.size(); i++) {
        const AttributePointer& input = inputs.at(i);
        MetavoxelNode* node = _roots.value(input);
        firstVisitation.inputNodes[i] = node;
        firstVisitation.info.inputValues[i] = node ? node->getAttributeValue(input) : input;
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
        value = AttributeValue();
    }
    visitor.releaseVisitation();
}

void MetavoxelData::guideToDifferent(const MetavoxelData& other, MetavoxelVisitor& visitor) {
    // if the other data is smaller, we need to expand it to compare
    const MetavoxelData* expandedOther = &other;
    if (_size > other._size) {
        MetavoxelData* expanded = new MetavoxelData(other);
        while (expanded->_size < _size) {
            expanded->expand();
        }
        expandedOther = expanded;
    }
    
    // let the visitor know we're about to begin a tour
    visitor.prepare(this);

    // start with the root values/defaults (plus the guide attribute)
    const QVector<AttributePointer>& inputs = visitor.getInputs();
    const QVector<AttributePointer>& outputs = visitor.getOutputs();
    MetavoxelVisitation& firstVisitation = visitor.acquireVisitation();
    firstVisitation.compareNodes.resize(inputs.size() + 1);
    firstVisitation.info.minimum = getMinimum();
    firstVisitation.info.size = _size;
    bool allNodesSame = true;
    for (int i = 0; i < inputs.size(); i++) {
        const AttributePointer& input = inputs.at(i);
        MetavoxelNode* node = _roots.value(input);
        firstVisitation.inputNodes[i] = node;
        firstVisitation.info.inputValues[i] = node ? node->getAttributeValue(input) : input;
        MetavoxelNode* compareNode = expandedOther->_roots.value(input);
        firstVisitation.compareNodes[i] = compareNode;
        allNodesSame &= (node == compareNode);
    }
    AttributePointer guideAttribute = AttributeRegistry::getInstance()->getGuideAttribute();
    MetavoxelNode* node = _roots.value(guideAttribute);
    firstVisitation.inputNodes.last() = node;
    firstVisitation.info.inputValues.last() = node ? node->getAttributeValue(guideAttribute) : guideAttribute;
    MetavoxelNode* compareNode = expandedOther->_roots.value(guideAttribute);
    firstVisitation.compareNodes.last() = compareNode;
    allNodesSame &= (node == compareNode);
    if (!allNodesSame) {
        for (int i = 0; i < outputs.size(); i++) {
            MetavoxelNode* node = _roots.value(outputs.at(i));
            firstVisitation.outputNodes[i] = node;
        }
        static_cast<MetavoxelGuide*>(firstVisitation.info.inputValues.last().getInlineValue<
            SharedObjectPointer>().data())->guideToDifferent(firstVisitation);
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
            value = AttributeValue();
        }
    }
    visitor.releaseVisitation();
    
    // delete the expanded other if we had to expand
    if (expandedOther != &other) {
        delete expandedOther;
    }
}

typedef void (*SpannerUpdateFunction)(SharedObjectSet& set, const SharedObjectPointer& object);

void insertSpanner(SharedObjectSet& set, const SharedObjectPointer& object) {
    set.insert(object);
}

void removeSpanner(SharedObjectSet& set, const SharedObjectPointer& object) {
    set.remove(object);
}

void toggleSpanner(SharedObjectSet& set, const SharedObjectPointer& object) {
    if (!set.remove(object)) {
        set.insert(object);
    }
}

template<SpannerUpdateFunction F> class SpannerUpdateVisitor : public MetavoxelVisitor {
public:
    
    SpannerUpdateVisitor(const AttributePointer& attribute, const Box& bounds,
        float granularity, const SharedObjectPointer& object);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    const AttributePointer& _attribute;
    const Box& _bounds;
    float _longestSide;
    const SharedObjectPointer& _object;
};

template<SpannerUpdateFunction F> SpannerUpdateVisitor<F>::SpannerUpdateVisitor(const AttributePointer& attribute,
        const Box& bounds, float granularity, const SharedObjectPointer& object) :
    MetavoxelVisitor(QVector<AttributePointer>() << attribute, QVector<AttributePointer>() << attribute),
    _attribute(attribute),
    _bounds(bounds),
    _longestSide(qMax(bounds.getLongestSide(), granularity)),
    _object(object) {
}

template<SpannerUpdateFunction F> int SpannerUpdateVisitor<F>::visit(MetavoxelInfo& info) {
    if (!info.getBounds().intersects(_bounds)) {
        return STOP_RECURSION;
    }
    if (info.size > _longestSide) {
        return DEFAULT_ORDER;
    }
    SharedObjectSet set = info.inputValues.at(0).getInlineValue<SharedObjectSet>();
    F(set, _object);
    info.outputValues[0] = AttributeValue(_attribute, encodeInline(set));
    return STOP_RECURSION;
}

void MetavoxelData::insert(const AttributePointer& attribute, const SharedObjectPointer& object) {
    Spanner* spanner = static_cast<Spanner*>(object.data());
    insert(attribute, spanner->getBounds(), spanner->getPlacementGranularity(), object);
}

void MetavoxelData::insert(const AttributePointer& attribute, const Box& bounds,
        float granularity, const SharedObjectPointer& object) {
    // expand to fit the entire bounds
    while (!getBounds().contains(bounds)) {
        expand();
    }
    SpannerUpdateVisitor<insertSpanner> visitor(attribute, bounds, granularity, object);
    guide(visitor);
}

void MetavoxelData::remove(const AttributePointer& attribute, const SharedObjectPointer& object) {
    Spanner* spanner = static_cast<Spanner*>(object.data());
    remove(attribute, spanner->getBounds(), spanner->getPlacementGranularity(), object);
}

void MetavoxelData::remove(const AttributePointer& attribute, const Box& bounds,
        float granularity, const SharedObjectPointer& object) {
    SpannerUpdateVisitor<removeSpanner> visitor(attribute, bounds, granularity, object);
    guide(visitor);
}

void MetavoxelData::toggle(const AttributePointer& attribute, const SharedObjectPointer& object) {
    Spanner* spanner = static_cast<Spanner*>(object.data());
    toggle(attribute, spanner->getBounds(), spanner->getPlacementGranularity(), object);
}

void MetavoxelData::toggle(const AttributePointer& attribute, const Box& bounds,
        float granularity, const SharedObjectPointer& object) {
    SpannerUpdateVisitor<toggleSpanner> visitor(attribute, bounds, granularity, object);
    guide(visitor);
}

void MetavoxelData::replace(const AttributePointer& attribute, const SharedObjectPointer& oldObject,
        const SharedObjectPointer& newObject) {
    Spanner* spanner = static_cast<Spanner*>(oldObject.data());
    replace(attribute, spanner->getBounds(), spanner->getPlacementGranularity(), oldObject, newObject);
}

class SpannerReplaceVisitor : public MetavoxelVisitor {
public:
    
    SpannerReplaceVisitor(const AttributePointer& attribute, const Box& bounds,
        float granularity, const SharedObjectPointer& oldObject, const SharedObjectPointer& newObject);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    const AttributePointer& _attribute;
    const Box& _bounds;
    float _longestSide;
    const SharedObjectPointer& _oldObject;
    const SharedObjectPointer& _newObject;
};

SpannerReplaceVisitor::SpannerReplaceVisitor(const AttributePointer& attribute, const Box& bounds, float granularity,
        const SharedObjectPointer& oldObject, const SharedObjectPointer& newObject) :
    MetavoxelVisitor(QVector<AttributePointer>() << attribute, QVector<AttributePointer>() << attribute),
    _attribute(attribute),
    _bounds(bounds),
    _longestSide(qMax(bounds.getLongestSide(), granularity)),
    _oldObject(oldObject),
    _newObject(newObject) {
}

int SpannerReplaceVisitor::visit(MetavoxelInfo& info) {
    if (!info.getBounds().intersects(_bounds)) {
        return STOP_RECURSION;
    }
    if (info.size > _longestSide) {
        return DEFAULT_ORDER;
    }
    SharedObjectSet set = info.inputValues.at(0).getInlineValue<SharedObjectSet>();
    if (set.remove(_oldObject)) {
        set.insert(_newObject);
    }
    info.outputValues[0] = AttributeValue(_attribute, encodeInline(set));
    return STOP_RECURSION;
}

void MetavoxelData::replace(const AttributePointer& attribute, const Box& bounds, float granularity,
        const SharedObjectPointer& oldObject, const SharedObjectPointer& newObject) {
    Spanner* newSpanner = static_cast<Spanner*>(newObject.data());
    if (bounds != newSpanner->getBounds() || granularity != newSpanner->getPlacementGranularity()) {
        // if the bounds have changed, we must remove and reinsert
        remove(attribute, bounds, granularity, oldObject);
        insert(attribute, newSpanner->getBounds(), newSpanner->getPlacementGranularity(), newObject);
        return;
    }
    SpannerReplaceVisitor visitor(attribute, bounds, granularity, oldObject, newObject);
    guide(visitor);
}

void MetavoxelData::clear(const AttributePointer& attribute) {
    MetavoxelNode* node = _roots.take(attribute);
    if (node) {
        node->decrementReferenceCount(attribute);
    }
}

void MetavoxelData::touch(const AttributePointer& attribute) {
    MetavoxelNode* root = _roots.value(attribute);
    if (root) {
        setRoot(attribute, root->touch(attribute));
    }
}

class FirstRaySpannerIntersectionVisitor : public RaySpannerIntersectionVisitor {
public:
    
    FirstRaySpannerIntersectionVisitor(const glm::vec3& origin, const glm::vec3& direction,
        const AttributePointer& attribute, const MetavoxelLOD& lod);
    
    Spanner* getSpanner() const { return _spanner; }
    float getDistance() const { return _distance; }
    
    virtual bool visitSpanner(Spanner* spanner, float distance);

private:
    
    Spanner* _spanner;
    float _distance;
};

FirstRaySpannerIntersectionVisitor::FirstRaySpannerIntersectionVisitor(
        const glm::vec3& origin, const glm::vec3& direction, const AttributePointer& attribute, const MetavoxelLOD& lod) :
    RaySpannerIntersectionVisitor(origin, direction, QVector<AttributePointer>() << attribute,
        QVector<AttributePointer>(), QVector<AttributePointer>(), QVector<AttributePointer>(), lod),
    _spanner(NULL) {
}

bool FirstRaySpannerIntersectionVisitor::visitSpanner(Spanner* spanner, float distance) {
    _spanner = spanner;
    _distance = distance;
    return false;
}

SharedObjectPointer MetavoxelData::findFirstRaySpannerIntersection(
        const glm::vec3& origin, const glm::vec3& direction, const AttributePointer& attribute,
            float& distance, const MetavoxelLOD& lod) {
    FirstRaySpannerIntersectionVisitor visitor(origin, direction, attribute, lod);
    guide(visitor);
    if (!visitor.getSpanner()) {
        return SharedObjectPointer();
    }
    distance = visitor.getDistance();
    return SharedObjectPointer(visitor.getSpanner());
}

const int X_MAXIMUM_FLAG = 1;
const int Y_MAXIMUM_FLAG = 2;
const int Z_MAXIMUM_FLAG = 4;
const int MAXIMUM_FLAG_MASK = X_MAXIMUM_FLAG | Y_MAXIMUM_FLAG | Z_MAXIMUM_FLAG;

static glm::vec3 getNextMinimum(const glm::vec3& minimum, float nextSize, int index) {
    return minimum + glm::vec3(
        (index & X_MAXIMUM_FLAG) ? nextSize : 0.0f,
        (index & Y_MAXIMUM_FLAG) ? nextSize : 0.0f,
        (index & Z_MAXIMUM_FLAG) ? nextSize : 0.0f);
}

static void setNode(const AttributeValue& value, MetavoxelNode*& node, MetavoxelNode* other, bool blend) {
    if (!blend) {
        // if we're not blending, we can just make a shallow copy
        if (node) {
            node->decrementReferenceCount(value.getAttribute());
        }
        (node = other)->incrementReferenceCount();
        return;
    }
    if (node) {
        MetavoxelNode* oldNode = node;
        node = new MetavoxelNode(value.getAttribute(), oldNode);
        oldNode->decrementReferenceCount(value.getAttribute());
        
    } else {
        node = new MetavoxelNode(value);
    }
    OwnedAttributeValue oldValue = node->getAttributeValue(value.getAttribute());
    node->blendAttributeValues(other->getAttributeValue(value.getAttribute()), oldValue);
    if (!other->isLeaf()) {
        for (int i = 0; i < MetavoxelNode::CHILD_COUNT; i++) {
            MetavoxelNode* child = node->getChild(i);
            setNode(oldValue, child, other->getChild(i), true);
            node->setChild(i, child);
        }
    }
    node->mergeChildren(value.getAttribute());
}

static void setNode(const AttributeValue& value, MetavoxelNode*& node, const glm::vec3& minimum, float size,
        MetavoxelNode* other, const glm::vec3& otherMinimum, float otherSize, bool blend) {
    if (otherSize >= size) {
        setNode(value, node, other, blend);
        return;
    }
    if (node) {
        MetavoxelNode* oldNode = node;
        node = new MetavoxelNode(value.getAttribute(), oldNode);
        oldNode->decrementReferenceCount(value.getAttribute());
        
    } else {
        node = new MetavoxelNode(value);
    }
    int index = 0;
    float otherHalfSize = otherSize * 0.5f;
    float nextSize = size * 0.5f;
    if (otherMinimum.x + otherHalfSize >= minimum.x + nextSize) {
        index |= X_MAXIMUM_FLAG;
    }
    if (otherMinimum.y + otherHalfSize >= minimum.y + nextSize) {
        index |= Y_MAXIMUM_FLAG;
    }
    if (otherMinimum.z + otherHalfSize >= minimum.z + nextSize) {
        index |= Z_MAXIMUM_FLAG;
    }
    if (node->isLeaf()) {
        for (int i = 1; i < MetavoxelNode::CHILD_COUNT; i++) {
            node->setChild((index + i) % MetavoxelNode::CHILD_COUNT, new MetavoxelNode(
                node->getAttributeValue(value.getAttribute())));
        }
    }
    MetavoxelNode* nextNode = node->getChild(index);
    setNode(node->getAttributeValue(value.getAttribute()), nextNode, getNextMinimum(minimum, nextSize, index),
        nextSize, other, otherMinimum, otherSize, blend);
    node->setChild(index, nextNode);
    node->mergeChildren(value.getAttribute());
}

void MetavoxelData::set(const glm::vec3& minimum, const MetavoxelData& data, bool blend) {
    // expand to fit the entire data
    Box bounds(minimum, minimum + glm::vec3(data.getSize(), data.getSize(), data.getSize()));
    while (!getBounds().contains(bounds)) {
        expand();
    }
    
    // set/mix each attribute separately
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = data._roots.constBegin();
            it != data._roots.constEnd(); it++) {
        MetavoxelNode*& root = _roots[it.key()];
        setNode(it.key(), root, getMinimum(), getSize(), it.value(), minimum, data.getSize(), blend);
        if (root->isLeaf() && root->getAttributeValue(it.key()).isDefault()) {
            _roots.remove(it.key());
            root->decrementReferenceCount(it.key());
        }
    }
}

void MetavoxelData::expand() {
    for (QHash<AttributePointer, MetavoxelNode*>::iterator it = _roots.begin(); it != _roots.end(); it++) {
        MetavoxelNode* newNode = it.key()->expandMetavoxelRoot(*it.value());
        it.value()->decrementReferenceCount(it.key());
        it.value() = newNode;
    }
    _size *= 2.0f;
}

void MetavoxelData::read(Bitstream& in, const MetavoxelLOD& lod) {
    // clear out any existing roots
    decrementRootReferenceCounts();
    _roots.clear();

    in >> _size;
    
    // read in the new roots
    forever {
        AttributePointer attribute;
        in >> attribute;
        if (!attribute) {
            break;
        }
        MetavoxelStreamBase base = { attribute, in, lod, lod };
        MetavoxelStreamState state = { base, getMinimum(), _size };
        attribute->readMetavoxelRoot(*this, state);
    }
}

void MetavoxelData::write(Bitstream& out, const MetavoxelLOD& lod) const {
    out << _size;
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = _roots.constBegin(); it != _roots.constEnd(); it++) {
        out << it.key();
        MetavoxelStreamBase base = { it.key(), out, lod, lod };
        MetavoxelStreamState state = { base, getMinimum(), _size };
        it.key()->writeMetavoxelRoot(*it.value(), state);
    }
    out << AttributePointer();
}

void MetavoxelData::readDelta(const MetavoxelData& reference, const MetavoxelLOD& referenceLOD, 
        Bitstream& in, const MetavoxelLOD& lod) {
    // shallow copy the reference
    *this = reference;

    QHash<AttributePointer, MetavoxelNode*> remainingRoots = _roots;
    
    bool changed;
    in >> changed;
    if (changed) {
        bool sizeChanged;
        in >> sizeChanged;
        if (sizeChanged) {
            float size;
            in >> size;
            while (_size < size) {
                expand();
            }
        }
    
        glm::vec3 minimum = getMinimum();
        forever {
            AttributePointer attribute;
            in >> attribute;
            if (!attribute) {
                break;
            }
            MetavoxelStreamBase base = { attribute, in, lod, referenceLOD };
            MetavoxelStreamState state = { base, minimum, _size };
            MetavoxelNode* oldRoot = _roots.value(attribute);
            if (oldRoot) {
                bool changed;
                in >> changed;
                if (changed) {
                    oldRoot->incrementReferenceCount();
                    attribute->readMetavoxelDelta(*this, *oldRoot, state);
                    oldRoot->decrementReferenceCount(attribute);    
                } else {
                    attribute->readMetavoxelSubdivision(*this, state);
                }
                remainingRoots.remove(attribute);
                
            } else {
                attribute->readMetavoxelRoot(*this, state);
            } 
        }
        
        forever {
            AttributePointer attribute;
            in >> attribute;
            if (!attribute) {
                break;
            }
            _roots.take(attribute)->decrementReferenceCount(attribute);
            remainingRoots.remove(attribute);
        }
    }
    
    // read subdivisions for the remaining roots if there's any chance of a collapse
    if (!(lod.position == referenceLOD.position && lod.threshold <= referenceLOD.threshold)) {
        glm::vec3 minimum = getMinimum();
        for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = remainingRoots.constBegin();
                it != remainingRoots.constEnd(); it++) {
            MetavoxelStreamBase base = { it.key(), in, lod, referenceLOD };
            MetavoxelStreamState state = { base, minimum, _size };
            it.key()->readMetavoxelSubdivision(*this, state);
        }
    }
}

void MetavoxelData::writeDelta(const MetavoxelData& reference, const MetavoxelLOD& referenceLOD,
        Bitstream& out, const MetavoxelLOD& lod) const {
    // first things first: there might be no change whatsoever
    glm::vec3 minimum = getMinimum();
    bool becameSubdivided = lod.becameSubdivided(minimum, _size, referenceLOD);
    if (_size == reference._size && _roots == reference._roots && !becameSubdivided) {
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

    // write the added/changed/subdivided roots
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = _roots.constBegin(); it != _roots.constEnd(); it++) {
        MetavoxelNode* referenceRoot = expandedReference->_roots.value(it.key());
        MetavoxelStreamBase base = { it.key(), out, lod, referenceLOD };
        MetavoxelStreamState state = { base, minimum, _size };
        if (it.value() != referenceRoot || becameSubdivided) {
            out << it.key();    
            if (referenceRoot) {
                if (it.value() == referenceRoot) {
                    out << false;
                    it.key()->writeMetavoxelSubdivision(*it.value(), state);
                } else {
                    out << true;
                    it.key()->writeMetavoxelDelta(*it.value(), *referenceRoot, state);
                }
            } else {
                it.key()->writeMetavoxelRoot(*it.value(), state);
            }
        }
    }
    out << AttributePointer();
    
    // same with nodes removed
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = expandedReference->_roots.constBegin();
            it != expandedReference->_roots.constEnd(); it++) {
        if (!_roots.contains(it.key())) {
            out << it.key();
        }
    }
    out << AttributePointer();
    
    // delete the expanded reference if we had to expand
    if (expandedReference != &reference) {
        delete expandedReference;
    }
}

void MetavoxelData::setRoot(const AttributePointer& attribute, MetavoxelNode* root) {
    MetavoxelNode*& rootReference = _roots[attribute];
    if (rootReference) {
        rootReference->decrementReferenceCount(attribute);
    }
    rootReference = root;
}

MetavoxelNode* MetavoxelData::createRoot(const AttributePointer& attribute) {
    MetavoxelNode* root = new MetavoxelNode(attribute);
    setRoot(attribute, root);
    return root;
}

bool MetavoxelData::deepEquals(const MetavoxelData& other, const MetavoxelLOD& lod) const {
    if (_size != other._size) {
        return false;
    }
    if (_roots.size() != other._roots.size()) {
        return false;
    }
    glm::vec3 minimum = getMinimum();
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = _roots.constBegin(); it != _roots.constEnd(); it++) {
        MetavoxelNode* otherNode = other._roots.value(it.key());
        if (!(otherNode && it.key()->metavoxelRootsEqual(*it.value(), *otherNode, minimum, _size, lod))) {
            return false;
        }
    }
    return true;
}

void MetavoxelData::countNodes(int& internal, int& leaves, const MetavoxelLOD& lod) const {
    glm::vec3 minimum = getMinimum();
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = _roots.constBegin(); it != _roots.constEnd(); it++) {
        it.value()->countNodes(it.key(), minimum, _size, lod, internal, leaves);
    }
}

void MetavoxelData::dumpStats(QDebug debug) const {
    QDebugStateSaver saver(debug);
    debug.nospace() << "[size=" << _size << ", roots=[";
    int totalInternal = 0, totalLeaves = 0;
    glm::vec3 minimum = getMinimum();
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = _roots.constBegin(); it != _roots.constEnd(); it++) {
        if (it != _roots.constBegin()) {
            debug << ", ";
        }
        debug << it.key()->getName() << " (" << it.key()->metaObject()->className() << "): ";
        int internal = 0, leaves = 0;
        it.value()->countNodes(it.key(), minimum, _size, MetavoxelLOD(), internal, leaves);
        debug << internal << " internal, " << leaves << " leaves, " << (internal + leaves) << " total";
        totalInternal += internal;
        totalLeaves += leaves;
    }
    debug << "], totalInternal=" << totalInternal << ", totalLeaves=" << totalLeaves <<
        ", grandTotal=" << (totalInternal + totalLeaves) << "]";
}

bool MetavoxelData::operator==(const MetavoxelData& other) const {
    return _size == other._size && _roots == other._roots;
}

bool MetavoxelData::operator!=(const MetavoxelData& other) const {
    return _size != other._size || _roots != other._roots;
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

Bitstream& operator<<(Bitstream& out, const MetavoxelData& data) {
    data.write(out);
    return out;
}

Bitstream& operator>>(Bitstream& in, MetavoxelData& data) {
    data.read(in);
    return in;
}

template<> void Bitstream::writeDelta(const MetavoxelData& value, const MetavoxelData& reference) {
    value.writeDelta(reference, MetavoxelLOD(), *this, MetavoxelLOD());
}

template<> void Bitstream::readDelta(MetavoxelData& value, const MetavoxelData& reference) {
    value.readDelta(reference, MetavoxelLOD(), *this, MetavoxelLOD());
}

bool MetavoxelStreamState::shouldSubdivide() const {
    return base.lod.shouldSubdivide(minimum, size, base.attribute->getLODThresholdMultiplier());
}

bool MetavoxelStreamState::shouldSubdivideReference() const {
    return base.referenceLOD.shouldSubdivide(minimum, size, base.attribute->getLODThresholdMultiplier());
}

bool MetavoxelStreamState::becameSubdivided() const {
    return base.lod.becameSubdivided(minimum, size, base.referenceLOD, base.attribute->getLODThresholdMultiplier());
}

bool MetavoxelStreamState::becameSubdividedOrCollapsed() const {
    return base.lod.becameSubdividedOrCollapsed(minimum, size, base.referenceLOD, base.attribute->getLODThresholdMultiplier());
}

void MetavoxelStreamState::setMinimum(const glm::vec3& lastMinimum, int index) {
    minimum = getNextMinimum(lastMinimum, size, index);
}

int MetavoxelNode::getOppositeChildIndex(int index) {
    return index ^ MAXIMUM_FLAG_MASK;
}

MetavoxelNode::MetavoxelNode(const AttributeValue& attributeValue, const MetavoxelNode* copyChildren) :
        _referenceCount(1) {

    _attributeValue = attributeValue.copy();
    if (copyChildren) {
        for (int i = 0; i < CHILD_COUNT; i++) {
            if ((_children[i] = copyChildren->_children[i])) {
                _children[i]->incrementReferenceCount();
            }
        }
    } else {
        for (int i = 0; i < CHILD_COUNT; i++) {
            _children[i] = NULL;
        }
    }
}

MetavoxelNode::MetavoxelNode(const AttributePointer& attribute, const MetavoxelNode* copy) :
        _referenceCount(1) {
        
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
}

void MetavoxelNode::blendAttributeValues(const AttributeValue& source, const AttributeValue& dest) {
    source.getAttribute()->destroy(_attributeValue);
    _attributeValue = source.getAttribute()->blend(source.getValue(), dest.getValue());
}

AttributeValue MetavoxelNode::getAttributeValue(const AttributePointer& attribute) const {
    return AttributeValue(attribute, _attributeValue);
}

void MetavoxelNode::mergeChildren(const AttributePointer& attribute, bool postRead) {
    if (isLeaf()) {
        return;
    }
    void* childValues[CHILD_COUNT];
    bool allLeaves = true;
    for (int i = 0; i < CHILD_COUNT; i++) {
        childValues[i] = _children[i]->_attributeValue;
        allLeaves &= _children[i]->isLeaf();
    }
    if (attribute->merge(_attributeValue, childValues, postRead) && allLeaves && !postRead) {
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

void MetavoxelNode::read(MetavoxelStreamState& state) {
    clearChildren(state.base.attribute);
    
    if (!state.shouldSubdivide()) {
        state.base.attribute->read(state.base.stream, _attributeValue, true);
        return;
    }
    bool leaf;
    state.base.stream >> leaf;
    state.base.attribute->read(state.base.stream, _attributeValue, leaf);
    if (!leaf) {
        MetavoxelStreamState nextState = { state.base, glm::vec3(), state.size * 0.5f };
        for (int i = 0; i < CHILD_COUNT; i++) {
            nextState.setMinimum(state.minimum, i);
            _children[i] = new MetavoxelNode(state.base.attribute);
            _children[i]->read(nextState);
        }
        mergeChildren(state.base.attribute, true);
    }
}

void MetavoxelNode::write(MetavoxelStreamState& state) const {
    if (!state.shouldSubdivide()) {
        state.base.attribute->write(state.base.stream, _attributeValue, true);
        return;
    }
    bool leaf = isLeaf();
    state.base.stream << leaf;
    state.base.attribute->write(state.base.stream, _attributeValue, leaf);
    if (!leaf) {
        MetavoxelStreamState nextState = { state.base, glm::vec3(), state.size * 0.5f };
        for (int i = 0; i < CHILD_COUNT; i++) {
            nextState.setMinimum(state.minimum, i);
            _children[i]->write(nextState);
        }
    }
}

void MetavoxelNode::readDelta(const MetavoxelNode& reference, MetavoxelStreamState& state) {
    clearChildren(state.base.attribute);

    if (!state.shouldSubdivide()) {
        state.base.attribute->readDelta(state.base.stream, _attributeValue, reference._attributeValue, true);
        return;
    }
    bool leaf;
    state.base.stream >> leaf;
    state.base.attribute->readDelta(state.base.stream, _attributeValue, reference._attributeValue, leaf);
    if (!leaf) {
        MetavoxelStreamState nextState = { state.base, glm::vec3(), state.size * 0.5f };
        if (reference.isLeaf() || !state.shouldSubdivideReference()) {
            for (int i = 0; i < CHILD_COUNT; i++) {
                nextState.setMinimum(state.minimum, i);
                _children[i] = new MetavoxelNode(state.base.attribute);
                _children[i]->read(nextState);
            }
        } else {
            for (int i = 0; i < CHILD_COUNT; i++) {
                nextState.setMinimum(state.minimum, i);
                bool changed;
                state.base.stream >> changed;
                if (changed) {    
                    _children[i] = new MetavoxelNode(state.base.attribute);
                    _children[i]->readDelta(*reference._children[i], nextState);
                } else {
                    if (nextState.becameSubdividedOrCollapsed()) {
                        _children[i] = reference._children[i]->readSubdivision(nextState);
                        if (_children[i] == reference._children[i]) {
                            _children[i]->incrementReferenceCount();
                        }
                    } else {
                        _children[i] = reference._children[i];
                        _children[i]->incrementReferenceCount();    
                    }
                }
            }
        }
        mergeChildren(state.base.attribute, true);
    }
}

void MetavoxelNode::writeDelta(const MetavoxelNode& reference, MetavoxelStreamState& state) const {
    if (!state.shouldSubdivide()) {
        state.base.attribute->writeDelta(state.base.stream, _attributeValue, reference._attributeValue, true);
        return;    
    }
    bool leaf = isLeaf();
    state.base.stream << leaf;
    state.base.attribute->writeDelta(state.base.stream, _attributeValue, reference._attributeValue, leaf);
    if (!leaf) {
        MetavoxelStreamState nextState = { state.base, glm::vec3(), state.size * 0.5f };
        if (reference.isLeaf() || !state.shouldSubdivideReference()) {
            for (int i = 0; i < CHILD_COUNT; i++) {
                nextState.setMinimum(state.minimum, i);
                _children[i]->write(nextState);
            }
        } else {
            for (int i = 0; i < CHILD_COUNT; i++) {
                nextState.setMinimum(state.minimum, i);
                if (_children[i] == reference._children[i]) {
                    state.base.stream << false;
                    if (nextState.becameSubdivided()) {
                        _children[i]->writeSubdivision(nextState);
                    }
                } else {                    
                    state.base.stream << true;
                    _children[i]->writeDelta(*reference._children[i], nextState);
                }
            }
        }
    }
}

MetavoxelNode* MetavoxelNode::readSubdivision(MetavoxelStreamState& state) {
    if (state.shouldSubdivide()) {
        if (!state.shouldSubdivideReference()) {
            bool leaf;
            state.base.stream >> leaf;
            if (leaf) {
                return isLeaf() ? this : new MetavoxelNode(getAttributeValue(state.base.attribute));
                
            } else {
                MetavoxelNode* newNode = new MetavoxelNode(getAttributeValue(state.base.attribute));
                MetavoxelStreamState nextState = { state.base, glm::vec3(), state.size * 0.5f };
                for (int i = 0; i < CHILD_COUNT; i++) {
                    nextState.setMinimum(state.minimum, i);
                    newNode->_children[i] = new MetavoxelNode(state.base.attribute);
                    newNode->_children[i]->readSubdivided(nextState, state, _attributeValue);
                }
                return newNode;
            }
        } else if (!isLeaf()) {
            MetavoxelNode* node = this;
            MetavoxelStreamState nextState = { state.base, glm::vec3(), state.size * 0.5f };
            for (int i = 0; i < CHILD_COUNT; i++) {
                nextState.setMinimum(state.minimum, i);
                if (nextState.becameSubdividedOrCollapsed()) {
                    MetavoxelNode* child = _children[i]->readSubdivision(nextState);
                    if (child != _children[i]) {
                        if (node == this) {
                            node = new MetavoxelNode(state.base.attribute, this);
                        }
                        node->_children[i] = child;   
                        _children[i]->decrementReferenceCount(state.base.attribute);
                    }
                }
            }
            if (node != this) {
                node->mergeChildren(state.base.attribute, true);
            }
            return node;
        }
    } else if (!isLeaf()) {
        return new MetavoxelNode(getAttributeValue(state.base.attribute));
    }
    return this;
}

void MetavoxelNode::writeSubdivision(MetavoxelStreamState& state) const {
    bool leaf = isLeaf();
    if (!state.shouldSubdivideReference()) {
        state.base.stream << leaf;
        if (!leaf) {
            MetavoxelStreamState nextState = { state.base, glm::vec3(), state.size * 0.5f };
            for (int i = 0; i < CHILD_COUNT; i++) {
                nextState.setMinimum(state.minimum, i);
                _children[i]->writeSubdivided(nextState, state, _attributeValue);
            }
        }
    } else if (!leaf) {
        MetavoxelStreamState nextState = { state.base, glm::vec3(), state.size * 0.5f };
        for (int i = 0; i < CHILD_COUNT; i++) {
            nextState.setMinimum(state.minimum, i);
            if (nextState.becameSubdivided()) {
                _children[i]->writeSubdivision(nextState);
            }
        }
    }
}

void MetavoxelNode::readSubdivided(MetavoxelStreamState& state, const MetavoxelStreamState& ancestorState,
        void* ancestorValue) {
    clearChildren(state.base.attribute);
    
    if (!state.shouldSubdivide()) {
        state.base.attribute->readSubdivided(state, _attributeValue, ancestorState, ancestorValue, true);
        return;
    }
    bool leaf;
    state.base.stream >> leaf;
    state.base.attribute->readSubdivided(state, _attributeValue, ancestorState, ancestorValue, leaf);
    if (!leaf) {
        MetavoxelStreamState nextState = { state.base, glm::vec3(), state.size * 0.5f };
        for (int i = 0; i < CHILD_COUNT; i++) {
            nextState.setMinimum(state.minimum, i);
            _children[i] = new MetavoxelNode(state.base.attribute);
            _children[i]->readSubdivided(nextState, ancestorState, ancestorValue);
        }
        mergeChildren(state.base.attribute, true);
    }
}

void MetavoxelNode::writeSubdivided(MetavoxelStreamState& state, const MetavoxelStreamState& ancestorState,
        void* ancestorValue) const {
    if (!state.shouldSubdivide()) {
        state.base.attribute->writeSubdivided(state, _attributeValue, ancestorState, ancestorValue, true);
        return;
    }
    bool leaf = isLeaf();
    state.base.stream << leaf;
    state.base.attribute->writeSubdivided(state, _attributeValue, ancestorState, ancestorValue, leaf);
    if (!leaf) {
        MetavoxelStreamState nextState = { state.base, glm::vec3(), state.size * 0.5f };
        for (int i = 0; i < CHILD_COUNT; i++) {
            nextState.setMinimum(state.minimum, i);
            _children[i]->writeSubdivided(nextState, ancestorState, ancestorValue);
        }
    }
}

void MetavoxelNode::writeSpanners(MetavoxelStreamState& state) const {
    foreach (const SharedObjectPointer& object, decodeInline<SharedObjectSet>(_attributeValue)) {
        if (static_cast<Spanner*>(object.data())->testAndSetVisited(state.base.visit)) {
            state.base.stream << object;
        }
    }
    if (!state.shouldSubdivide() || isLeaf()) {
        return;
    }
    MetavoxelStreamState nextState = { state.base, glm::vec3(), state.size * 0.5f };
    for (int i = 0; i < CHILD_COUNT; i++) {
        nextState.setMinimum(state.minimum, i);
        _children[i]->writeSpanners(nextState);
    }
}

void MetavoxelNode::writeSpannerDelta(const MetavoxelNode& reference, MetavoxelStreamState& state) const {
    SharedObjectSet oldSet = decodeInline<SharedObjectSet>(reference.getAttributeValue());
    SharedObjectSet newSet = decodeInline<SharedObjectSet>(_attributeValue);
    foreach (const SharedObjectPointer& object, oldSet) {
        if (static_cast<Spanner*>(object.data())->testAndSetVisited(state.base.visit) && !newSet.contains(object)) {
            state.base.stream << object;
        }
    }
    foreach (const SharedObjectPointer& object, newSet) {
        if (static_cast<Spanner*>(object.data())->testAndSetVisited(state.base.visit) && !oldSet.contains(object)) {
            state.base.stream << object;
        }
    }
    if (isLeaf() || !state.shouldSubdivide()) {
        if (!reference.isLeaf() && state.shouldSubdivideReference()) {
            MetavoxelStreamState nextState = { state.base, glm::vec3(), state.size * 0.5f };
            for (int i = 0; i < CHILD_COUNT; i++) {
                nextState.setMinimum(state.minimum, i);
                reference._children[i]->writeSpanners(nextState);
            }
        }
        return;
    }
    MetavoxelStreamState nextState = { state.base, glm::vec3(), state.size * 0.5f };
    if (reference.isLeaf() || !state.shouldSubdivideReference()) {
        for (int i = 0; i < CHILD_COUNT; i++) {
            nextState.setMinimum(state.minimum, i);
            _children[i]->writeSpanners(nextState);
        }
        return;
    }
    for (int i = 0; i < CHILD_COUNT; i++) {
        nextState.setMinimum(state.minimum, i);
        if (_children[i] != reference._children[i]) {
            _children[i]->writeSpannerDelta(*reference._children[i], nextState);
            
        } else if (nextState.becameSubdivided()) {
            _children[i]->writeSpannerSubdivision(nextState);
        }
    }
}

void MetavoxelNode::writeSpannerSubdivision(MetavoxelStreamState& state) const {
    if (!isLeaf()) {
        MetavoxelStreamState nextState = { state.base, glm::vec3(), state.size * 0.5f };
        if (!state.shouldSubdivideReference()) {
            for (int i = 0; i < CHILD_COUNT; i++) {
                nextState.setMinimum(state.minimum, i);
                _children[i]->writeSpanners(nextState);
            }
        } else {
            for (int i = 0; i < CHILD_COUNT; i++) {
                nextState.setMinimum(state.minimum, i);
                if (nextState.becameSubdivided()) {
                    _children[i]->writeSpannerSubdivision(nextState);
                }
            }
        }
    }
}

void MetavoxelNode::decrementReferenceCount(const AttributePointer& attribute) {
    if (!_referenceCount.deref()) {
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

bool MetavoxelNode::clearChildren(const AttributePointer& attribute) {
    bool cleared = false;
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (_children[i]) {
            _children[i]->decrementReferenceCount(attribute);
            _children[i] = NULL;
            cleared = true;
        }
    }
    return cleared;
}

bool MetavoxelNode::deepEquals(const AttributePointer& attribute, const MetavoxelNode& other,
        const glm::vec3& minimum, float size, const MetavoxelLOD& lod) const {
    if (!attribute->deepEqual(_attributeValue, other._attributeValue)) {
        return false;
    }
    if (!lod.shouldSubdivide(minimum, size, attribute->getLODThresholdMultiplier())) {
        return true;
    }
    bool leaf = isLeaf(), otherLeaf = other.isLeaf();
    if (leaf && otherLeaf) {
        return true;
    }
    if (leaf || otherLeaf) {
        return false;
    }
    float nextSize = size * 0.5f;
    for (int i = 0; i < CHILD_COUNT; i++) {
        glm::vec3 nextMinimum = getNextMinimum(minimum, nextSize, i);
        if (!_children[i]->deepEquals(attribute, *(other._children[i]), nextMinimum, nextSize, lod)) {
            return false;
        }
    }
    return true;
}

void MetavoxelNode::getSpanners(const AttributePointer& attribute, const glm::vec3& minimum,
        float size, const MetavoxelLOD& lod, SharedObjectSet& results) const {
    results.unite(decodeInline<SharedObjectSet>(_attributeValue));
    if (isLeaf() || !lod.shouldSubdivide(minimum, size, attribute->getLODThresholdMultiplier())) {
        return;
    }
    float nextSize = size * 0.5f;
    for (int i = 0; i < CHILD_COUNT; i++) {
        _children[i]->getSpanners(attribute, getNextMinimum(minimum, nextSize, i), nextSize, lod, results);
    }
}

void MetavoxelNode::countNodes(const AttributePointer& attribute, const glm::vec3& minimum,
        float size, const MetavoxelLOD& lod, int& internal, int& leaves) const {
    if (isLeaf() || !lod.shouldSubdivide(minimum, size, attribute->getLODThresholdMultiplier())) {
        leaves++;
        return;
    }
    internal++;
    float nextSize = size * 0.5f;
    for (int i = 0; i < CHILD_COUNT; i++) {
        _children[i]->countNodes(attribute, getNextMinimum(minimum, nextSize, i), nextSize, lod, internal, leaves);
    }
}

MetavoxelNode* MetavoxelNode::touch(const AttributePointer& attribute) const {
    MetavoxelNode* node = new MetavoxelNode(getAttributeValue(attribute));
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (_children[i]) {
            node->setChild(i, _children[i]->touch(attribute));
        }
    }
    return node;
}

MetavoxelInfo::MetavoxelInfo(MetavoxelInfo* parentInfo, int inputValuesSize, int outputValuesSize) :
    parentInfo(parentInfo),
    inputValues(inputValuesSize),
    outputValues(outputValuesSize) {
}

MetavoxelInfo::MetavoxelInfo() {
}

int MetavoxelVisitor::encodeOrder(int first, int second, int third, int fourth,
        int fifth, int sixth, int seventh, int eighth) {
    return first | (second << 3) | (third << 6) | (fourth << 9) |
        (fifth << 12) | (sixth << 15) | (seventh << 18) | (eighth << 21);
}

class IndexDistance {
public:
    int index;
    float distance;
};

bool operator<(const IndexDistance& first, const IndexDistance& second) {
    return first.distance < second.distance;
}

int MetavoxelVisitor::encodeOrder(const glm::vec3& direction) {
    QList<IndexDistance> indexDistances;
    for (int i = 0; i < MetavoxelNode::CHILD_COUNT; i++) {
        IndexDistance indexDistance = { i, glm::dot(direction, getNextMinimum(glm::vec3(), 1.0f, i)) };
        indexDistances.append(indexDistance);
    }
    qStableSort(indexDistances);
    return encodeOrder(indexDistances.at(0).index, indexDistances.at(1).index, indexDistances.at(2).index,
        indexDistances.at(3).index, indexDistances.at(4).index, indexDistances.at(5).index,
        indexDistances.at(6).index, indexDistances.at(7).index);
}

const int ORDER_ELEMENT_BITS = 3;
const int ORDER_ELEMENT_MASK = (1 << ORDER_ELEMENT_BITS) - 1;

int MetavoxelVisitor::encodeRandomOrder() {
    // see http://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle#The_.22inside-out.22_algorithm
    int order = 0;
    int randomValues = rand();
    for (int i = 0, iShift = 0; i < MetavoxelNode::CHILD_COUNT; i++, iShift += ORDER_ELEMENT_BITS) {
        int j = (randomValues >> iShift) % (i + 1);
        int jShift = j * ORDER_ELEMENT_BITS;
        if (j != i) {
            int jValue = (order >> jShift) & ORDER_ELEMENT_MASK;
            order |= (jValue << iShift);
        }
        order = (order & ~(ORDER_ELEMENT_MASK << jShift)) | (i << jShift);
    }
    return order;
}

const int MetavoxelVisitor::DEFAULT_ORDER = encodeOrder(0, 1, 2, 3, 4, 5, 6, 7);
const int MetavoxelVisitor::STOP_RECURSION = 0;
const int MetavoxelVisitor::SHORT_CIRCUIT = -1;
const int MetavoxelVisitor::ALL_NODES = 1 << 24;
const int MetavoxelVisitor::ALL_NODES_REST = 1 << 25;

MetavoxelVisitor::MetavoxelVisitor(const QVector<AttributePointer>& inputs,
        const QVector<AttributePointer>& outputs, const MetavoxelLOD& lod) :
    _inputs(inputs),
    _outputs(outputs),
    _lod(lod),
    _minimumLODThresholdMultiplier(FLT_MAX),
    _depth(-1) {
    
    // find the minimum LOD threshold multiplier over all attributes
    foreach (const AttributePointer& attribute, _inputs) {
        _minimumLODThresholdMultiplier = qMin(attribute->getLODThresholdMultiplier(), _minimumLODThresholdMultiplier);
    }
    foreach (const AttributePointer& attribute, _outputs) {
        _minimumLODThresholdMultiplier = qMin(attribute->getLODThresholdMultiplier(), _minimumLODThresholdMultiplier);
    }
}

MetavoxelVisitor::~MetavoxelVisitor() {
}

void MetavoxelVisitor::prepare(MetavoxelData* data) {
    _data = data;
}

bool MetavoxelVisitor::postVisit(MetavoxelInfo& info) {
    return false;
}

MetavoxelVisitation& MetavoxelVisitor::acquireVisitation() {
    if (++_depth >= _visitations.size()) {
        _visitations.append(MetavoxelVisitation(_depth == 0 ? NULL : &_visitations[_depth - 1],
            this, _inputs.size() + 1, _outputs.size()));
    }
    return _visitations[_depth];
}

SpannerVisitor::SpannerVisitor(const QVector<AttributePointer>& spannerInputs, const QVector<AttributePointer>& spannerMasks,
        const QVector<AttributePointer>& inputs, const QVector<AttributePointer>& outputs,
        const MetavoxelLOD& lod, int order) :
    MetavoxelVisitor(inputs + spannerInputs + spannerMasks, outputs, lod),
    _spannerInputCount(spannerInputs.size()),
    _spannerMaskCount(spannerMasks.size()),
    _order(order) {
}

void SpannerVisitor::prepare(MetavoxelData* data) {
    MetavoxelVisitor::prepare(data);
    _visit = Spanner::getAndIncrementNextVisit();
}

int SpannerVisitor::visit(MetavoxelInfo& info) {
    for (int end = _inputs.size() - _spannerMaskCount, i = end - _spannerInputCount, j = end; i < end; i++, j++) {
        foreach (const SharedObjectPointer& object, info.inputValues.at(i).getInlineValue<SharedObjectSet>()) {
            Spanner* spanner = static_cast<Spanner*>(object.data());
            if (!(spanner->isMasked() && j < _inputs.size()) && spanner->testAndSetVisited(_visit) &&
                    !visit(spanner, glm::vec3(), 0.0f)) {
                return SHORT_CIRCUIT;
            }
        }
    }
    if (!info.isLeaf) {
        return _order;
    }
    for (int i = _inputs.size() - _spannerMaskCount; i < _inputs.size(); i++) {
        float maskValue = info.inputValues.at(i).getInlineValue<float>();
        if (maskValue < 0.5f) {
            const MetavoxelInfo* nextInfo = &info;
            do {
                foreach (const SharedObjectPointer& object, nextInfo->inputValues.at(
                        i - _spannerInputCount).getInlineValue<SharedObjectSet>()) {
                    Spanner* spanner = static_cast<Spanner*>(object.data());
                    if (spanner->isMasked() && !visit(spanner, info.minimum, info.size)) {
                        return SHORT_CIRCUIT;
                    }
                }                
            } while ((nextInfo = nextInfo->parentInfo));
        }
    }
    return STOP_RECURSION;
}

RayIntersectionVisitor::RayIntersectionVisitor(const glm::vec3& origin, const glm::vec3& direction,
        const QVector<AttributePointer>& inputs, const QVector<AttributePointer>& outputs, const MetavoxelLOD& lod) :
    MetavoxelVisitor(inputs, outputs, lod),
    _origin(origin),
    _direction(direction),
    _order(encodeOrder(direction)) {
}

int RayIntersectionVisitor::visit(MetavoxelInfo& info) {
    float distance;
    if (!info.getBounds().findRayIntersection(_origin, _direction, distance)) {
        return STOP_RECURSION;
    }
    return visit(info, distance);
}

RaySpannerIntersectionVisitor::RaySpannerIntersectionVisitor(const glm::vec3& origin, const glm::vec3& direction,
        const QVector<AttributePointer>& spannerInputs, const QVector<AttributePointer>& spannerMasks,
        const QVector<AttributePointer>& inputs, const QVector<AttributePointer>& outputs, const MetavoxelLOD& lod) :
    RayIntersectionVisitor(origin, direction, inputs + spannerInputs + spannerMasks, outputs, lod),
    _spannerInputCount(spannerInputs.size()),
    _spannerMaskCount(spannerMasks.size()) {
}

void RaySpannerIntersectionVisitor::prepare(MetavoxelData* data) {
    MetavoxelVisitor::prepare(data);
    _visit = Spanner::getAndIncrementNextVisit();
}

class SpannerDistance {
public:
    Spanner* spanner;
    float distance;
};

bool operator<(const SpannerDistance& first, const SpannerDistance& second) {
    return first.distance < second.distance;
}

int RaySpannerIntersectionVisitor::visit(MetavoxelInfo& info, float distance) {
    QVarLengthArray<SpannerDistance, 4> spannerDistances;
    for (int end = _inputs.size() - _spannerMaskCount, i = end - _spannerInputCount, j = end; i < end; i++, j++) {
        foreach (const SharedObjectPointer& object, info.inputValues.at(i).getInlineValue<SharedObjectSet>()) {
            Spanner* spanner = static_cast<Spanner*>(object.data());
            if (!(spanner->isMasked() && j < _inputs.size()) && spanner->testAndSetVisited(_visit)) {
                SpannerDistance spannerDistance = { spanner };
                if (spanner->findRayIntersection(_origin, _direction, glm::vec3(), 0.0f, spannerDistance.distance)) {
                    spannerDistances.append(spannerDistance);
                }
            }
        }
        qStableSort(spannerDistances);
        foreach (const SpannerDistance& spannerDistance, spannerDistances) {
            if (!visitSpanner(spannerDistance.spanner, spannerDistance.distance)) {
                return SHORT_CIRCUIT;
            }
        }
    }
    if (!info.isLeaf) {
        return _order;
    }
    for (int i = _inputs.size() - _spannerMaskCount; i < _inputs.size(); i++) {
        float maskValue = info.inputValues.at(i).getInlineValue<float>();
        if (maskValue < 0.5f) {
            const MetavoxelInfo* nextInfo = &info;
            do {
                foreach (const SharedObjectPointer& object, nextInfo->inputValues.at(
                        i - _spannerInputCount).getInlineValue<SharedObjectSet>()) {
                    Spanner* spanner = static_cast<Spanner*>(object.data());
                    if (spanner->isMasked()) {
                        SpannerDistance spannerDistance = { spanner };
                        if (spanner->findRayIntersection(_origin, _direction,
                                info.minimum, info.size, spannerDistance.distance)) {
                            spannerDistances.append(spannerDistance);
                        }
                    }
                }                
            } while ((nextInfo = nextInfo->parentInfo));
            
            qStableSort(spannerDistances);
            foreach (const SpannerDistance& spannerDistance, spannerDistances) {
                if (!visitSpanner(spannerDistance.spanner, spannerDistance.distance)) {
                    return SHORT_CIRCUIT;
                }
            }
        }
    }
    return STOP_RECURSION;
}

bool MetavoxelGuide::guideToDifferent(MetavoxelVisitation& visitation) {
    return guide(visitation);
}

DefaultMetavoxelGuide::DefaultMetavoxelGuide() {
}

static inline bool defaultGuideToChildren(MetavoxelVisitation& visitation, float lodBase, int encodedOrder) {
    MetavoxelVisitation& nextVisitation = visitation.visitor->acquireVisitation();
    nextVisitation.info.size = visitation.info.size * 0.5f;
    for (int i = 0; i < MetavoxelNode::CHILD_COUNT; i++) {
        // the encoded order tells us the child indices for each iteration
        int index = encodedOrder & ORDER_ELEMENT_MASK;
        encodedOrder >>= ORDER_ELEMENT_BITS;
        for (int j = 0; j < visitation.inputNodes.size(); j++) {
            MetavoxelNode* node = visitation.inputNodes.at(j);
            const AttributeValue& parentValue = visitation.info.inputValues.at(j);
            MetavoxelNode* child = (node && (visitation.info.size >= lodBase *
                parentValue.getAttribute()->getLODThresholdMultiplier())) ? node->getChild(index) : NULL;
            nextVisitation.info.inputValues[j] = ((nextVisitation.inputNodes[j] = child)) ?
                child->getAttributeValue(parentValue.getAttribute()) : parentValue.getAttribute()->inherit(parentValue);
        }
        for (int j = 0; j < visitation.outputNodes.size(); j++) {
            MetavoxelNode* node = visitation.outputNodes.at(j);
            MetavoxelNode* child = (node && (visitation.info.size >= lodBase *
                visitation.visitor->getOutputs().at(j)->getLODThresholdMultiplier())) ? node->getChild(index) : NULL;
            nextVisitation.outputNodes[j] = child;
        }
        nextVisitation.info.minimum = getNextMinimum(visitation.info.minimum, nextVisitation.info.size, index);
        if (!static_cast<MetavoxelGuide*>(nextVisitation.info.inputValues.last().getInlineValue<
                SharedObjectPointer>().data())->guide(nextVisitation)) {
            visitation.visitor->releaseVisitation();
            return false;
        }
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
                    node = new MetavoxelNode(value.getAttribute()->inherit(visitation.getInheritedOutputValue(j)));
                }
            }
            MetavoxelNode* node = visitation.outputNodes.at(j);
            MetavoxelNode* child = node->getChild(index);
            if (child) {
                child->decrementReferenceCount(value.getAttribute());
            } else {
                // it's a leaf; we need to split it up
                AttributeValue nodeValue = value.getAttribute()->inherit(node->getAttributeValue(value.getAttribute()));
                for (int k = 1; k < MetavoxelNode::CHILD_COUNT; k++) {
                    node->setChild((index + k) % MetavoxelNode::CHILD_COUNT, new MetavoxelNode(nodeValue));
                }
            }
            node->setChild(index, nextVisitation.outputNodes.at(j));
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
    visitation.visitor->releaseVisitation();
    visitation.info.outputValues.swap(nextVisitation.info.outputValues);
    bool changed = visitation.visitor->postVisit(visitation.info);
    visitation.info.outputValues.swap(nextVisitation.info.outputValues);
    if (changed) {
        for (int i = 0; i < visitation.outputNodes.size(); i++) {
            OwnedAttributeValue& newValue = nextVisitation.info.outputValues[i];
            if (!newValue.getAttribute()) {
                continue;
            }
            OwnedAttributeValue& value = visitation.info.outputValues[i];
            MetavoxelNode*& node = visitation.outputNodes[i];
            if (value.getAttribute()) {
                node->setAttributeValue(value = newValue);
                
            } else if (!(node && node->isLeaf() && newValue.getAttribute()->equal(
                    newValue.getValue(), node->getAttributeValue()))) {
                node = newValue.getAttribute()->createMetavoxelNode(value = newValue, node);    
            }
            newValue = AttributeValue();
        }
    }
    return true;
}

bool DefaultMetavoxelGuide::guide(MetavoxelVisitation& visitation) {
    // save the core of the LOD calculation; we'll reuse it to determine whether to subdivide each attribute
    float lodBase = glm::distance(visitation.visitor->getLOD().position, visitation.info.getCenter()) *
        visitation.visitor->getLOD().threshold;
    visitation.info.isLODLeaf = (visitation.info.size < lodBase * visitation.visitor->getMinimumLODThresholdMultiplier());
    visitation.info.isLeaf = visitation.info.isLODLeaf || visitation.allInputNodesLeaves();
    int encodedOrder = visitation.visitor->visit(visitation.info);
    if (encodedOrder == MetavoxelVisitor::SHORT_CIRCUIT) {
        return false;
    }
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
            node = value.getAttribute()->createMetavoxelNode(value, node);
        }
    }
    if (encodedOrder == MetavoxelVisitor::STOP_RECURSION) {
        return true;
    }
    return (encodedOrder == MetavoxelVisitor::STOP_RECURSION || defaultGuideToChildren(visitation, lodBase, encodedOrder));
}

bool DefaultMetavoxelGuide::guideToDifferent(MetavoxelVisitation& visitation) {
    // save the core of the LOD calculation; we'll reuse it to determine whether to subdivide each attribute
    float lodBase = glm::distance(visitation.visitor->getLOD().position, visitation.info.getCenter()) *
        visitation.visitor->getLOD().threshold;
    visitation.info.isLODLeaf = (visitation.info.size < lodBase * visitation.visitor->getMinimumLODThresholdMultiplier());
    visitation.info.isLeaf = visitation.info.isLODLeaf || visitation.allInputNodesLeaves();
    int encodedOrder = visitation.visitor->visit(visitation.info);
    if (encodedOrder == MetavoxelVisitor::SHORT_CIRCUIT) {
        return false;
    }
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
            node = value.getAttribute()->createMetavoxelNode(value, node);
        }
    }
    if (encodedOrder == MetavoxelVisitor::STOP_RECURSION) {
        return true;
    }
    if (encodedOrder & MetavoxelVisitor::ALL_NODES_REST) {
        return defaultGuideToChildren(visitation, lodBase, encodedOrder);
    }
    bool onlyVisitDifferent = !(encodedOrder & MetavoxelVisitor::ALL_NODES);
    MetavoxelVisitation& nextVisitation = visitation.visitor->acquireVisitation();
    nextVisitation.compareNodes.resize(visitation.compareNodes.size());
    nextVisitation.info.size = visitation.info.size * 0.5f;
    for (int i = 0; i < MetavoxelNode::CHILD_COUNT; i++) {
        // the encoded order tells us the child indices for each iteration
        int index = encodedOrder & ORDER_ELEMENT_MASK;
        encodedOrder >>= ORDER_ELEMENT_BITS;
        bool allNodesSame = onlyVisitDifferent;
        for (int j = 0; j < visitation.inputNodes.size(); j++) {
            MetavoxelNode* node = visitation.inputNodes.at(j);
            const AttributeValue& parentValue = visitation.info.inputValues.at(j);
            bool expand = (visitation.info.size >= lodBase * parentValue.getAttribute()->getLODThresholdMultiplier());
            MetavoxelNode* child = (node && expand) ? node->getChild(index) : NULL;
            nextVisitation.info.inputValues[j] = ((nextVisitation.inputNodes[j] = child)) ?
                child->getAttributeValue(parentValue.getAttribute()) : parentValue.getAttribute()->inherit(parentValue);
            MetavoxelNode* compareNode = visitation.compareNodes.at(j);
            MetavoxelNode* compareChild = (compareNode && expand) ? compareNode->getChild(index) : NULL;
            nextVisitation.compareNodes[j] = compareChild;
            allNodesSame &= (child == compareChild);
        }
        if (allNodesSame) {
            continue;
        }
        for (int j = 0; j < visitation.outputNodes.size(); j++) {
            MetavoxelNode* node = visitation.outputNodes.at(j);
            MetavoxelNode* child = (node && (visitation.info.size >= lodBase *
                visitation.visitor->getOutputs().at(j)->getLODThresholdMultiplier())) ? node->getChild(index) : NULL;
            nextVisitation.outputNodes[j] = child;
        }
        nextVisitation.info.minimum = getNextMinimum(visitation.info.minimum, nextVisitation.info.size, index);
        if (!static_cast<MetavoxelGuide*>(nextVisitation.info.inputValues.last().getInlineValue<
                SharedObjectPointer>().data())->guideToDifferent(nextVisitation)) {
            visitation.visitor->releaseVisitation();
            return false;
        }
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
                    node = new MetavoxelNode(value.getAttribute()->inherit(visitation.getInheritedOutputValue(j)));
                }
            }
            MetavoxelNode* node = visitation.outputNodes.at(j);
            MetavoxelNode* child = node->getChild(index);
            if (child) {
                child->decrementReferenceCount(value.getAttribute());
            } else {
                // it's a leaf; we need to split it up
                AttributeValue nodeValue = value.getAttribute()->inherit(node->getAttributeValue(value.getAttribute()));
                for (int k = 1; k < MetavoxelNode::CHILD_COUNT; k++) {
                    node->setChild((index + k) % MetavoxelNode::CHILD_COUNT, new MetavoxelNode(nodeValue));
                }
            }
            node->setChild(index, nextVisitation.outputNodes.at(j));
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
    visitation.visitor->releaseVisitation();
    visitation.info.outputValues.swap(nextVisitation.info.outputValues);
    bool changed = visitation.visitor->postVisit(visitation.info);
    visitation.info.outputValues.swap(nextVisitation.info.outputValues);
    if (changed) {
        for (int i = 0; i < visitation.outputNodes.size(); i++) {
            OwnedAttributeValue& newValue = nextVisitation.info.outputValues[i];
            if (!newValue.getAttribute()) {
                continue;
            }
            OwnedAttributeValue& value = visitation.info.outputValues[i];
            MetavoxelNode*& node = visitation.outputNodes[i];
            if (value.getAttribute()) {
                node->setAttributeValue(value = newValue);
                
            } else if (!(node && node->isLeaf() && newValue.getAttribute()->equal(
                    newValue.getValue(), node->getAttributeValue()))) {
                node = newValue.getAttribute()->createMetavoxelNode(value = newValue, node);    
            }
            newValue = AttributeValue();
        }
    }
    return true;
}

ThrobbingMetavoxelGuide::ThrobbingMetavoxelGuide() : _rate(10.0) {
}

bool ThrobbingMetavoxelGuide::guide(MetavoxelVisitation& visitation) {
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
    
    return DefaultMetavoxelGuide::guide(visitation);
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
    return getAttributes(engine, guide, guide->_visitation->visitor->getInputs());
}

QScriptValue ScriptedMetavoxelGuide::getOutputs(QScriptContext* context, QScriptEngine* engine) {
    ScriptedMetavoxelGuide* guide = static_cast<ScriptedMetavoxelGuide*>(context->callee().data().toVariant().value<void*>());
    return getAttributes(engine, guide, guide->_visitation->visitor->getOutputs());
}

QScriptValue ScriptedMetavoxelGuide::visit(QScriptContext* context, QScriptEngine* engine) {
    ScriptedMetavoxelGuide* guide = static_cast<ScriptedMetavoxelGuide*>(context->callee().data().toVariant().value<void*>());

    // start with the basics, including inherited attribute values
    QScriptValue infoValue = context->argument(0);
    QScriptValue minimum = infoValue.property(guide->_minimumHandle);
    MetavoxelInfo info(NULL, 0, 0);
    info.inputValues = guide->_visitation->info.inputValues;
    info.outputValues = guide->_visitation->info.outputValues;
    info.minimum = glm::vec3(minimum.property(0).toNumber(), minimum.property(1).toNumber(), minimum.property(2).toNumber());
    info.size = (float)infoValue.property(guide->_sizeHandle).toNumber();
    info.isLeaf = infoValue.property(guide->_isLeafHandle).toBool();
    
    // extract and convert the values provided by the script
    QScriptValue inputValues = infoValue.property(guide->_inputValuesHandle);
    const QVector<AttributePointer>& inputs = guide->_visitation->visitor->getInputs();
    for (int i = 0; i < inputs.size(); i++) {
        QScriptValue attributeValue = inputValues.property(i);
        if (attributeValue.isValid()) {
            info.inputValues[i] = AttributeValue(inputs.at(i),
                inputs.at(i)->createFromScript(attributeValue, engine));
        }
    }
    
    QScriptValue result = guide->_visitation->visitor->visit(info);
    
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

bool ScriptedMetavoxelGuide::guide(MetavoxelVisitation& visitation) {
    QScriptValue guideFunction;
    if (_guideFunction) {
        guideFunction = _guideFunction->getValue();    
        
    } else if (_url.isValid()) {
        _guideFunction = ScriptCache::getInstance()->getValue(_url);
        guideFunction = _guideFunction->getValue();
    }
    if (!guideFunction.isValid()) {
        // before we load, just use the default behavior
        return DefaultMetavoxelGuide::guide(visitation);
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
    return true;
}

void ScriptedMetavoxelGuide::setURL(const ParameterizedURL& url) {
    _url = url;
    _guideFunction.reset();
    _minimumHandle = QScriptString();
}

MetavoxelVisitation::MetavoxelVisitation(MetavoxelVisitation* previous,
        MetavoxelVisitor* visitor, int inputNodesSize, int outputNodesSize) :
    previous(previous),
    visitor(visitor),
    inputNodes(inputNodesSize),
    outputNodes(outputNodesSize),
    info(previous ? &previous->info : NULL, inputNodesSize, outputNodesSize) {
}

MetavoxelVisitation::MetavoxelVisitation() {
}

bool MetavoxelVisitation::allInputNodesLeaves() const {
    foreach (MetavoxelNode* node, inputNodes) {
        if (node && !node->isLeaf()) {
            return false;
        }
    }
    return true;
}

AttributeValue MetavoxelVisitation::getInheritedOutputValue(int index) const {
    for (const MetavoxelVisitation* visitation = previous; visitation; visitation = visitation->previous) {
        MetavoxelNode* node = visitation->outputNodes.at(index);
        if (node) {
            return node->getAttributeValue(visitor->getOutputs().at(index));
        }
    }
    return AttributeValue(visitor->getOutputs().at(index));
}

MetavoxelRenderer::MetavoxelRenderer() :
    _implementation(NULL) {
}

MetavoxelRendererImplementation* MetavoxelRenderer::getImplementation() {
    QMutexLocker locker(&_implementationMutex);
    if (!_implementation) {
        QByteArray className = getImplementationClassName();
        const QMetaObject* metaObject = Bitstream::getMetaObject(className);
        if (!metaObject) {
            qDebug() << "Unknown class name:" << className;
            metaObject = &MetavoxelRendererImplementation::staticMetaObject;
        }
        _implementation = static_cast<MetavoxelRendererImplementation*>(metaObject->newInstance());
        connect(this, &QObject::destroyed, _implementation, &QObject::deleteLater);
        _implementation->init(this);
    }
    return _implementation;
}

MetavoxelRendererImplementation::MetavoxelRendererImplementation() {
}

void MetavoxelRendererImplementation::init(MetavoxelRenderer* renderer) {
    _renderer = renderer;
}

void MetavoxelRendererImplementation::augment(MetavoxelData& data, const MetavoxelData& previous,
        MetavoxelInfo& info, const MetavoxelLOD& lod) {
    // nothing by default
}

void MetavoxelRendererImplementation::simulate(MetavoxelData& data, float deltaTime,
        MetavoxelInfo& info, const MetavoxelLOD& lod) {
    // nothing by default
}

void MetavoxelRendererImplementation::render(MetavoxelData& data, MetavoxelInfo& info, const MetavoxelLOD& lod) {
    // nothing by default
}

QByteArray MetavoxelRenderer::getImplementationClassName() const {
    return "MetavoxelRendererImplementation";
}

DefaultMetavoxelRenderer::DefaultMetavoxelRenderer() {
}

QByteArray DefaultMetavoxelRenderer::getImplementationClassName() const {
    return "DefaultMetavoxelRendererImplementation";
}

const float DEFAULT_PLACEMENT_GRANULARITY = 0.01f;
const float DEFAULT_VOXELIZATION_GRANULARITY = powf(2.0f, -3.0f);

Spanner::Spanner() :
    _renderer(NULL),
    _placementGranularity(DEFAULT_PLACEMENT_GRANULARITY),
    _voxelizationGranularity(DEFAULT_VOXELIZATION_GRANULARITY),
    _masked(false) {
}

void Spanner::setBounds(const Box& bounds) {
    if (_bounds == bounds) {
        return;
    }
    emit boundsWillChange();
    emit boundsChanged(_bounds = bounds);
}

const QVector<AttributePointer>& Spanner::getAttributes() const {
    static QVector<AttributePointer> emptyVector;
    return emptyVector;
}

const QVector<AttributePointer>& Spanner::getVoxelizedAttributes() const {
    static QVector<AttributePointer> emptyVector;
    return emptyVector;
}

bool Spanner::getAttributeValues(MetavoxelInfo& info, bool force) const {
    return false;
}

bool Spanner::blendAttributeValues(MetavoxelInfo& info, bool force) const {
    return false;
}

bool Spanner::testAndSetVisited(int visit) {
    QMutexLocker locker(&_lastVisitsMutex);
    int& lastVisit = _lastVisits[QThread::currentThread()];
    if (lastVisit == visit) {
        return false;
    }
    lastVisit = visit;
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
        connect(this, &QObject::destroyed, _renderer, &QObject::deleteLater);
        _renderer->init(this);
    }
    return _renderer;
}

bool Spanner::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& clipMinimum, float clipSize, float& distance) const {
    return _bounds.findRayIntersection(origin, direction, distance);
}

bool Spanner::hasOwnColors() const {
    return false;
}

bool Spanner::hasOwnMaterials() const {
    return false;
}

QRgb Spanner::getColorAt(const glm::vec3& point) {
    return 0;
}

int Spanner::getMaterialAt(const glm::vec3& point) {
    return 0;
}

QVector<SharedObjectPointer>& Spanner::getMaterials() {
    static QVector<SharedObjectPointer> emptyMaterials;
    return emptyMaterials;
}

bool Spanner::contains(const glm::vec3& point) {
    return false;
}

bool Spanner::intersects(const glm::vec3& start, const glm::vec3& end, float& distance, glm::vec3& normal) {
    return false;
}

QByteArray Spanner::getRendererClassName() const {
    return "SpannerRendererer";
}

QAtomicInt Spanner::_nextVisit(1);

SpannerRenderer::SpannerRenderer() {
}

void SpannerRenderer::init(Spanner* spanner) {
    _spanner = spanner;
}

void SpannerRenderer::simulate(float deltaTime) {
    // nothing by default
}

void SpannerRenderer::render(const glm::vec4& color, Mode mode, const glm::vec3& clipMinimum, float clipSize) {
    // nothing by default
}

bool SpannerRenderer::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& clipMinimum, float clipSize, float& distance) const {
    return false;
}

Transformable::Transformable() : _scale(1.0f) {
}

void Transformable::setTranslation(const glm::vec3& translation) {
    if (_translation != translation) {
        emit translationChanged(_translation = translation);
    }
}

void Transformable::setRotation(const glm::quat& rotation) {
    if (_rotation != rotation) {
        emit rotationChanged(_rotation = rotation);
    }
}

void Transformable::setScale(float scale) {
    if (_scale != scale) {
        emit scaleChanged(_scale = scale);
    }
}

ColorTransformable::ColorTransformable() :
    _color(Qt::white) {
}

void ColorTransformable::setColor(const QColor& color) {
    if (_color != color) {
        emit colorChanged(_color = color);
    }
}

Sphere::Sphere() {
    connect(this, SIGNAL(translationChanged(const glm::vec3&)), SLOT(updateBounds()));
    connect(this, SIGNAL(scaleChanged(float)), SLOT(updateBounds()));
    updateBounds();
}

const QVector<AttributePointer>& Sphere::getAttributes() const {
    static QVector<AttributePointer> attributes = QVector<AttributePointer>() <<
        AttributeRegistry::getInstance()->getColorAttribute() << AttributeRegistry::getInstance()->getNormalAttribute();
    return attributes;
}

const QVector<AttributePointer>& Sphere::getVoxelizedAttributes() const {
    static QVector<AttributePointer> attributes = QVector<AttributePointer>() <<
        AttributeRegistry::getInstance()->getSpannerColorAttribute() <<
        AttributeRegistry::getInstance()->getSpannerNormalAttribute();
    return attributes;
}

bool Sphere::getAttributeValues(MetavoxelInfo& info, bool force) const {
    // bounds check
    Box bounds = info.getBounds();
    if (!(force || getBounds().intersects(bounds))) {
        return false;
    }
    // count the points inside the sphere
    int pointsWithin = 0;
    for (int i = 0; i < Box::VERTEX_COUNT; i++) {
        if (glm::distance(bounds.getVertex(i), getTranslation()) <= getScale()) {
            pointsWithin++;
        }
    }
    if (pointsWithin == Box::VERTEX_COUNT) {
        // entirely contained
        info.outputValues[0] = AttributeValue(getAttributes().at(0), encodeInline<QRgb>(_color.rgba()));
        info.outputValues[1] = getNormal(info, _color.alpha());
        return false;
    }
    if (force || info.size <= getVoxelizationGranularity()) {
        // best guess
        if (pointsWithin > 0) {
            int alpha = _color.alpha() * pointsWithin / Box::VERTEX_COUNT;
            info.outputValues[0] = AttributeValue(getAttributes().at(0), encodeInline<QRgb>(qRgba(
                _color.red(), _color.green(), _color.blue(), alpha)));
            info.outputValues[1] = getNormal(info, alpha);
        }
        return false;
    }
    return true;
}

bool Sphere::blendAttributeValues(MetavoxelInfo& info, bool force) const {
    // bounds check
    Box bounds = info.getBounds();
    if (!(force || getBounds().intersects(bounds))) {
        return false;
    }
    // count the points inside the sphere
    int pointsWithin = 0;
    for (int i = 0; i < Box::VERTEX_COUNT; i++) {
        if (glm::distance(bounds.getVertex(i), getTranslation()) <= getScale()) {
            pointsWithin++;
        }
    }
    if (pointsWithin == Box::VERTEX_COUNT) {
        // entirely contained
        info.outputValues[0] = AttributeValue(getAttributes().at(0), encodeInline<QRgb>(_color.rgba()));
        info.outputValues[1] = getNormal(info, _color.alpha());
        return false;
    }
    if (force || info.size <= getVoxelizationGranularity()) {
        // best guess
        if (pointsWithin > 0) {
            const AttributeValue& oldColor = info.outputValues.at(0).getAttribute() ?
                info.outputValues.at(0) : info.inputValues.at(0);
            const AttributeValue& oldNormal = info.outputValues.at(1).getAttribute() ?
                info.outputValues.at(1) : info.inputValues.at(1);
            int oldAlpha = qAlpha(oldColor.getInlineValue<QRgb>());
            int newAlpha = _color.alpha() * pointsWithin / Box::VERTEX_COUNT;
            float combinedAlpha = (float)newAlpha / (oldAlpha + newAlpha);
            int baseAlpha = _color.alpha() * pointsWithin / Box::VERTEX_COUNT;
            info.outputValues[0].mix(oldColor, AttributeValue(getAttributes().at(0),
                encodeInline<QRgb>(qRgba(_color.red(), _color.green(), _color.blue(), baseAlpha))), combinedAlpha);
            info.outputValues[1].mix(oldNormal, getNormal(info, baseAlpha), combinedAlpha);
        }
        return false;
    }
    return true;
}

bool Sphere::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& clipMinimum, float clipSize, float& distance) const {
    return findRaySphereIntersection(origin, direction, getTranslation(), getScale(), distance);
}

bool Sphere::contains(const glm::vec3& point) {
    return glm::distance(point, getTranslation()) <= getScale();
}

bool Sphere::intersects(const glm::vec3& start, const glm::vec3& end, float& distance, glm::vec3& normal) {
    glm::vec3 relativeStart = start - getTranslation();
    glm::vec3 vector = end - start;
    float a = glm::dot(vector, vector);
    if (a == 0.0f) {
        return false;
    }
    float b = glm::dot(relativeStart, vector);
    float radicand = b * b - a * (glm::dot(relativeStart, relativeStart) - getScale() * getScale());
    if (radicand < 0.0f) {
        return false;
    }
    float radical = glm::sqrt(radicand);
    float first = (-b - radical) / a;
    if (first >= 0.0f && first <= 1.0f) {
        distance = first;
        normal = glm::normalize(relativeStart + vector * distance);
        return true;
    }
    float second = (-b + radical) / a;
    if (second >= 0.0f && second <= 1.0f) {
        distance = second;
        normal = glm::normalize(relativeStart + vector * distance);
        return true;
    }
    return false;
}

QByteArray Sphere::getRendererClassName() const {
    return "SphereRenderer";
}

void Sphere::updateBounds() {
    glm::vec3 extent(getScale(), getScale(), getScale());
    setBounds(Box(getTranslation() - extent, getTranslation() + extent));
}

AttributeValue Sphere::getNormal(MetavoxelInfo& info, int alpha) const {
    glm::vec3 normal = info.getCenter() - getTranslation();
    float length = glm::length(normal);
    QRgb color;
    if (alpha != 0 && length > EPSILON) {
        const float NORMAL_SCALE = 127.0f;
        float scale = NORMAL_SCALE / length;
        const int BYTE_MASK = 0xFF;
        color = qRgba((int)(normal.x * scale) & BYTE_MASK, (int)(normal.y * scale) & BYTE_MASK,
            (int)(normal.z * scale) & BYTE_MASK, alpha);
        
    } else {
        color = QRgb();
    }
    return AttributeValue(getAttributes().at(1), encodeInline<QRgb>(color));
}

Cuboid::Cuboid() :
    _aspectY(1.0f),
    _aspectZ(1.0f) {
    
    connect(this, &Cuboid::translationChanged, this, &Cuboid::updateBoundsAndPlanes);
    connect(this, &Cuboid::rotationChanged, this, &Cuboid::updateBoundsAndPlanes);
    connect(this, &Cuboid::scaleChanged, this, &Cuboid::updateBoundsAndPlanes);
    connect(this, &Cuboid::aspectYChanged, this, &Cuboid::updateBoundsAndPlanes);
    connect(this, &Cuboid::aspectZChanged, this, &Cuboid::updateBoundsAndPlanes);
    updateBoundsAndPlanes();
}

void Cuboid::setAspectY(float aspectY) {
    if (_aspectY != aspectY) {
        emit aspectYChanged(_aspectY = aspectY);
    }
}

void Cuboid::setAspectZ(float aspectZ) {
    if (_aspectZ != aspectZ) {
        emit aspectZChanged(_aspectZ = aspectZ);
    }
}

bool Cuboid::contains(const glm::vec3& point) {
    glm::vec4 point4(point, 1.0f);
    for (int i = 0; i < PLANE_COUNT; i++) {
        if (glm::dot(_planes[i], point4) > 0.0f) {
            return false;
        }
    }
    return true;
}

bool Cuboid::intersects(const glm::vec3& start, const glm::vec3& end, float& distance, glm::vec3& normal) {
    glm::vec4 start4(start, 1.0f);
    glm::vec4 vector = glm::vec4(end - start, 0.0f);
    for (int i = 0; i < PLANE_COUNT; i++) {
        // first check the segment against the plane
        float divisor = glm::dot(_planes[i], vector);
        if (glm::abs(divisor) < EPSILON) {
            continue;
        }
        float t = -glm::dot(_planes[i], start4) / divisor;
        if (t < 0.0f || t > 1.0f) {
            continue;
        }
        // now that we've established that it intersects the plane, check against the other sides
        glm::vec4 point = start4 + vector * t;
        const int PLANES_PER_AXIS = 2;
        int indexOffset = ((i / PLANES_PER_AXIS) + 1) * PLANES_PER_AXIS;
        for (int j = 0; j < PLANE_COUNT - PLANES_PER_AXIS; j++) {
            if (glm::dot(_planes[(indexOffset + j) % PLANE_COUNT], point) > 0.0f) {
                goto outerContinue;
            }
        }
        distance = t;
        normal = glm::vec3(_planes[i]);
        return true;
        
        outerContinue: ;
    }
    return false;
}

QByteArray Cuboid::getRendererClassName() const {
    return "CuboidRenderer";
}

void Cuboid::updateBoundsAndPlanes() {
    glm::vec3 extent(getScale(), getScale() * _aspectY, getScale() * _aspectZ);
    glm::mat4 rotationMatrix = glm::mat4_cast(getRotation());
    setBounds(glm::translate(getTranslation()) * rotationMatrix * Box(-extent, extent));
    
    glm::vec4 translation4 = glm::vec4(getTranslation(), 1.0f);
    _planes[0] = glm::vec4(glm::vec3(rotationMatrix[0]), -glm::dot(rotationMatrix[0], translation4) - getScale());
    _planes[1] = glm::vec4(glm::vec3(-rotationMatrix[0]), glm::dot(rotationMatrix[0], translation4) - getScale());
    _planes[2] = glm::vec4(glm::vec3(rotationMatrix[1]), -glm::dot(rotationMatrix[1], translation4) - getScale() * _aspectY);
    _planes[3] = glm::vec4(glm::vec3(-rotationMatrix[1]), glm::dot(rotationMatrix[1], translation4) - getScale() * _aspectY);
    _planes[4] = glm::vec4(glm::vec3(rotationMatrix[2]), -glm::dot(rotationMatrix[2], translation4) - getScale() * _aspectZ);
    _planes[5] = glm::vec4(glm::vec3(-rotationMatrix[2]), glm::dot(rotationMatrix[2], translation4) - getScale() * _aspectZ);
}

StaticModel::StaticModel() {
}

void StaticModel::setURL(const QUrl& url) {
    if (_url != url) {
        emit urlChanged(_url = url);
    }
}

bool StaticModel::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& clipMinimum, float clipSize, float& distance) const {
    // delegate to renderer, if we have one
    return _renderer ? _renderer->findRayIntersection(origin, direction, clipMinimum, clipSize, distance) :
        Spanner::findRayIntersection(origin, direction, clipMinimum, clipSize, distance);
}

QByteArray StaticModel::getRendererClassName() const {
    return "StaticModelRenderer";
}

const float EIGHT_BIT_MAXIMUM = 255.0f;

Heightfield::Heightfield(const Box& bounds, float increment, const QByteArray& height, const QByteArray& color,
        const QByteArray& material, const QVector<SharedObjectPointer>& materials) :
    _increment(increment),
    _width((int)glm::round((bounds.maximum.x - bounds.minimum.x) / increment) + 1),
    _heightScale((bounds.maximum.y - bounds.minimum.y) / EIGHT_BIT_MAXIMUM),
    _height(height),
    _color(color),
    _material(material),
    _materials(materials) {
    
    setBounds(bounds);
}

bool Heightfield::hasOwnColors() const {
    return true;
}

bool Heightfield::hasOwnMaterials() const {
    return true;
}

QRgb Heightfield::getColorAt(const glm::vec3& point) {
    glm::vec3 relative = (point - getBounds().minimum) / _increment;
    glm::vec3 floors = glm::floor(relative);
    glm::vec3 ceils = glm::ceil(relative);
    glm::vec3 fracts = glm::fract(relative);
    int floorX = (int)floors.x;
    int floorZ = (int)floors.z;
    int ceilX = (int)ceils.x;
    int ceilZ = (int)ceils.z;
    const uchar* src = (const uchar*)_color.constData();
    const uchar* upperLeft = src + (floorZ * _width + floorX) * DataBlock::COLOR_BYTES;
    const uchar* lowerRight = src + (ceilZ * _width + ceilX) * DataBlock::COLOR_BYTES;
    glm::vec3 interpolatedColor = glm::mix(glm::vec3(upperLeft[0], upperLeft[1], upperLeft[2]),
        glm::vec3(lowerRight[0], lowerRight[1], lowerRight[2]), fracts.z);
    
    // the final vertex (and thus which triangle we check) depends on which half we're on
    if (fracts.x >= fracts.z) {
        const uchar* upperRight = src + (floorZ * _width + ceilX) * DataBlock::COLOR_BYTES;
        interpolatedColor = glm::mix(interpolatedColor, glm::mix(glm::vec3(upperRight[0], upperRight[1], upperRight[2]),
            glm::vec3(lowerRight[0], lowerRight[1], lowerRight[2]), fracts.z), (fracts.x - fracts.z) / (1.0f - fracts.z));
        
    } else {
        const uchar* lowerLeft = src + (ceilZ * _width + floorX) * DataBlock::COLOR_BYTES;
        interpolatedColor = glm::mix(glm::mix(glm::vec3(upperLeft[0], upperLeft[1], upperLeft[2]),
            glm::vec3(lowerLeft[0], lowerLeft[1], lowerLeft[2]), fracts.z), interpolatedColor, fracts.x / fracts.z);
    }
    return qRgb(interpolatedColor.r, interpolatedColor.g, interpolatedColor.b);
}

int Heightfield::getMaterialAt(const glm::vec3& point) {
    glm::vec3 relative = (point - getBounds().minimum) / _increment;
    const uchar* src = (const uchar*)_material.constData();
    return src[(int)glm::round(relative.z) * _width + (int)glm::round(relative.x)];
}

QVector<SharedObjectPointer>& Heightfield::getMaterials() {
    return _materials;
}

bool Heightfield::contains(const glm::vec3& point) {
    if (!getBounds().contains(point)) {
        return false;
    }
    glm::vec3 relative = (point - getBounds().minimum) / _increment;
    glm::vec3 floors = glm::floor(relative);
    glm::vec3 ceils = glm::ceil(relative);
    glm::vec3 fracts = glm::fract(relative);
    int floorX = (int)floors.x;
    int floorZ = (int)floors.z;
    int ceilX = (int)ceils.x;
    int ceilZ = (int)ceils.z;
    const uchar* src = (const uchar*)_height.constData();
    float upperLeft = src[floorZ * _width + floorX];
    float lowerRight = src[ceilZ * _width + ceilX];
    float interpolatedHeight = glm::mix(upperLeft, lowerRight, fracts.z);
    
    // the final vertex (and thus which triangle we check) depends on which half we're on
    if (fracts.x >= fracts.z) {
        float upperRight = src[floorZ * _width + ceilX];
        interpolatedHeight = glm::mix(interpolatedHeight, glm::mix(upperRight, lowerRight, fracts.z),
            (fracts.x - fracts.z) / (1.0f - fracts.z));
        
    } else {
        float lowerLeft = src[ceilZ * _width + floorX];
        interpolatedHeight = glm::mix(glm::mix(upperLeft, lowerLeft, fracts.z), interpolatedHeight, fracts.x / fracts.z);
    }
    return interpolatedHeight != 0.0f && point.y <= interpolatedHeight * _heightScale + getBounds().minimum.y;
}

bool Heightfield::intersects(const glm::vec3& start, const glm::vec3& end, float& distance, glm::vec3& normal) {
    // find the initial location in heightfield coordinates
    float rayDistance;
    glm::vec3 direction = end - start;
    if (!getBounds().findRayIntersection(start, direction, rayDistance) || rayDistance > 1.0f) {
        return false;
    }
    glm::vec3 entry = start + direction * rayDistance;
    const float DISTANCE_THRESHOLD = 0.001f;
    if (glm::abs(entry.x - getBounds().minimum.x) < DISTANCE_THRESHOLD) {
        normal = glm::vec3(-1.0f, 0.0f, 0.0f);
        distance = rayDistance;
        return true;
        
    } else if (glm::abs(entry.x - getBounds().maximum.x) < DISTANCE_THRESHOLD) {
        normal = glm::vec3(1.0f, 0.0f, 0.0f);
        distance = rayDistance;
        return true;
        
    } else if (glm::abs(entry.y - getBounds().minimum.y) < DISTANCE_THRESHOLD) {
        normal = glm::vec3(0.0f, -1.0f, 0.0f);
        distance = rayDistance;
        return true;
        
    } else if (glm::abs(entry.y - getBounds().maximum.y) < DISTANCE_THRESHOLD) {
        normal = glm::vec3(0.0f, 1.0f, 0.0f);
        distance = rayDistance;
        return true;
        
    } else if (glm::abs(entry.z - getBounds().minimum.z) < DISTANCE_THRESHOLD) {
        normal = glm::vec3(0.0f, 0.0f, -1.0f);
        distance = rayDistance;
        return true;
        
    } else if (glm::abs(entry.z - getBounds().maximum.z) < DISTANCE_THRESHOLD) {
        normal = glm::vec3(0.0f, 0.0f, 1.0f);
        distance = rayDistance;
        return true;
    }
    entry = (entry - getBounds().minimum) / _increment;
    glm::vec3 floors = glm::floor(entry);
    glm::vec3 ceils = glm::ceil(entry);
    if (floors.x == ceils.x) {
        if (direction.x > 0.0f) {
            ceils.x += 1.0f;
        } else {
            floors.x -= 1.0f;
        } 
    }
    if (floors.z == ceils.z) {
        if (direction.z > 0.0f) {
            ceils.z += 1.0f;
        } else {
            floors.z -= 1.0f;
        }
    }
    
    bool withinBounds = true;
    float accumulatedDistance = 0.0f;
    const uchar* src = (const uchar*)_height.constData();
    int highestX = _width - 1;
    float highestY = (getBounds().maximum.y - getBounds().minimum.y) / _increment;
    int highestZ = (int)glm::round((getBounds().maximum.z - getBounds().minimum.z) / _increment);
    float heightScale = _heightScale / _increment;
    while (withinBounds && accumulatedDistance <= 1.0f) {
        // find the heights at the corners of the current cell
        int floorX = qMin(qMax((int)floors.x, 0), highestX);
        int floorZ = qMin(qMax((int)floors.z, 0), highestZ);
        int ceilX = qMin(qMax((int)ceils.x, 0), highestX);
        int ceilZ = qMin(qMax((int)ceils.z, 0), highestZ);
        float upperLeft = src[floorZ * _width + floorX] * heightScale;
        float upperRight = src[floorZ * _width + ceilX] * heightScale;
        float lowerLeft = src[ceilZ * _width + floorX] * heightScale;
        float lowerRight = src[ceilZ * _width + ceilX] * heightScale;
        
        // find the distance to the next x coordinate
        float xDistance = FLT_MAX;
        if (direction.x > 0.0f) {
            xDistance = (ceils.x - entry.x) / direction.x;
        } else if (direction.x < 0.0f) {
            xDistance = (floors.x - entry.x) / direction.x;
        }
        
        // and the distance to the next z coordinate
        float zDistance = FLT_MAX;
        if (direction.z > 0.0f) {
            zDistance = (ceils.z - entry.z) / direction.z;
        } else if (direction.z < 0.0f) {
            zDistance = (floors.z - entry.z) / direction.z;
        }
        
        // the exit distance is the lower of those two
        float exitDistance = qMin(xDistance, zDistance);
        glm::vec3 exit, nextFloors = floors, nextCeils = ceils;
        if (exitDistance == FLT_MAX) {
            withinBounds = false; // line points upwards/downwards; check this cell only
            
        } else {
            // find the exit point and the next cell, and determine whether it's still within the bounds
            exit = entry + exitDistance * direction;
            withinBounds = (exit.y >= 0.0f && exit.y <= highestY);
            if (exitDistance == xDistance) {
                if (direction.x > 0.0f) {
                    nextFloors.x += 1.0f;
                    withinBounds &= (nextCeils.x += 1.0f) <= highestX;
                } else {
                    withinBounds &= (nextFloors.x -= 1.0f) >= 0.0f;
                    nextCeils.x -= 1.0f;
                }
            }
            if (exitDistance == zDistance) {
                if (direction.z > 0.0f) {
                    nextFloors.z += 1.0f;
                    withinBounds &= (nextCeils.z += 1.0f) <= highestZ;
                } else {
                    withinBounds &= (nextFloors.z -= 1.0f) >= 0.0f;
                    nextCeils.z -= 1.0f;
                }
            }
            // check the vertical range of the ray against the ranges of the cell heights
            if (qMin(entry.y, exit.y) > qMax(qMax(upperLeft, upperRight), qMax(lowerLeft, lowerRight)) ||
                    qMax(entry.y, exit.y) < qMin(qMin(upperLeft, upperRight), qMin(lowerLeft, lowerRight))) {
                entry = exit;
                floors = nextFloors;
                ceils = nextCeils;
                accumulatedDistance += exitDistance;
                continue;
            } 
        }
        // having passed the bounds check, we must check against the planes
        glm::vec3 relativeEntry = entry - glm::vec3(floors.x, upperLeft, floors.z);
        
        // first check the triangle including the Z+ segment
        glm::vec3 lowerNormal(lowerLeft - lowerRight, 1.0f, upperLeft - lowerLeft);
        float lowerProduct = glm::dot(lowerNormal, direction);
        if (lowerProduct != 0.0f) {
            float planeDistance = -glm::dot(lowerNormal, relativeEntry) / lowerProduct;
            glm::vec3 intersection = relativeEntry + planeDistance * direction;
            if (intersection.x >= 0.0f && intersection.x <= 1.0f && intersection.z >= 0.0f && intersection.z <= 1.0f &&
                    intersection.z >= intersection.x) {
                distance = rayDistance + (accumulatedDistance + planeDistance) * _increment;
                normal = glm::normalize(lowerNormal);
                return true;
            }
        }
        
        // then the one with the X+ segment
        glm::vec3 upperNormal(upperLeft - upperRight, 1.0f, upperRight - lowerRight);
        float upperProduct = glm::dot(upperNormal, direction);
        if (upperProduct != 0.0f) {
            float planeDistance = -glm::dot(upperNormal, relativeEntry) / upperProduct;
            glm::vec3 intersection = relativeEntry + planeDistance * direction;
            if (intersection.x >= 0.0f && intersection.x <= 1.0f && intersection.z >= 0.0f && intersection.z <= 1.0f &&
                    intersection.x >= intersection.z) {
                distance = rayDistance + (accumulatedDistance + planeDistance) * _increment;
                normal = glm::normalize(upperNormal);
                return true;
            }
        }
        
        // no joy; continue on our way
        entry = exit;
        floors = nextFloors;
        ceils = nextCeils;
        accumulatedDistance += exitDistance;
    }
    
    return false;
}
