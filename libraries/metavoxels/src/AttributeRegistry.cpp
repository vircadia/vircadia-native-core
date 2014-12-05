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
REGISTER_META_OBJECT(MaterialObject)
REGISTER_META_OBJECT(SharedObjectAttribute)
REGISTER_META_OBJECT(SharedObjectSetAttribute)
REGISTER_META_OBJECT(SpannerSetAttribute)
REGISTER_META_OBJECT(VoxelColorAttribute)
REGISTER_META_OBJECT(VoxelHermiteAttribute)
REGISTER_META_OBJECT(VoxelMaterialAttribute)

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
    _spannersAttribute(registerAttribute(new SpannerSetAttribute("spanners", &Spanner::staticMetaObject))),
    _voxelColorAttribute(registerAttribute(new VoxelColorAttribute("voxelColor"))),
    _voxelMaterialAttribute(registerAttribute(new VoxelMaterialAttribute("voxelMaterial"))),
    _voxelHermiteAttribute(registerAttribute(new VoxelHermiteAttribute("voxelHermite"))) {
    
    // our baseline LOD threshold is for voxels; spanners and heightfields are a different story
    const float SPANNER_LOD_THRESHOLD_MULTIPLIER = 8.0f;
    _spannersAttribute->setLODThresholdMultiplier(SPANNER_LOD_THRESHOLD_MULTIPLIER);
    _spannersAttribute->setUserFacing(true);
    
    const float VOXEL_LOD_THRESHOLD_MULTIPLIER = 16.0f;
    _voxelColorAttribute->setLODThresholdMultiplier(VOXEL_LOD_THRESHOLD_MULTIPLIER);
    _voxelColorAttribute->setUserFacing(true);
    _voxelMaterialAttribute->setLODThresholdMultiplier(VOXEL_LOD_THRESHOLD_MULTIPLIER);
    _voxelHermiteAttribute->setLODThresholdMultiplier(VOXEL_LOD_THRESHOLD_MULTIPLIER);
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

const float CHAR_SCALE = 127.0f;
const float INVERSE_CHAR_SCALE = 1.0f / CHAR_SCALE;

QRgb packNormal(const glm::vec3& normal) {
    return qRgb((char)(normal.x * CHAR_SCALE), (char)(normal.y * CHAR_SCALE), (char)(normal.z * CHAR_SCALE));
}

QRgb packNormal(const glm::vec3& normal, int alpha) {
    return qRgba((char)(normal.x * CHAR_SCALE), (char)(normal.y * CHAR_SCALE), (char)(normal.z * CHAR_SCALE), alpha);
}

glm::vec3 unpackNormal(QRgb value) {
    return glm::vec3((char)qRed(value) * INVERSE_CHAR_SCALE, (char)qGreen(value) * INVERSE_CHAR_SCALE,
        (char)qBlue(value) * INVERSE_CHAR_SCALE);
}

DataBlock::~DataBlock() {
}

MaterialObject::MaterialObject() :
    _scaleS(1.0f),
    _scaleT(1.0f) {
}

static QHash<uchar, int> countIndices(const QByteArray& contents) {
    QHash<uchar, int> counts;
    for (const uchar* src = (const uchar*)contents.constData(), *end = src + contents.size(); src != end; src++) {
        if (*src != 0) {
            counts[*src]++;
        }
    }
    return counts;
}

const float EIGHT_BIT_MAXIMUM = 255.0f;

uchar getMaterialIndex(const SharedObjectPointer& material, QVector<SharedObjectPointer>& materials, QByteArray& contents) {
    if (!(material && static_cast<MaterialObject*>(material.data())->getDiffuse().isValid())) {
        return 0;
    }
    // first look for a matching existing material, noting the first reusable slot
    int firstEmptyIndex = -1;
    for (int i = 0; i < materials.size(); i++) {
        const SharedObjectPointer& existingMaterial = materials.at(i);
        if (existingMaterial) {
            if (existingMaterial->equals(material.data())) {
                return i + 1;
            }
        } else if (firstEmptyIndex == -1) {
            firstEmptyIndex = i;
        }
    }
    // if nothing found, use the first empty slot or append
    if (firstEmptyIndex != -1) {
        materials[firstEmptyIndex] = material;
        return firstEmptyIndex + 1;
    }
    if (materials.size() < EIGHT_BIT_MAXIMUM) {
        materials.append(material);
        return materials.size();
    }
    // last resort: find the least-used material and remove it
    QHash<uchar, int> counts = countIndices(contents);
    uchar materialIndex = 0;
    int lowestCount = INT_MAX;
    for (QHash<uchar, int>::const_iterator it = counts.constBegin(); it != counts.constEnd(); it++) {
        if (it.value() < lowestCount) {
            materialIndex = it.key();
            lowestCount = it.value();
        }
    }
    contents.replace((char)materialIndex, (char)0);
    return materialIndex;
}

void clearUnusedMaterials(QVector<SharedObjectPointer>& materials, const QByteArray& contents) {
    QHash<uchar, int> counts = countIndices(contents);
    for (int i = 0; i < materials.size(); i++) {
        if (counts.value(i + 1) == 0) {
            materials[i] = SharedObjectPointer();
        }
    }
    while (!(materials.isEmpty() || materials.last())) {
        materials.removeLast();
    }
}

const int VOXEL_COLOR_HEADER_SIZE = sizeof(qint32) * 6;

static QByteArray encodeVoxelColor(int offsetX, int offsetY, int offsetZ,
        int sizeX, int sizeY, int sizeZ, const QVector<QRgb>& contents) {
    QByteArray inflated(VOXEL_COLOR_HEADER_SIZE, 0);
    qint32* header = (qint32*)inflated.data();
    *header++ = offsetX;
    *header++ = offsetY;
    *header++ = offsetZ;
    *header++ = sizeX;
    *header++ = sizeY;
    *header++ = sizeZ;
    inflated.append((const char*)contents.constData(), contents.size() * sizeof(QRgb));
    return qCompress(inflated);
}

static QVector<QRgb> decodeVoxelColor(const QByteArray& encoded, int& offsetX, int& offsetY, int& offsetZ,
        int& sizeX, int& sizeY, int& sizeZ) {
    QByteArray inflated = qUncompress(encoded);
    const qint32* header = (const qint32*)inflated.constData();
    offsetX = *header++;
    offsetY = *header++;
    offsetZ = *header++;
    sizeX = *header++;
    sizeY = *header++;
    sizeZ = *header++;
    int payloadSize = inflated.size() - VOXEL_COLOR_HEADER_SIZE;
    QVector<QRgb> contents(payloadSize / sizeof(QRgb));
    memcpy(contents.data(), inflated.constData() + VOXEL_COLOR_HEADER_SIZE, payloadSize);
    return contents;
}

VoxelColorData::VoxelColorData(const QVector<QRgb>& contents, int size) :
    _contents(contents),
    _size(size) {
}

VoxelColorData::VoxelColorData(Bitstream& in, int bytes) {
    read(in, bytes);
}

VoxelColorData::VoxelColorData(Bitstream& in, int bytes, const VoxelColorDataPointer& reference) {
    if (!reference) {
        read(in, bytes);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    reference->setEncodedDelta(in.readAligned(bytes));
    reference->setDeltaData(DataBlockPointer(this));
    _contents = reference->getContents();
    _size = reference->getSize();
    
    int offsetX, offsetY, offsetZ, sizeX, sizeY, sizeZ;
    QVector<QRgb> delta = decodeVoxelColor(reference->getEncodedDelta(), offsetX, offsetY, offsetZ, sizeX, sizeY, sizeZ);
    if (delta.isEmpty()) {
        return;
    }
    if (offsetX == 0) {
        _contents = delta;
        _size = sizeX;
        return;
    }
    int minX = offsetX - 1;
    int minY = offsetY - 1;
    int minZ = offsetZ - 1;
    const QRgb* src = delta.constData();
    int size2 = _size * _size;
    QRgb* planeDest = _contents.data() + minZ * size2 + minY * _size + minX;
    int length = sizeX * sizeof(QRgb);
    for (int z = 0; z < sizeZ; z++, planeDest += size2) {
        QRgb* dest = planeDest;
        for (int y = 0; y < sizeY; y++, src += sizeX, dest += _size) {
            memcpy(dest, src, length);
        }
    }
}

void VoxelColorData::write(Bitstream& out) {
    QMutexLocker locker(&_encodedMutex);
    if (_encoded.isEmpty()) {
        _encoded = encodeVoxelColor(0, 0, 0, _size, _size, _size, _contents);
    }
    out << _encoded.size();
    out.writeAligned(_encoded);
}

void VoxelColorData::writeDelta(Bitstream& out, const VoxelColorDataPointer& reference) {
    if (!reference || reference->getSize() != _size) {
        write(out);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    if (reference->getEncodedDelta().isEmpty() || reference->getDeltaData() != this) {
        int minX = _size, minY = _size, minZ = _size;
        int maxX = -1, maxY = -1, maxZ = -1;
        const QRgb* src = _contents.constData();
        const QRgb* ref = reference->getContents().constData();
        for (int z = 0; z < _size; z++) {
            bool differenceZ = false;
            for (int y = 0; y < _size; y++) {
                bool differenceY = false;
                for (int x = 0; x < _size; x++) {
                    if (*src++ != *ref++) {
                        minX = qMin(minX, x);
                        maxX = qMax(maxX, x);
                        differenceY = differenceZ = true;
                    }
                }
                if (differenceY) {
                    minY = qMin(minY, y);
                    maxY = qMax(maxY, y);
                }
            }
            if (differenceZ) {
                minZ = qMin(minZ, z);
                maxZ = qMax(maxZ, z);
            }
        }
        QVector<QRgb> delta;
        int sizeX = 0, sizeY = 0, sizeZ = 0;
        if (maxX >= minX) {
            sizeX = maxX - minX + 1;
            sizeY = maxY - minY + 1;
            sizeZ = maxZ - minZ + 1;
            delta = QVector<QRgb>(sizeX * sizeY * sizeZ, 0);
            QRgb* dest = delta.data();
            int size2 = _size * _size;
            const QRgb* planeSrc = _contents.constData() + minZ * size2 + minY * _size + minX;
            int length = sizeX * sizeof(QRgb);
            for (int z = 0; z < sizeZ; z++, planeSrc += size2) {
                src = planeSrc;
                for (int y = 0; y < sizeY; y++, src += _size, dest += sizeX) {
                    memcpy(dest, src, length);
                }
            }
        }
        reference->setEncodedDelta(encodeVoxelColor(minX + 1, minY + 1, minZ + 1, sizeX, sizeY, sizeZ, delta));
        reference->setDeltaData(DataBlockPointer(this));
    }
    out << reference->getEncodedDelta().size();
    out.writeAligned(reference->getEncodedDelta());
}

void VoxelColorData::read(Bitstream& in, int bytes) {
    int offsetX, offsetY, offsetZ, sizeX, sizeY, sizeZ;
    _contents = decodeVoxelColor(_encoded = in.readAligned(bytes), offsetX, offsetY, offsetZ, sizeX, sizeY, sizeZ);
    _size = sizeX;
}

VoxelColorAttribute::VoxelColorAttribute(const QString& name) :
    InlineAttribute<VoxelColorDataPointer>(name) {
}

void VoxelColorAttribute::read(Bitstream& in, void*& value, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    int size;
    in >> size;
    if (size == 0) {
        *(VoxelColorDataPointer*)&value = VoxelColorDataPointer();
    } else {
        *(VoxelColorDataPointer*)&value = VoxelColorDataPointer(new VoxelColorData(in, size));
    }
}

void VoxelColorAttribute::write(Bitstream& out, void* value, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    VoxelColorDataPointer data = decodeInline<VoxelColorDataPointer>(value);
    if (data) {
        data->write(out);
    } else {
        out << 0;
    }
}

void VoxelColorAttribute::readDelta(Bitstream& in, void*& value, void* reference, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    int size;
    in >> size;
    if (size == 0) {
        *(VoxelColorDataPointer*)&value = VoxelColorDataPointer(); 
    } else {
        *(VoxelColorDataPointer*)&value = VoxelColorDataPointer(new VoxelColorData(
            in, size, decodeInline<VoxelColorDataPointer>(reference)));
    }
}

void VoxelColorAttribute::writeDelta(Bitstream& out, void* value, void* reference, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    VoxelColorDataPointer data = decodeInline<VoxelColorDataPointer>(value);
    if (data) {
        data->writeDelta(out, decodeInline<VoxelColorDataPointer>(reference));
    } else {
        out << 0;
    }
}

bool VoxelColorAttribute::merge(void*& parent, void* children[], bool postRead) const {
    int maxSize = 0;
    for (int i = 0; i < MERGE_COUNT; i++) {
        VoxelColorDataPointer pointer = decodeInline<VoxelColorDataPointer>(children[i]);
        if (pointer) {
            maxSize = qMax(maxSize, pointer->getSize());
        }
    }
    if (maxSize == 0) {
        *(VoxelColorDataPointer*)&parent = VoxelColorDataPointer();
        return true;
    }
    int size = maxSize;
    int area = size * size;
    QVector<QRgb> contents(area * size);
    int halfSize = size / 2;
    int halfSizeComplement = size - halfSize;
    for (int i = 0; i < MERGE_COUNT; i++) {
        VoxelColorDataPointer child = decodeInline<VoxelColorDataPointer>(children[i]);
        if (!child) {
            continue;
        }
        const QVector<QRgb>& childContents = child->getContents();
        int childSize = child->getSize();
        int childArea = childSize * childSize;
        const int INDEX_MASK = 1;
        int xIndex = i & INDEX_MASK;
        const int Y_SHIFT = 1;
        int yIndex = (i >> Y_SHIFT) & INDEX_MASK;
        int Z_SHIFT = 2;
        int zIndex = (i >> Z_SHIFT) & INDEX_MASK;
        QRgb* dest = contents.data() + (zIndex * halfSize * area) + (yIndex * halfSize * size) + (xIndex * halfSize);
        const QRgb* src = childContents.data();
        
        const int MAX_ALPHA = 255;
        if (childSize == size) {
            // simple case: one destination value for four child values
            for (int z = 0; z < halfSizeComplement; z++) {
                int offset4 = (z == halfSize) ? 0 : childArea;
                for (int y = 0; y < halfSizeComplement; y++) {
                    int offset2 = (y == halfSize) ? 0 : childSize;
                    int offset6 = offset4 + offset2;
                    for (QRgb* end = dest + halfSizeComplement; dest != end; ) {
                        int offset1 = (dest == end - 1) ? 0 : 1;
                        QRgb v0 = src[0], v1 = src[offset1], v2 = src[offset2], v3 = src[offset2 + offset1], v4 = src[offset4],
                            v5 = src[offset4 + offset1], v6 = src[offset6], v7 = src[offset6 + offset1];
                        src += (1 + offset1);
                        int a0 = qAlpha(v0), a1 = qAlpha(v1), a2 = qAlpha(v2), a3 = qAlpha(v3),
                            a4 = qAlpha(v4), a5 = qAlpha(v5), a6 = qAlpha(v6), a7 = qAlpha(v7);
                        if (a0 == 0) {
                            *dest++ = qRgba(0, 0, 0, 0);
                            continue;
                        }
                        int alphaTotal = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7;
                        *dest++ = qRgba(
                            (qRed(v0) * a0 + qRed(v1) * a1 + qRed(v2) * a2 + qRed(v3) * a3 +
                                qRed(v4) * a4 + qRed(v5) * a5 + qRed(v6) * a6 + qRed(v7) * a7) / alphaTotal,
                            (qGreen(v0) * a0 + qGreen(v1) * a1 + qGreen(v2) * a2 + qGreen(v3) * a3 +
                                qGreen(v4) * a4 + qGreen(v5) * a5 + qGreen(v6) * a6 + qGreen(v7) * a7) / alphaTotal,
                            (qBlue(v0) * a0 + qBlue(v1) * a1 + qBlue(v2) * a2 + qBlue(v3) * a3 +
                                qBlue(v4) * a4 + qBlue(v5) * a5 + qBlue(v6) * a6 + qBlue(v7) * a7) / alphaTotal,
                            MAX_ALPHA);
                    }
                    dest += halfSize;
                    src += offset2;
                }
                dest += halfSize * size;
                src += offset4;
            }
        } else {
            // more complex: N destination values for four child values
            // ...
        }
    }
    *(VoxelColorDataPointer*)&parent = VoxelColorDataPointer(new VoxelColorData(contents, size));
    return false;
}

const int VOXEL_MATERIAL_HEADER_SIZE = sizeof(qint32) * 6;

static QByteArray encodeVoxelMaterial(int offsetX, int offsetY, int offsetZ,
        int sizeX, int sizeY, int sizeZ, const QByteArray& contents) {
    QByteArray inflated(VOXEL_MATERIAL_HEADER_SIZE, 0);
    qint32* header = (qint32*)inflated.data();
    *header++ = offsetX;
    *header++ = offsetY;
    *header++ = offsetZ;
    *header++ = sizeX;
    *header++ = sizeY;
    *header++ = sizeZ;
    inflated.append(contents);
    return qCompress(inflated);
}

static QByteArray decodeVoxelMaterial(const QByteArray& encoded, int& offsetX, int& offsetY, int& offsetZ,
        int& sizeX, int& sizeY, int& sizeZ) {
    QByteArray inflated = qUncompress(encoded);
    const qint32* header = (const qint32*)inflated.constData();
    offsetX = *header++;
    offsetY = *header++;
    offsetZ = *header++;
    sizeX = *header++;
    sizeY = *header++;
    sizeZ = *header++;
    return inflated.mid(VOXEL_MATERIAL_HEADER_SIZE);
}

VoxelMaterialData::VoxelMaterialData(const QByteArray& contents, int size, const QVector<SharedObjectPointer>& materials) :
    _contents(contents),
    _size(size),
    _materials(materials) {
}

VoxelMaterialData::VoxelMaterialData(Bitstream& in, int bytes) {
    read(in, bytes);
}

VoxelMaterialData::VoxelMaterialData(Bitstream& in, int bytes, const VoxelMaterialDataPointer& reference) {
    if (!reference) {
        read(in, bytes);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    reference->setEncodedDelta(in.readAligned(bytes));
    in.readDelta(_materials, reference->getMaterials());
    reference->setDeltaData(DataBlockPointer(this));
    _contents = reference->getContents();
    _size = reference->getSize();
    
    int offsetX, offsetY, offsetZ, sizeX, sizeY, sizeZ;
    QByteArray delta = decodeVoxelMaterial(reference->getEncodedDelta(), offsetX, offsetY, offsetZ, sizeX, sizeY, sizeZ);
    if (delta.isEmpty()) {
        return;
    }
    if (offsetX == 0) {
        _contents = delta;
        _size = sizeX;
        return;
    }
    int minX = offsetX - 1;
    int minY = offsetY - 1;
    int minZ = offsetZ - 1;
    const char* src = delta.constData();
    int size2 = _size * _size;
    char* planeDest = _contents.data() + minZ * size2 + minY * _size + minX;
    for (int z = 0; z < sizeZ; z++, planeDest += size2) {
        char* dest = planeDest;
        for (int y = 0; y < sizeY; y++, src += sizeX, dest += _size) {
            memcpy(dest, src, sizeX);
        }
    }
}

void VoxelMaterialData::write(Bitstream& out) {
    QMutexLocker locker(&_encodedMutex);
    if (_encoded.isEmpty()) {
        _encoded = encodeVoxelMaterial(0, 0, 0, _size, _size, _size, _contents);
    }
    out << _encoded.size();
    out.writeAligned(_encoded);
    out << _materials;
}

void VoxelMaterialData::writeDelta(Bitstream& out, const VoxelMaterialDataPointer& reference) {
    if (!reference || reference->getSize() != _size) {
        write(out);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    if (reference->getEncodedDelta().isEmpty() || reference->getDeltaData() != this) {
        int minX = _size, minY = _size, minZ = _size;
        int maxX = -1, maxY = -1, maxZ = -1;
        const char* src = _contents.constData();
        const char* ref = reference->getContents().constData();
        for (int z = 0; z < _size; z++) {
            bool differenceZ = false;
            for (int y = 0; y < _size; y++) {
                bool differenceY = false;
                for (int x = 0; x < _size; x++) {
                    if (*src++ != *ref++) {
                        minX = qMin(minX, x);
                        maxX = qMax(maxX, x);
                        differenceY = differenceZ = true;
                    }
                }
                if (differenceY) {
                    minY = qMin(minY, y);
                    maxY = qMax(maxY, y);
                }
            }
            if (differenceZ) {
                minZ = qMin(minZ, z);
                maxZ = qMax(maxZ, z);
            }
        }
        QByteArray delta;
        int sizeX = 0, sizeY = 0, sizeZ = 0;
        if (maxX >= minX) {
            sizeX = maxX - minX + 1;
            sizeY = maxY - minY + 1;
            sizeZ = maxZ - minZ + 1;
            delta = QByteArray(sizeX * sizeY * sizeZ, 0);
            char* dest = delta.data();
            int size2 = _size * _size;
            const char* planeSrc = _contents.constData() + minZ * size2 + minY * _size + minX;
            for (int z = 0; z < sizeZ; z++, planeSrc += size2) {
                src = planeSrc;
                for (int y = 0; y < sizeY; y++, src += _size, dest += sizeX) {
                    memcpy(dest, src, sizeX);
                }
            }
        }
        reference->setEncodedDelta(encodeVoxelMaterial(minX + 1, minY + 1, minZ + 1, sizeX, sizeY, sizeZ, delta));
        reference->setDeltaData(DataBlockPointer(this));
    }
    out << reference->getEncodedDelta().size();
    out.writeAligned(reference->getEncodedDelta());
    out.writeDelta(_materials, reference->getMaterials());
}

void VoxelMaterialData::read(Bitstream& in, int bytes) {
    int offsetX, offsetY, offsetZ, sizeX, sizeY, sizeZ;
    _contents = decodeVoxelMaterial(_encoded = in.readAligned(bytes), offsetX, offsetY, offsetZ, sizeX, sizeY, sizeZ);
    _size = sizeX;
    in >> _materials;
}

VoxelMaterialAttribute::VoxelMaterialAttribute(const QString& name) :
    InlineAttribute<VoxelMaterialDataPointer>(name) {
}

void VoxelMaterialAttribute::read(Bitstream& in, void*& value, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    int size;
    in >> size;
    if (size == 0) {
        *(VoxelMaterialDataPointer*)&value = VoxelMaterialDataPointer();
    } else {
        *(VoxelMaterialDataPointer*)&value = VoxelMaterialDataPointer(new VoxelMaterialData(in, size));
    }
}

void VoxelMaterialAttribute::write(Bitstream& out, void* value, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    VoxelMaterialDataPointer data = decodeInline<VoxelMaterialDataPointer>(value);
    if (data) {
        data->write(out);
    } else {
        out << 0;
    }
}

void VoxelMaterialAttribute::readDelta(Bitstream& in, void*& value, void* reference, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    int size;
    in >> size;
    if (size == 0) {
        *(VoxelMaterialDataPointer*)&value = VoxelMaterialDataPointer(); 
    } else {
        *(VoxelMaterialDataPointer*)&value = VoxelMaterialDataPointer(new VoxelMaterialData(
            in, size, decodeInline<VoxelMaterialDataPointer>(reference)));
    }
}

void VoxelMaterialAttribute::writeDelta(Bitstream& out, void* value, void* reference, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    VoxelMaterialDataPointer data = decodeInline<VoxelMaterialDataPointer>(value);
    if (data) {
        data->writeDelta(out, decodeInline<VoxelMaterialDataPointer>(reference));
    } else {
        out << 0;
    }
}

bool VoxelMaterialAttribute::merge(void*& parent, void* children[], bool postRead) const {
    int maxSize = 0;
    for (int i = 0; i < MERGE_COUNT; i++) {
        VoxelMaterialDataPointer pointer = decodeInline<VoxelMaterialDataPointer>(children[i]);
        if (pointer) {
            maxSize = qMax(maxSize, pointer->getSize());
        }
    }
    *(VoxelMaterialDataPointer*)&parent = VoxelMaterialDataPointer();
    return maxSize == 0;
}

VoxelHermiteData::VoxelHermiteData(const QVector<QRgb>& contents, int size) :
    _contents(contents),
    _size(size) {
}

VoxelHermiteData::VoxelHermiteData(Bitstream& in, int bytes) {
    read(in, bytes);
}

VoxelHermiteData::VoxelHermiteData(Bitstream& in, int bytes, const VoxelHermiteDataPointer& reference) {
    if (!reference) {
        read(in, bytes);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    reference->setEncodedDelta(in.readAligned(bytes));
    reference->setDeltaData(DataBlockPointer(this));
    _contents = reference->getContents();
    _size = reference->getSize();
    
    int offsetX, offsetY, offsetZ, sizeX, sizeY, sizeZ;
    QVector<QRgb> delta = decodeVoxelColor(reference->getEncodedDelta(), offsetX, offsetY, offsetZ, sizeX, sizeY, sizeZ);
    if (delta.isEmpty()) {
        return;
    }
    if (offsetX == 0) {
        _contents = delta;
        _size = sizeX;
        return;
    }
    int minX = offsetX - 1;
    int minY = offsetY - 1;
    int minZ = offsetZ - 1;
    const QRgb* src = delta.constData();
    int destStride = _size * EDGE_COUNT;
    int destStride2 = _size * destStride;
    QRgb* planeDest = _contents.data() + minZ * destStride2 + minY * destStride + minX * EDGE_COUNT;
    int srcStride = sizeX * EDGE_COUNT;
    int length = srcStride * sizeof(QRgb);
    for (int z = 0; z < sizeZ; z++, planeDest += destStride2) {
        QRgb* dest = planeDest;
        for (int y = 0; y < sizeY; y++, src += srcStride, dest += destStride) {
            memcpy(dest, src, length);
        }
    }
}

void VoxelHermiteData::write(Bitstream& out) {
    QMutexLocker locker(&_encodedMutex);
    if (_encoded.isEmpty()) {
        _encoded = encodeVoxelColor(0, 0, 0, _size, _size, _size, _contents);
    }
    out << _encoded.size();
    out.writeAligned(_encoded);
}

void VoxelHermiteData::writeDelta(Bitstream& out, const VoxelHermiteDataPointer& reference) {
    if (!reference || reference->getSize() != _size) {
        write(out);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    if (reference->getEncodedDelta().isEmpty() || reference->getDeltaData() != this) {
        int minX = _size, minY = _size, minZ = _size;
        int maxX = -1, maxY = -1, maxZ = -1;
        const QRgb* src = _contents.constData();
        const QRgb* ref = reference->getContents().constData();
        for (int z = 0; z < _size; z++) {
            bool differenceZ = false;
            for (int y = 0; y < _size; y++) {
                bool differenceY = false;
                for (int x = 0; x < _size; x++, src += EDGE_COUNT, ref += EDGE_COUNT) {
                    if (src[0] != ref[0] || src[1] != ref[1] || src[2] != ref[2]) {
                        minX = qMin(minX, x);
                        maxX = qMax(maxX, x);
                        differenceY = differenceZ = true;
                    }
                }
                if (differenceY) {
                    minY = qMin(minY, y);
                    maxY = qMax(maxY, y);
                }
            }
            if (differenceZ) {
                minZ = qMin(minZ, z);
                maxZ = qMax(maxZ, z);
            }
        }
        QVector<QRgb> delta;
        int sizeX = 0, sizeY = 0, sizeZ = 0;
        if (maxX >= minX) {
            sizeX = maxX - minX + 1;
            sizeY = maxY - minY + 1;
            sizeZ = maxZ - minZ + 1;
            delta = QVector<QRgb>(sizeX * sizeY * sizeZ * EDGE_COUNT, 0);
            QRgb* dest = delta.data();
            int srcStride = _size * EDGE_COUNT;
            int srcStride2 = _size * srcStride;
            const QRgb* planeSrc = _contents.constData() + minZ * srcStride2 + minY * srcStride + minX * EDGE_COUNT;
            int destStride = sizeX * EDGE_COUNT;
            int length = destStride * sizeof(QRgb);
            for (int z = 0; z < sizeZ; z++, planeSrc += srcStride2) {
                src = planeSrc;
                for (int y = 0; y < sizeY; y++, src += srcStride, dest += destStride) {
                    memcpy(dest, src, length);
                }
            }
        }
        reference->setEncodedDelta(encodeVoxelColor(minX + 1, minY + 1, minZ + 1, sizeX, sizeY, sizeZ, delta));
        reference->setDeltaData(DataBlockPointer(this));
    }
    out << reference->getEncodedDelta().size();
    out.writeAligned(reference->getEncodedDelta());
}

void VoxelHermiteData::read(Bitstream& in, int bytes) {
    int offsetX, offsetY, offsetZ, sizeX, sizeY, sizeZ;
    _contents = decodeVoxelColor(_encoded = in.readAligned(bytes), offsetX, offsetY, offsetZ, sizeX, sizeY, sizeZ);
    _size = sizeX;
}

VoxelHermiteAttribute::VoxelHermiteAttribute(const QString& name) :
    InlineAttribute<VoxelHermiteDataPointer>(name) {
}

void VoxelHermiteAttribute::read(Bitstream& in, void*& value, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    int size;
    in >> size;
    if (size == 0) {
        *(VoxelHermiteDataPointer*)&value = VoxelHermiteDataPointer();
    } else {
        *(VoxelHermiteDataPointer*)&value = VoxelHermiteDataPointer(new VoxelHermiteData(in, size));
    }
}

void VoxelHermiteAttribute::write(Bitstream& out, void* value, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    VoxelHermiteDataPointer data = decodeInline<VoxelHermiteDataPointer>(value);
    if (data) {
        data->write(out);
    } else {
        out << 0;
    }
}

void VoxelHermiteAttribute::readDelta(Bitstream& in, void*& value, void* reference, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    int size;
    in >> size;
    if (size == 0) {
        *(VoxelHermiteDataPointer*)&value = VoxelHermiteDataPointer(); 
    } else {
        *(VoxelHermiteDataPointer*)&value = VoxelHermiteDataPointer(new VoxelHermiteData(
            in, size, decodeInline<VoxelHermiteDataPointer>(reference)));
    }
}

void VoxelHermiteAttribute::writeDelta(Bitstream& out, void* value, void* reference, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    VoxelHermiteDataPointer data = decodeInline<VoxelHermiteDataPointer>(value);
    if (data) {
        data->writeDelta(out, decodeInline<VoxelHermiteDataPointer>(reference));
    } else {
        out << 0;
    }
}

bool VoxelHermiteAttribute::merge(void*& parent, void* children[], bool postRead) const {
    int maxSize = 0;
    for (int i = 0; i < MERGE_COUNT; i++) {
        VoxelHermiteDataPointer pointer = decodeInline<VoxelHermiteDataPointer>(children[i]);
        if (pointer) {
            maxSize = qMax(maxSize, pointer->getSize());
        }
    }
    if (maxSize == 0) {
        *(VoxelHermiteDataPointer*)&parent = VoxelHermiteDataPointer();
        return true;
    }
    int size = maxSize;
    int area = size * size;
    QVector<QRgb> contents(area * size * VoxelHermiteData::EDGE_COUNT);
    int halfSize = size / 2;
    int halfSizeComplement = size - halfSize;
    for (int i = 0; i < MERGE_COUNT; i++) {
        VoxelHermiteDataPointer child = decodeInline<VoxelHermiteDataPointer>(children[i]);
        if (!child) {
            continue;
        }
        const QVector<QRgb>& childContents = child->getContents();
        int childSize = child->getSize();
        int childArea = childSize * childSize;
        const int INDEX_MASK = 1;
        int xIndex = i & INDEX_MASK;
        const int Y_SHIFT = 1;
        int yIndex = (i >> Y_SHIFT) & INDEX_MASK;
        int Z_SHIFT = 2;
        int zIndex = (i >> Z_SHIFT) & INDEX_MASK;
        QRgb* dest = contents.data() + ((zIndex * halfSize * area) + (yIndex * halfSize * size) + (xIndex * halfSize)) *
            VoxelHermiteData::EDGE_COUNT;
        const QRgb* src = childContents.data();
        int offsets[VoxelHermiteData::EDGE_COUNT];
        
        if (childSize == size) {
            // simple case: one destination value for four child values
            for (int z = 0; z < halfSizeComplement; z++) {
                offsets[2] = (z == halfSize) ? 0 : (childArea * VoxelHermiteData::EDGE_COUNT);
                for (int y = 0; y < halfSizeComplement; y++) {
                    offsets[1] = (y == halfSize) ? 0 : (childSize * VoxelHermiteData::EDGE_COUNT);
                    for (QRgb* end = dest + halfSizeComplement * VoxelHermiteData::EDGE_COUNT; dest != end;
                            dest += VoxelHermiteData::EDGE_COUNT) {
                        offsets[0] = (dest == end - VoxelHermiteData::EDGE_COUNT) ? 0 : VoxelHermiteData::EDGE_COUNT;
                        for (int i = 0; i < VoxelHermiteData::EDGE_COUNT; i++) {
                            QRgb v0 = src[i], v1 = src[i + offsets[i]];
                            glm::vec3 n0 = unpackNormal(v0), n1 = unpackNormal(v1);
                            float l0 = glm::length(n0), l1 = glm::length(n1);
                            float lengthTotal = l0 + l1;
                            if (lengthTotal == 0.0f) {
                                dest[i] = qRgba(0, 0, 0, 0);
                                continue;
                            }
                            glm::vec3 combinedNormal = n0 + n1;
                            float combinedLength = glm::length(combinedNormal);
                            if (combinedLength > 0.0f) {
                                combinedNormal /= combinedLength;
                            }
                            float combinedOffset = qAlpha(v0) * 0.5f * l0 + (qAlpha(v1) + EIGHT_BIT_MAXIMUM) * 0.5f * l1;
                            dest[i] = packNormal(combinedNormal, combinedOffset / lengthTotal);
                        }     
                        src += (VoxelHermiteData::EDGE_COUNT + offsets[0]);
                    }
                    dest += (halfSize * VoxelHermiteData::EDGE_COUNT);
                    src += offsets[1];
                }
                dest += (halfSize * size * VoxelHermiteData::EDGE_COUNT);
                src += offsets[2];
            }
        } else {
            // more complex: N destination values for four child values
            // ...
        }
    }
    *(VoxelHermiteDataPointer*)&parent = VoxelHermiteDataPointer(new VoxelHermiteData(contents, size));
    return false;
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
