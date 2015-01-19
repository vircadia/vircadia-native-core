//
//  AttributeRegistry.cpp
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QBuffer>
#include <QMutexLocker>
#include <QReadLocker>
#include <QScriptEngine>
#include <QWriteLocker>

#include "AttributeRegistry.h"
#include "MetavoxelData.h"
#include "Spanner.h"

REGISTER_META_OBJECT(FloatAttribute)
REGISTER_META_OBJECT(SharedObjectAttribute)
REGISTER_META_OBJECT(SharedObjectSetAttribute)
REGISTER_META_OBJECT(SpannerSetAttribute)

static int attributePointerMetaTypeId = qRegisterMetaType<AttributePointer>();
static int ownedAttributeValueMetaTypeId = qRegisterMetaType<OwnedAttributeValue>();

AttributeRegistry* AttributeRegistry::getInstance() {
    static AttributeRegistry registry;
    return &registry;
}

AttributeRegistry::AttributeRegistry() :
    _guideAttribute(registerAttribute(new SharedObjectAttribute("guide", &MetavoxelGuide::staticMetaObject,
        new DefaultMetavoxelGuide()))),
    _rendererAttribute(registerAttribute(new SharedObjectAttribute("renderer", &MetavoxelRenderer::staticMetaObject,
        new DefaultMetavoxelRenderer()))),
    _spannersAttribute(registerAttribute(new SpannerSetAttribute("spanners", &Spanner::staticMetaObject))) {
    
    // our baseline LOD threshold is for voxels; spanners are a different story
    const float SPANNER_LOD_THRESHOLD_MULTIPLIER = 16.0f;
    _spannersAttribute->setLODThresholdMultiplier(SPANNER_LOD_THRESHOLD_MULTIPLIER);
    _spannersAttribute->setUserFacing(true);
}

static QScriptValue qDebugFunction(QScriptContext* context, QScriptEngine* engine) {
    QDebug debug = qDebug();
    
    for (int i = 0; i < context->argumentCount(); i++) {
        debug << context->argument(i).toString();
    }
    
    return QScriptValue();
}

void AttributeRegistry::configureScriptEngine(QScriptEngine* engine) {
    QScriptValue registry = engine->newObject();
    registry.setProperty("getAttribute", engine->newFunction(getAttribute, 1));
    engine->globalObject().setProperty("AttributeRegistry", registry);
    engine->globalObject().setProperty("qDebug", engine->newFunction(qDebugFunction, 1));
}

AttributePointer AttributeRegistry::registerAttribute(AttributePointer attribute) {
    if (!attribute) {
        return attribute;
    }
    QWriteLocker locker(&_attributesLock);
    AttributePointer& pointer = _attributes[attribute->getName()];
    if (!pointer) {
        pointer = attribute;
    }
    return pointer;
}

void AttributeRegistry::deregisterAttribute(const QString& name) {
    QWriteLocker locker(&_attributesLock);
    _attributes.remove(name);
}

AttributePointer AttributeRegistry::getAttribute(const QString& name) {
    QReadLocker locker(&_attributesLock);
    return _attributes.value(name);
}

QScriptValue AttributeRegistry::getAttribute(QScriptContext* context, QScriptEngine* engine) {
    return engine->newQObject(getInstance()->getAttribute(context->argument(0).toString()).data(), QScriptEngine::QtOwnership,
        QScriptEngine::PreferExistingWrapperObject);
}

AttributeValue::AttributeValue(const AttributePointer& attribute) :
    _attribute(attribute), _value(attribute ? attribute->getDefaultValue() : NULL) {
}

AttributeValue::AttributeValue(const AttributePointer& attribute, void* value) :
    _attribute(attribute), _value(value) {
}

void* AttributeValue::copy() const {
    return _attribute->create(_value);
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

bool AttributeValue::operator!=(const AttributeValue& other) const {
    return _attribute != other._attribute || (_attribute && !_attribute->equal(_value, other._value));
}

bool AttributeValue::operator!=(void* other) const {
    return !_attribute || !_attribute->equal(_value, other);
}

OwnedAttributeValue::OwnedAttributeValue(const AttributePointer& attribute, void* value) :
    AttributeValue(attribute, value) {
}

OwnedAttributeValue::OwnedAttributeValue(const AttributePointer& attribute) :
    AttributeValue(attribute, attribute ? attribute->create() : NULL) {
}

OwnedAttributeValue::OwnedAttributeValue(const AttributeValue& other) :
    AttributeValue(other.getAttribute(), other.getAttribute() ? other.copy() : NULL) {
}

OwnedAttributeValue::OwnedAttributeValue(const OwnedAttributeValue& other) :
    AttributeValue(other.getAttribute(), other.getAttribute() ? other.copy() : NULL) {
}

OwnedAttributeValue::~OwnedAttributeValue() {
    if (_attribute) {
        _attribute->destroy(_value);
    }
}

void OwnedAttributeValue::mix(const AttributeValue& first, const AttributeValue& second, float alpha) {
    if (_attribute) {
        _attribute->destroy(_value);
    }
    _attribute = first.getAttribute();
    _value = _attribute->mix(first.getValue(), second.getValue(), alpha);
}

void OwnedAttributeValue::blend(const AttributeValue& source, const AttributeValue& dest) {
    if (_attribute) {
        _attribute->destroy(_value);
    }
    _attribute = source.getAttribute();
    _value = _attribute->blend(source.getValue(), dest.getValue());
}

OwnedAttributeValue& OwnedAttributeValue::operator=(const AttributeValue& other) {
    if (_attribute) {
        _attribute->destroy(_value);
    }
    if ((_attribute = other.getAttribute())) {
        _value = other.copy();
    }
    return *this;
}

OwnedAttributeValue& OwnedAttributeValue::operator=(const OwnedAttributeValue& other) {
    if (_attribute) {
        _attribute->destroy(_value);
    }
    if ((_attribute = other.getAttribute())) {
        _value = other.copy();
    }
    return *this;
}

Attribute::Attribute(const QString& name) :
        _lodThresholdMultiplier(1.0f),
        _userFacing(false) {
    setObjectName(name);
}

Attribute::~Attribute() {
}

void Attribute::readSubdivided(MetavoxelStreamState& state, void*& value,
        const MetavoxelStreamState& ancestorState, void* ancestorValue, bool isLeaf) const {
    read(state.base.stream, value, isLeaf);
}

void Attribute::writeSubdivided(MetavoxelStreamState& state, void* value,
        const MetavoxelStreamState& ancestorState, void* ancestorValue, bool isLeaf) const {
    write(state.base.stream, value, isLeaf);
}

MetavoxelNode* Attribute::createMetavoxelNode(const AttributeValue& value, const MetavoxelNode* original) const {
    return new MetavoxelNode(value);
}

void Attribute::readMetavoxelRoot(MetavoxelData& data, MetavoxelStreamState& state) {
    data.createRoot(state.base.attribute)->read(state);
}

void Attribute::writeMetavoxelRoot(const MetavoxelNode& root, MetavoxelStreamState& state) {
    root.write(state);
}

void Attribute::readMetavoxelDelta(MetavoxelData& data, const MetavoxelNode& reference, MetavoxelStreamState& state) {
    data.createRoot(state.base.attribute)->readDelta(reference, state);
}

void Attribute::writeMetavoxelDelta(const MetavoxelNode& root, const MetavoxelNode& reference, MetavoxelStreamState& state) {
    root.writeDelta(reference, state);
}

void Attribute::readMetavoxelSubdivision(MetavoxelData& data, MetavoxelStreamState& state) {
    // copy if changed
    MetavoxelNode* oldRoot = data.getRoot(state.base.attribute);
    MetavoxelNode* newRoot = oldRoot->readSubdivision(state);
    if (newRoot != oldRoot) {
        data.setRoot(state.base.attribute, newRoot);
    }
}

void Attribute::writeMetavoxelSubdivision(const MetavoxelNode& root, MetavoxelStreamState& state) {
    root.writeSubdivision(state);
}

bool Attribute::metavoxelRootsEqual(const MetavoxelNode& firstRoot, const MetavoxelNode& secondRoot,
        const glm::vec3& minimum, float size, const MetavoxelLOD& lod) {
    return firstRoot.deepEquals(this, secondRoot, minimum, size, lod);
}

MetavoxelNode* Attribute::expandMetavoxelRoot(const MetavoxelNode& root) {
    AttributePointer attribute(this);
    MetavoxelNode* newParent = new MetavoxelNode(attribute);
    for (int i = 0; i < MetavoxelNode::CHILD_COUNT; i++) {
        MetavoxelNode* newChild = new MetavoxelNode(attribute);
        newParent->setChild(i, newChild);
        int index = MetavoxelNode::getOppositeChildIndex(i);
        if (root.isLeaf()) {
            newChild->setChild(index, new MetavoxelNode(root.getAttributeValue(attribute)));               
        } else {
            MetavoxelNode* grandchild = root.getChild(i);
            grandchild->incrementReferenceCount();
            newChild->setChild(index, grandchild);
        }
        for (int j = 1; j < MetavoxelNode::CHILD_COUNT; j++) {
            MetavoxelNode* newGrandchild = new MetavoxelNode(attribute);
            newChild->setChild((index + j) % MetavoxelNode::CHILD_COUNT, newGrandchild);
        }
    }
    return newParent;
}

FloatAttribute::FloatAttribute(const QString& name) :
    SimpleInlineAttribute(name) {
}

SharedObjectAttribute::SharedObjectAttribute(const QString& name, const QMetaObject* metaObject,
        const SharedObjectPointer& defaultValue) :
    InlineAttribute<SharedObjectPointer>(name, defaultValue),
    _metaObject(metaObject) {
    
}

void SharedObjectAttribute::read(Bitstream& in, void*& value, bool isLeaf) const {
    if (isLeaf) {
        in >> *((SharedObjectPointer*)&value);
    }
}

void SharedObjectAttribute::write(Bitstream& out, void* value, bool isLeaf) const {
    if (isLeaf) {
        out << decodeInline<SharedObjectPointer>(value);
    }
}

bool SharedObjectAttribute::deepEqual(void* first, void* second) const {
    SharedObjectPointer firstObject = decodeInline<SharedObjectPointer>(first);
    SharedObjectPointer secondObject = decodeInline<SharedObjectPointer>(second);
    return firstObject ? firstObject->equals(secondObject) : !secondObject;
}

bool SharedObjectAttribute::merge(void*& parent, void* children[], bool postRead) const {
    SharedObjectPointer firstChild = decodeInline<SharedObjectPointer>(children[0]);
    for (int i = 1; i < MERGE_COUNT; i++) {
        if (firstChild != decodeInline<SharedObjectPointer>(children[i])) {
            *(SharedObjectPointer*)&parent = _defaultValue;
            return false;
        }
    }
    *(SharedObjectPointer*)&parent = firstChild;
    return true;
}

void* SharedObjectAttribute::createFromVariant(const QVariant& value) const {
    return create(encodeInline(value.value<SharedObjectPointer>()));
}

QWidget* SharedObjectAttribute::createEditor(QWidget* parent) const {
    SharedObjectEditor* editor = new SharedObjectEditor(_metaObject, parent);
    editor->setObject(_defaultValue);
    return editor;
}

SharedObjectSetAttribute::SharedObjectSetAttribute(const QString& name, const QMetaObject* metaObject) :
    InlineAttribute<SharedObjectSet>(name),
    _metaObject(metaObject) {
}

void SharedObjectSetAttribute::read(Bitstream& in, void*& value, bool isLeaf) const {
    in >> *((SharedObjectSet*)&value);
}

void SharedObjectSetAttribute::write(Bitstream& out, void* value, bool isLeaf) const {
    out << decodeInline<SharedObjectSet>(value);
}

MetavoxelNode* SharedObjectSetAttribute::createMetavoxelNode(
        const AttributeValue& value, const MetavoxelNode* original) const {
    return new MetavoxelNode(value, original);
}

static bool setsEqual(const SharedObjectSet& firstSet, const SharedObjectSet& secondSet) {
    if (firstSet.size() != secondSet.size()) {
        return false;
    }
    // some hackiness here: we assume that the local ids of the first set correspond to the remote ids of the second,
    // so that this will work with the tests
    foreach (const SharedObjectPointer& firstObject, firstSet) {
        int id = firstObject->getID();
        bool found = false;
        foreach (const SharedObjectPointer& secondObject, secondSet) {
            if (secondObject->getRemoteID() == id) {
                if (!firstObject->equals(secondObject)) {
                    return false;
                }
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

bool SharedObjectSetAttribute::deepEqual(void* first, void* second) const {
    return setsEqual(decodeInline<SharedObjectSet>(first), decodeInline<SharedObjectSet>(second));
}

MetavoxelNode* SharedObjectSetAttribute::expandMetavoxelRoot(const MetavoxelNode& root) {
    AttributePointer attribute(this);
    MetavoxelNode* newParent = new MetavoxelNode(attribute);
    for (int i = 0; i < MetavoxelNode::CHILD_COUNT; i++) {
        MetavoxelNode* newChild = new MetavoxelNode(root.getAttributeValue(attribute));
        newParent->setChild(i, newChild);
        if (root.isLeaf()) {
            continue;
        }       
        MetavoxelNode* grandchild = root.getChild(i);
        grandchild->incrementReferenceCount();
        int index = MetavoxelNode::getOppositeChildIndex(i);
        newChild->setChild(index, grandchild);
        for (int j = 1; j < MetavoxelNode::CHILD_COUNT; j++) {
            MetavoxelNode* newGrandchild = new MetavoxelNode(attribute);
            newChild->setChild((index + j) % MetavoxelNode::CHILD_COUNT, newGrandchild);
        }
    }
    return newParent;
}

bool SharedObjectSetAttribute::merge(void*& parent, void* children[], bool postRead) const {
    for (int i = 0; i < MERGE_COUNT; i++) {
        if (!decodeInline<SharedObjectSet>(children[i]).isEmpty()) {
            return false;
        }
    }
    return true;
}

AttributeValue SharedObjectSetAttribute::inherit(const AttributeValue& parentValue) const {
    return AttributeValue(parentValue.getAttribute());
}

QWidget* SharedObjectSetAttribute::createEditor(QWidget* parent) const {
    return new SharedObjectEditor(_metaObject, parent);
}

SpannerSetAttribute::SpannerSetAttribute(const QString& name, const QMetaObject* metaObject) :
    SharedObjectSetAttribute(name, metaObject) {
}

void SpannerSetAttribute::readMetavoxelRoot(MetavoxelData& data, MetavoxelStreamState& state) {
    forever {
        SharedObjectPointer object;
        state.base.stream >> object;
        if (!object) {
            break;
        }
        data.insert(state.base.attribute, object);
    }
    // even if the root is empty, it should still exist
    if (!data.getRoot(state.base.attribute)) {
        data.createRoot(state.base.attribute);
    }
}

void SpannerSetAttribute::writeMetavoxelRoot(const MetavoxelNode& root, MetavoxelStreamState& state) {
    state.base.visit = Spanner::getAndIncrementNextVisit();
    root.writeSpanners(state);
    state.base.stream << SharedObjectPointer();
}

void SpannerSetAttribute::readMetavoxelDelta(MetavoxelData& data,
        const MetavoxelNode& reference, MetavoxelStreamState& state) {
    readMetavoxelSubdivision(data, state);
}

static void writeDeltaSubdivision(SharedObjectSet& oldSet, SharedObjectSet& newSet, Bitstream& stream) {
    for (SharedObjectSet::iterator newIt = newSet.begin(); newIt != newSet.end(); ) {
        SharedObjectSet::iterator oldIt = oldSet.find(*newIt);
        if (oldIt == oldSet.end()) {
            stream << *newIt; // added
            newIt = newSet.erase(newIt);
            
        } else {
            oldSet.erase(oldIt);
            newIt++;
        }
    }
    foreach (const SharedObjectPointer& object, oldSet) {
        stream << object; // removed
    }
    stream << SharedObjectPointer();
    foreach (const SharedObjectPointer& object, newSet) {
        object->maybeWriteSubdivision(stream);
    }
    stream << SharedObjectPointer();
}

void SpannerSetAttribute::writeMetavoxelDelta(const MetavoxelNode& root,
        const MetavoxelNode& reference, MetavoxelStreamState& state) {
    SharedObjectSet oldSet, newSet;
    reference.getSpanners(this, state.minimum, state.size, state.base.referenceLOD, oldSet);
    root.getSpanners(this, state.minimum, state.size, state.base.lod, newSet);
    writeDeltaSubdivision(oldSet, newSet, state.base.stream);
}

void SpannerSetAttribute::readMetavoxelSubdivision(MetavoxelData& data, MetavoxelStreamState& state) {
    forever {
        SharedObjectPointer object;
        state.base.stream >> object;
        if (!object) {
            break;
        }
        data.toggle(state.base.attribute, object);
    }
    forever {
        SharedObjectPointer object;
        state.base.stream >> object;
        if (!object) {
            break;
        }
        SharedObjectPointer newObject = object->readSubdivision(state.base.stream);
        if (newObject != object) {
            data.replace(state.base.attribute, object, newObject);
            state.base.stream.addSubdividedObject(newObject);
        }
    }
    // even if the root is empty, it should still exist
    if (!data.getRoot(state.base.attribute)) {
        data.createRoot(state.base.attribute);
    }
}

void SpannerSetAttribute::writeMetavoxelSubdivision(const MetavoxelNode& root, MetavoxelStreamState& state) {
    SharedObjectSet oldSet, newSet;
    root.getSpanners(this, state.minimum, state.size, state.base.referenceLOD, oldSet);
    root.getSpanners(this, state.minimum, state.size, state.base.lod, newSet);
    writeDeltaSubdivision(oldSet, newSet, state.base.stream);
}

bool SpannerSetAttribute::metavoxelRootsEqual(const MetavoxelNode& firstRoot, const MetavoxelNode& secondRoot,
    const glm::vec3& minimum, float size, const MetavoxelLOD& lod) {
    
    SharedObjectSet firstSet;
    firstRoot.getSpanners(this, minimum, size, lod, firstSet);
    SharedObjectSet secondSet;
    secondRoot.getSpanners(this, minimum, size, lod, secondSet);
    return setsEqual(firstSet, secondSet);
}
