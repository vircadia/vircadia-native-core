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
#include <QtDebug>

#include "MetavoxelData.h"
#include "Spanner.h"

REGISTER_META_OBJECT(MetavoxelGuide)
REGISTER_META_OBJECT(DefaultMetavoxelGuide)
REGISTER_META_OBJECT(MetavoxelRenderer)
REGISTER_META_OBJECT(DefaultMetavoxelRenderer)

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

bool MetavoxelLOD::shouldSubdivide(const glm::vec2& minimum, float size, float multiplier) const {
    return size >= glm::distance(glm::vec2(position), minimum + glm::vec2(size, size) * 0.5f) * threshold * multiplier;
}

bool MetavoxelLOD::becameSubdivided(const glm::vec2& minimum, float size,
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

bool MetavoxelLOD::becameSubdividedOrCollapsed(const glm::vec2& minimum, float size,
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
    if (!newSpanner) {
        remove(attribute, bounds, granularity, oldObject);
        return;
    }
    if (bounds != newSpanner->getBounds() || granularity != newSpanner->getPlacementGranularity()) {
        // if the bounds have changed, we must remove and reinsert
        remove(attribute, bounds, granularity, oldObject);
        insert(attribute, newSpanner->getBounds(), newSpanner->getPlacementGranularity(), newObject);
        return;
    }
    SpannerReplaceVisitor visitor(attribute, bounds, granularity, oldObject, newObject);
    guide(visitor);
}

class SpannerFetchVisitor : public SpannerVisitor {
public:
    
    SpannerFetchVisitor(const AttributePointer& attribute, const Box& bounds, QVector<SharedObjectPointer>& results);
    
    virtual bool visit(Spanner* spanner);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    const Box& _bounds;
    QVector<SharedObjectPointer>& _results;
};

SpannerFetchVisitor::SpannerFetchVisitor(const AttributePointer& attribute, const Box& bounds,
        QVector<SharedObjectPointer>& results) :
    SpannerVisitor(QVector<AttributePointer>() << attribute),
    _bounds(bounds),
    _results(results) {
}

bool SpannerFetchVisitor::visit(Spanner* spanner) {
    if (spanner->getBounds().intersects(_bounds)) {
        _results.append(spanner);
    }
    return true;
}

int SpannerFetchVisitor::visit(MetavoxelInfo& info) {
    return info.getBounds().intersects(_bounds) ? SpannerVisitor::visit(info) : STOP_RECURSION;
}

void MetavoxelData::getIntersecting(const AttributePointer& attribute, const Box& bounds,
        QVector<SharedObjectPointer>& results) {
    SpannerFetchVisitor visitor(attribute, bounds, results);
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
        QVector<AttributePointer>(), QVector<AttributePointer>(), lod),
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
        in.setContext(&base);
        attribute->readMetavoxelRoot(*this, state);
        in.setContext(NULL);
    }
}

void MetavoxelData::write(Bitstream& out, const MetavoxelLOD& lod) const {
    out << _size;
    for (QHash<AttributePointer, MetavoxelNode*>::const_iterator it = _roots.constBegin(); it != _roots.constEnd(); it++) {
        out << it.key();
        MetavoxelStreamBase base = { it.key(), out, lod, lod };
        MetavoxelStreamState state = { base, getMinimum(), _size };
        out.setContext(&base);
        it.key()->writeMetavoxelRoot(*it.value(), state);
        out.setContext(NULL);
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
            in.setContext(&base);
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
            in.setContext(NULL);
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
            in.setContext(&base);
            it.key()->readMetavoxelSubdivision(*this, state);
            in.setContext(NULL);
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
        out.setContext(&base);
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
        out.setContext(NULL);
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
    if (!state.shouldSubdivide()) {
        return;
    }
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

SpannerVisitor::SpannerVisitor(const QVector<AttributePointer>& spannerInputs, const QVector<AttributePointer>& inputs,
        const QVector<AttributePointer>& outputs, const MetavoxelLOD& lod, int order) :
    MetavoxelVisitor(inputs + spannerInputs, outputs, lod),
    _spannerInputCount(spannerInputs.size()),
    _order(order) {
}

void SpannerVisitor::prepare(MetavoxelData* data) {
    MetavoxelVisitor::prepare(data);
    _visit = Spanner::getAndIncrementNextVisit();
}

int SpannerVisitor::visit(MetavoxelInfo& info) {
    for (int end = _inputs.size(), i = end - _spannerInputCount; i < end; i++) {
        foreach (const SharedObjectPointer& object, info.inputValues.at(i).getInlineValue<SharedObjectSet>()) {
            Spanner* spanner = static_cast<Spanner*>(object.data());
            if (spanner->testAndSetVisited(_visit) && !visit(spanner)) {
                return SHORT_CIRCUIT;
            }
        }
    }
    return info.isLeaf ? STOP_RECURSION : _order;
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
        const QVector<AttributePointer>& spannerInputs, const QVector<AttributePointer>& inputs,
        const QVector<AttributePointer>& outputs, const MetavoxelLOD& lod) :
    RayIntersectionVisitor(origin, direction, inputs + spannerInputs, outputs, lod),
    _spannerInputCount(spannerInputs.size()) {
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
    for (int end = _inputs.size(), i = end - _spannerInputCount; i < end; i++) {
        foreach (const SharedObjectPointer& object, info.inputValues.at(i).getInlineValue<SharedObjectSet>()) {
            Spanner* spanner = static_cast<Spanner*>(object.data());
            if (spanner->testAndSetVisited(_visit)) {
                SpannerDistance spannerDistance = { spanner };
                if (spanner->findRayIntersection(_origin, _direction, spannerDistance.distance)) {
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
    return info.isLeaf ? STOP_RECURSION : _order;
}

bool MetavoxelGuide::guideToDifferent(MetavoxelVisitation& visitation) {
    return guide(visitation);
}

DefaultMetavoxelGuide::DefaultMetavoxelGuide() {
}

static inline bool defaultGuideToChildren(MetavoxelVisitation& visitation, int encodedOrder) {
    MetavoxelVisitation& nextVisitation = visitation.visitor->acquireVisitation();
    nextVisitation.info.size = visitation.info.size * 0.5f;
    for (int i = 0; i < MetavoxelNode::CHILD_COUNT; i++) {
        // the encoded order tells us the child indices for each iteration
        int index = encodedOrder & ORDER_ELEMENT_MASK;
        encodedOrder >>= ORDER_ELEMENT_BITS;
        for (int j = 0; j < visitation.inputNodes.size(); j++) {
            MetavoxelNode* node = visitation.inputNodes.at(j);
            const AttributeValue& parentValue = visitation.info.inputValues.at(j);
            MetavoxelNode* child = (node && (visitation.info.size >= visitation.info.lodBase *
                parentValue.getAttribute()->getLODThresholdMultiplier())) ? node->getChild(index) : NULL;
            nextVisitation.info.inputValues[j] = ((nextVisitation.inputNodes[j] = child)) ?
                child->getAttributeValue(parentValue.getAttribute()) : parentValue.getAttribute()->inherit(parentValue);
        }
        for (int j = 0; j < visitation.outputNodes.size(); j++) {
            MetavoxelNode* node = visitation.outputNodes.at(j);
            MetavoxelNode* child = (node && (visitation.info.size >= visitation.info.lodBase *
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
    visitation.info.lodBase = glm::distance(visitation.visitor->getLOD().position, visitation.info.getCenter()) *
        visitation.visitor->getLOD().threshold;
    visitation.info.isLODLeaf = (visitation.info.size < visitation.info.lodBase *
        visitation.visitor->getMinimumLODThresholdMultiplier());
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
    return (encodedOrder == MetavoxelVisitor::STOP_RECURSION || defaultGuideToChildren(visitation, encodedOrder));
}

bool DefaultMetavoxelGuide::guideToDifferent(MetavoxelVisitation& visitation) {
    // save the core of the LOD calculation; we'll reuse it to determine whether to subdivide each attribute
    visitation.info.lodBase = glm::distance(visitation.visitor->getLOD().position, visitation.info.getCenter()) *
        visitation.visitor->getLOD().threshold;
    visitation.info.isLODLeaf = (visitation.info.size < visitation.info.lodBase *
        visitation.visitor->getMinimumLODThresholdMultiplier());
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
        return defaultGuideToChildren(visitation, encodedOrder);
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
            bool expand = (visitation.info.size >= visitation.info.lodBase *
                parentValue.getAttribute()->getLODThresholdMultiplier());
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
            MetavoxelNode* child = (node && (visitation.info.size >= visitation.info.lodBase *
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

bool MetavoxelVisitation::isInputLeaf(int index) const {   
    MetavoxelNode* node = inputNodes.at(index);  
    return !node || node->isLeaf() || info.size < info.lodBase *
        info.inputValues.at(index).getAttribute()->getLODThresholdMultiplier();
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

