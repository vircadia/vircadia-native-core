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

REGISTER_META_OBJECT(FloatAttribute)
REGISTER_META_OBJECT(QRgbAttribute)
REGISTER_META_OBJECT(PackedNormalAttribute)
REGISTER_META_OBJECT(SpannerQRgbAttribute)
REGISTER_META_OBJECT(SpannerPackedNormalAttribute)
REGISTER_META_OBJECT(MaterialObject)
REGISTER_META_OBJECT(HeightfieldAttribute)
REGISTER_META_OBJECT(HeightfieldColorAttribute)
REGISTER_META_OBJECT(HeightfieldMaterialAttribute)
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
    _spannersAttribute(registerAttribute(new SpannerSetAttribute("spanners", &Spanner::staticMetaObject))),
    _colorAttribute(registerAttribute(new QRgbAttribute("color"))),
    _normalAttribute(registerAttribute(new PackedNormalAttribute("normal"))),
    _spannerColorAttribute(registerAttribute(new SpannerQRgbAttribute("spannerColor"))),
    _spannerNormalAttribute(registerAttribute(new SpannerPackedNormalAttribute("spannerNormal"))),
    _spannerMaskAttribute(registerAttribute(new FloatAttribute("spannerMask"))),
    _heightfieldAttribute(registerAttribute(new HeightfieldAttribute("heightfield"))),
    _heightfieldColorAttribute(registerAttribute(new HeightfieldColorAttribute("heightfieldColor"))),
    _heightfieldMaterialAttribute(registerAttribute(new HeightfieldMaterialAttribute("heightfieldMaterial"))),
    _voxelMaterialAttribute(registerAttribute(new VoxelMaterialAttribute("voxelMaterial"))) {
    
    // our baseline LOD threshold is for voxels; spanners and heightfields are a different story
    const float SPANNER_LOD_THRESHOLD_MULTIPLIER = 8.0f;
    _spannersAttribute->setLODThresholdMultiplier(SPANNER_LOD_THRESHOLD_MULTIPLIER);
    
    const float HEIGHTFIELD_LOD_THRESHOLD_MULTIPLIER = 32.0f;
    _heightfieldAttribute->setLODThresholdMultiplier(HEIGHTFIELD_LOD_THRESHOLD_MULTIPLIER);
    _heightfieldColorAttribute->setLODThresholdMultiplier(HEIGHTFIELD_LOD_THRESHOLD_MULTIPLIER);
    _heightfieldMaterialAttribute->setLODThresholdMultiplier(HEIGHTFIELD_LOD_THRESHOLD_MULTIPLIER);
    
    const float VOXEL_LOD_THRESHOLD_MULTIPLIER = 32.0f;
    _voxelMaterialAttribute->setLODThresholdMultiplier(VOXEL_LOD_THRESHOLD_MULTIPLIER);
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
    registry.setProperty("colorAttribute", engine->newQObject(_colorAttribute.data()));
    registry.setProperty("normalAttribute", engine->newQObject(_normalAttribute.data()));
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
        _lodThresholdMultiplier(1.0f) {
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

FloatAttribute::FloatAttribute(const QString& name, float defaultValue) :
    SimpleInlineAttribute<float>(name, defaultValue) {
}

QRgbAttribute::QRgbAttribute(const QString& name, QRgb defaultValue) :
    InlineAttribute<QRgb>(name, defaultValue) {
}

bool QRgbAttribute::merge(void*& parent, void* children[], bool postRead) const {
    QRgb firstValue = decodeInline<QRgb>(children[0]);
    int totalAlpha = qAlpha(firstValue);
    int totalRed = qRed(firstValue) * totalAlpha;
    int totalGreen = qGreen(firstValue) * totalAlpha;
    int totalBlue = qBlue(firstValue) * totalAlpha;
    bool allChildrenEqual = true;
    for (int i = 1; i < Attribute::MERGE_COUNT; i++) {
        QRgb value = decodeInline<QRgb>(children[i]);
        int alpha = qAlpha(value);
        totalRed += qRed(value) * alpha;
        totalGreen += qGreen(value) * alpha;
        totalBlue += qBlue(value) * alpha;
        totalAlpha += alpha;
        allChildrenEqual &= (firstValue == value);
    }
    if (totalAlpha == 0) {
        parent = encodeInline(QRgb());
    } else {
        parent = encodeInline(qRgba(totalRed / totalAlpha, totalGreen / totalAlpha,
            totalBlue / totalAlpha, totalAlpha / MERGE_COUNT));
    }
    return allChildrenEqual;
} 

void* QRgbAttribute::mix(void* first, void* second, float alpha) const {
    QRgb firstValue = decodeInline<QRgb>(first);
    QRgb secondValue = decodeInline<QRgb>(second);
    return encodeInline(qRgba(
        glm::mix((float)qRed(firstValue), (float)qRed(secondValue), alpha),
        glm::mix((float)qGreen(firstValue), (float)qGreen(secondValue), alpha),
        glm::mix((float)qBlue(firstValue), (float)qBlue(secondValue), alpha),
        glm::mix((float)qAlpha(firstValue), (float)qAlpha(secondValue), alpha)));
}

const float EIGHT_BIT_MAXIMUM = 255.0f;

void* QRgbAttribute::blend(void* source, void* dest) const {
    QRgb sourceValue = decodeInline<QRgb>(source);
    QRgb destValue = decodeInline<QRgb>(dest);    
    float alpha = qAlpha(sourceValue) / EIGHT_BIT_MAXIMUM;
    return encodeInline(qRgba(
        glm::mix((float)qRed(destValue), (float)qRed(sourceValue), alpha),
        glm::mix((float)qGreen(destValue), (float)qGreen(sourceValue), alpha),
        glm::mix((float)qBlue(destValue), (float)qBlue(sourceValue), alpha),
        glm::mix((float)qAlpha(destValue), (float)qAlpha(sourceValue), alpha)));
}

void* QRgbAttribute::createFromScript(const QScriptValue& value, QScriptEngine* engine) const {
    return encodeInline((QRgb)value.toUInt32());
}

void* QRgbAttribute::createFromVariant(const QVariant& value) const {
    switch (value.userType()) {
        case QMetaType::QColor:
            return encodeInline(value.value<QColor>().rgba());
        
        default:
            return encodeInline((QRgb)value.toUInt());
    } 
}

QWidget* QRgbAttribute::createEditor(QWidget* parent) const {
    QColorEditor* editor = new QColorEditor(parent);
    editor->setColor(QColor::fromRgba(_defaultValue));
    return editor;
}

PackedNormalAttribute::PackedNormalAttribute(const QString& name, QRgb defaultValue) :
    QRgbAttribute(name, defaultValue) {
}

bool PackedNormalAttribute::merge(void*& parent, void* children[], bool postRead) const {
    QRgb firstValue = decodeInline<QRgb>(children[0]);
    glm::vec3 total = unpackNormal(firstValue) * (float)qAlpha(firstValue);
    bool allChildrenEqual = true;
    for (int i = 1; i < Attribute::MERGE_COUNT; i++) {
        QRgb value = decodeInline<QRgb>(children[i]);
        total += unpackNormal(value) * (float)qAlpha(value);
        allChildrenEqual &= (firstValue == value);
    }
    float length = glm::length(total);
    parent = encodeInline(length < EPSILON ? QRgb() : packNormal(total / length));
    return allChildrenEqual;
}

void* PackedNormalAttribute::mix(void* first, void* second, float alpha) const {
    glm::vec3 firstNormal = unpackNormal(decodeInline<QRgb>(first));
    glm::vec3 secondNormal = unpackNormal(decodeInline<QRgb>(second));
    return encodeInline(packNormal(glm::normalize(glm::mix(firstNormal, secondNormal, alpha))));
}

void* PackedNormalAttribute::blend(void* source, void* dest) const {
    QRgb sourceValue = decodeInline<QRgb>(source);
    QRgb destValue = decodeInline<QRgb>(dest);    
    float alpha = qAlpha(sourceValue) / EIGHT_BIT_MAXIMUM;
    return encodeInline(packNormal(glm::normalize(glm::mix(unpackNormal(destValue), unpackNormal(sourceValue), alpha))));
}

const float CHAR_SCALE = 127.0f;
const float INVERSE_CHAR_SCALE = 1.0f / CHAR_SCALE;

QRgb packNormal(const glm::vec3& normal) {
    return qRgb((char)(normal.x * CHAR_SCALE), (char)(normal.y * CHAR_SCALE), (char)(normal.z * CHAR_SCALE));
}

glm::vec3 unpackNormal(QRgb value) {
    return glm::vec3((char)qRed(value) * INVERSE_CHAR_SCALE, (char)qGreen(value) * INVERSE_CHAR_SCALE,
        (char)qBlue(value) * INVERSE_CHAR_SCALE);
}

SpannerQRgbAttribute::SpannerQRgbAttribute(const QString& name, QRgb defaultValue) :
    QRgbAttribute(name, defaultValue) {
}

void SpannerQRgbAttribute::read(Bitstream& in, void*& value, bool isLeaf) const {
    value = getDefaultValue();
    in.read(&value, 32);
}

void SpannerQRgbAttribute::write(Bitstream& out, void* value, bool isLeaf) const {
    out.write(&value, 32);
}

MetavoxelNode* SpannerQRgbAttribute::createMetavoxelNode(
        const AttributeValue& value, const MetavoxelNode* original) const {
    return new MetavoxelNode(value, original);
}

bool SpannerQRgbAttribute::merge(void*& parent, void* children[], bool postRead) const {
    if (postRead) {
        for (int i = 0; i < MERGE_COUNT; i++) {
            if (qAlpha(decodeInline<QRgb>(children[i])) != 0) {
                return false;
            }
        }
        return true;
    }
    QRgb parentValue = decodeInline<QRgb>(parent);
    int totalAlpha = qAlpha(parentValue) * Attribute::MERGE_COUNT;
    int totalRed = qRed(parentValue) * totalAlpha;
    int totalGreen = qGreen(parentValue) * totalAlpha;
    int totalBlue = qBlue(parentValue) * totalAlpha;
    bool allChildrenTransparent = true;
    for (int i = 0; i < Attribute::MERGE_COUNT; i++) {
        QRgb value = decodeInline<QRgb>(children[i]);
        int alpha = qAlpha(value);
        totalRed += qRed(value) * alpha;
        totalGreen += qGreen(value) * alpha;
        totalBlue += qBlue(value) * alpha;
        totalAlpha += alpha;
        allChildrenTransparent &= (alpha == 0);
    }
    if (totalAlpha == 0) {
        parent = encodeInline(QRgb());
    } else {
        parent = encodeInline(qRgba(totalRed / totalAlpha, totalGreen / totalAlpha,
            totalBlue / totalAlpha, totalAlpha / MERGE_COUNT));
    }
    return allChildrenTransparent;
} 

AttributeValue SpannerQRgbAttribute::inherit(const AttributeValue& parentValue) const {
    return AttributeValue(parentValue.getAttribute());
}

SpannerPackedNormalAttribute::SpannerPackedNormalAttribute(const QString& name, QRgb defaultValue) :
    PackedNormalAttribute(name, defaultValue) {
}

void SpannerPackedNormalAttribute::read(Bitstream& in, void*& value, bool isLeaf) const {
    value = getDefaultValue();
    in.read(&value, 32);
}

void SpannerPackedNormalAttribute::write(Bitstream& out, void* value, bool isLeaf) const {
    out.write(&value, 32);
}

MetavoxelNode* SpannerPackedNormalAttribute::createMetavoxelNode(
        const AttributeValue& value, const MetavoxelNode* original) const {
    return new MetavoxelNode(value, original);
}

bool SpannerPackedNormalAttribute::merge(void*& parent, void* children[], bool postRead) const {
    if (postRead) {
        for (int i = 0; i < MERGE_COUNT; i++) {
            if (qAlpha(decodeInline<QRgb>(children[i])) != 0) {
                return false;
            }
        }
        return true;
    }
    QRgb parentValue = decodeInline<QRgb>(parent);
    glm::vec3 total = unpackNormal(parentValue) * (float)(qAlpha(parentValue) * Attribute::MERGE_COUNT);
    bool allChildrenTransparent = true;
    for (int i = 0; i < Attribute::MERGE_COUNT; i++) {
        QRgb value = decodeInline<QRgb>(children[i]);
        int alpha = qAlpha(value);
        total += unpackNormal(value) * (float)alpha;
        allChildrenTransparent &= (alpha == 0);
    }
    float length = glm::length(total);
    parent = encodeInline(length < EPSILON ? QRgb() : packNormal(total / length));
    return allChildrenTransparent;
}

AttributeValue SpannerPackedNormalAttribute::inherit(const AttributeValue& parentValue) const {
    return AttributeValue(parentValue.getAttribute());
}

DataBlock::~DataBlock() {
}

enum HeightfieldImage { NULL_HEIGHTFIELD_IMAGE, NORMAL_HEIGHTFIELD_IMAGE, DEFLATED_HEIGHTFIELD_IMAGE };

static QByteArray encodeHeightfieldImage(const QImage& image, bool lossless = false) {
    if (image.isNull()) {
        return QByteArray(1, NULL_HEIGHTFIELD_IMAGE);
    }
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    const int JPEG_ENCODE_THRESHOLD = 16;
    if (image.width() >= JPEG_ENCODE_THRESHOLD && image.height() >= JPEG_ENCODE_THRESHOLD && !lossless) {
        qint32 offsetX = image.offset().x(), offsetY = image.offset().y();
        buffer.write((char*)&offsetX, sizeof(qint32));
        buffer.write((char*)&offsetY, sizeof(qint32));
        image.save(&buffer, "JPG");
        return QByteArray(1, DEFLATED_HEIGHTFIELD_IMAGE) + qCompress(buffer.data());
        
    } else {
        buffer.putChar(NORMAL_HEIGHTFIELD_IMAGE);
        image.save(&buffer, "PNG");
        return buffer.data();
    }
}

const QImage decodeHeightfieldImage(const QByteArray& data) {
    switch (data.at(0)) {
        case NULL_HEIGHTFIELD_IMAGE:
        default:
            return QImage();
         
        case NORMAL_HEIGHTFIELD_IMAGE:
            return QImage::fromData(QByteArray::fromRawData(data.constData() + 1, data.size() - 1));
        
        case DEFLATED_HEIGHTFIELD_IMAGE: {
            QByteArray inflated = qUncompress((const uchar*)data.constData() + 1, data.size() - 1);
            const int OFFSET_SIZE = sizeof(qint32) * 2;
            QImage image = QImage::fromData((const uchar*)inflated.constData() + OFFSET_SIZE, inflated.size() - OFFSET_SIZE);
            const qint32* offsets = (const qint32*)inflated.constData();
            image.setOffset(QPoint(offsets[0], offsets[1]));
            return image;
        }
    }
}

HeightfieldHeightData::HeightfieldHeightData(const QByteArray& contents) :
    _contents(contents) {
}

HeightfieldHeightData::HeightfieldHeightData(Bitstream& in, int bytes) {
    read(in, bytes);
}

HeightfieldHeightData::HeightfieldHeightData(Bitstream& in, int bytes, const HeightfieldHeightDataPointer& reference) {
    if (!reference) {
        read(in, bytes);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    reference->setEncodedDelta(in.readAligned(bytes));
    reference->setDeltaData(DataBlockPointer(this));
    _contents = reference->getContents();
    QImage image = decodeHeightfieldImage(reference->getEncodedDelta());
    if (image.isNull()) {
        return;
    }
    QPoint offset = image.offset();
    image = image.convertToFormat(QImage::Format_RGB888);
    if (offset.x() == 0) {
        set(image);
        return;
    }
    int minX = offset.x() - 1;
    int minY = offset.y() - 1;
    int size = glm::sqrt((float)_contents.size());
    char* lineDest = _contents.data() + minY * size + minX;
    for (int y = 0; y < image.height(); y++) {
        const uchar* src = image.constScanLine(y);
        for (char* dest = lineDest, *end = dest + image.width(); dest != end; dest++, src += COLOR_BYTES) {
            *dest = *src;
        }
        lineDest += size;
    }
}

HeightfieldHeightData::HeightfieldHeightData(Bitstream& in, int bytes, const HeightfieldHeightDataPointer& ancestor,
        const glm::vec3& minimum, float size) {
    QMutexLocker locker(&_encodedSubdivisionsMutex);
    int index = (int)glm::round(glm::log(size) / glm::log(0.5f)) - 1;
    if (_encodedSubdivisions.size() <= index) {
        _encodedSubdivisions.resize(index + 1);
    }
    EncodedSubdivision& subdivision = _encodedSubdivisions[index];
    subdivision.data = in.readAligned(bytes);
    subdivision.ancestor = ancestor;
    QImage image = decodeHeightfieldImage(subdivision.data);
    if (image.isNull()) {
        return;
    }
    image = image.convertToFormat(QImage::Format_RGB888);
    int destSize = image.width();
    const uchar* src = image.constBits();
    const QByteArray& ancestorContents = ancestor->getContents();
    
    int ancestorSize = glm::sqrt((float)ancestorContents.size());
    float ancestorY = minimum.z * ancestorSize;
    float ancestorIncrement = size * ancestorSize / destSize;
    
    _contents = QByteArray(destSize * destSize, 0);
    char* dest = _contents.data();
    
    for (int y = 0; y < destSize; y++, ancestorY += ancestorIncrement) {
        const uchar* lineRef = (const uchar*)ancestorContents.constData() + (int)ancestorY * ancestorSize;
        float ancestorX = minimum.x * ancestorSize;
        for (char* end = dest + destSize; dest != end; src += COLOR_BYTES, ancestorX += ancestorIncrement) {
            const uchar* ref = lineRef + (int)ancestorX;
            *dest++ = *ref++ + *src;
        }
    }
}

void HeightfieldHeightData::write(Bitstream& out) {
    QMutexLocker locker(&_encodedMutex);
    if (_encoded.isEmpty()) {
        QImage image;        
        int size = glm::sqrt((float)_contents.size());
        image = QImage(size, size, QImage::Format_RGB888);
        uchar* dest = image.bits();
        for (const char* src = _contents.constData(), *end = src + _contents.size(); src != end; src++) {
            *dest++ = *src;
            *dest++ = *src;
            *dest++ = *src;
        }
        _encoded = encodeHeightfieldImage(image);
    }
    out << _encoded.size();
    out.writeAligned(_encoded);
}

void HeightfieldHeightData::writeDelta(Bitstream& out, const HeightfieldHeightDataPointer& reference) {
    if (!reference || reference->getContents().size() != _contents.size()) {
        write(out);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    if (reference->getEncodedDelta().isEmpty() || reference->getDeltaData() != this) {
        QImage image;
        int size = glm::sqrt((float)_contents.size());
        int minX = size, minY = size;
        int maxX = -1, maxY = -1;
        const char* src = _contents.constData();
        const char* ref = reference->getContents().constData();
        for (int y = 0; y < size; y++) {
            bool difference = false;
            for (int x = 0; x < size; x++) {
                if (*src++ != *ref++) {
                    minX = qMin(minX, x);
                    maxX = qMax(maxX, x);
                    difference = true;
                }
            }
            if (difference) {
                minY = qMin(minY, y);
                maxY = qMax(maxY, y);
            }
        }
        if (maxX >= minX) {
            int width = qMax(maxX - minX + 1, 0);
            int height = qMax(maxY - minY + 1, 0);
            image = QImage(width, height, QImage::Format_RGB888);
            const uchar* lineSrc = (const uchar*)_contents.constData() + minY * size + minX;
            for (int y = 0; y < height; y++) {
                uchar* dest = image.scanLine(y);
                for (const uchar* src = lineSrc, *end = src + width; src != end; src++) {
                    *dest++ = *src;
                    *dest++ = *src;
                    *dest++ = *src;
                }
                lineSrc += size;
            }
        }
        image.setOffset(QPoint(minX + 1, minY + 1));
        reference->setEncodedDelta(encodeHeightfieldImage(image));
        reference->setDeltaData(DataBlockPointer(this));
    }
    out << reference->getEncodedDelta().size();
    out.writeAligned(reference->getEncodedDelta());
}

void HeightfieldHeightData::writeSubdivided(Bitstream& out, const HeightfieldHeightDataPointer& ancestor,
        const glm::vec3& minimum, float size) {
    QMutexLocker locker(&_encodedSubdivisionsMutex);
    int index = (int)glm::round(glm::log(size) / glm::log(0.5f)) - 1;
    if (_encodedSubdivisions.size() <= index) {
        _encodedSubdivisions.resize(index + 1);
    }
    EncodedSubdivision& subdivision = _encodedSubdivisions[index];
    if (subdivision.data.isEmpty() || subdivision.ancestor != ancestor) {
        QImage image;
        const QByteArray& ancestorContents = ancestor->getContents();
        const uchar* src = (const uchar*)_contents.constData();
        
        int destSize = glm::sqrt((float)_contents.size());
        image = QImage(destSize, destSize, QImage::Format_RGB888);
        uchar* dest = image.bits();
        
        int ancestorSize = glm::sqrt((float)ancestorContents.size());
        float ancestorY = minimum.z * ancestorSize;
        float ancestorIncrement = size * ancestorSize / destSize;
        
        for (int y = 0; y < destSize; y++, ancestorY += ancestorIncrement) {
            const uchar* lineRef = (const uchar*)ancestorContents.constData() + (int)ancestorY * ancestorSize;
            float ancestorX = minimum.x * ancestorSize;
            for (const uchar* end = src + destSize; src != end; ancestorX += ancestorIncrement) {
                const uchar* ref = lineRef + (int)ancestorX;
                uchar difference = *src++ - *ref;
                *dest++ = difference;
                *dest++ = difference;
                *dest++ = difference;
            }
        }    
        subdivision.data = encodeHeightfieldImage(image, true);
        subdivision.ancestor = ancestor;
    }
    out << subdivision.data.size();
    out.writeAligned(subdivision.data);
}

void HeightfieldHeightData::read(Bitstream& in, int bytes) {
    set(decodeHeightfieldImage(_encoded = in.readAligned(bytes)).convertToFormat(QImage::Format_RGB888));
}

void HeightfieldHeightData::set(const QImage& image) {
    _contents.resize(image.width() * image.height());
    char* dest = _contents.data();
    for (const uchar* src = image.constBits(), *end = src + _contents.size() * COLOR_BYTES;
            src != end; src += COLOR_BYTES) {
        *dest++ = *src;
    }
}

HeightfieldColorData::HeightfieldColorData(const QByteArray& contents) :
    _contents(contents) {
}

HeightfieldColorData::HeightfieldColorData(Bitstream& in, int bytes) {
    read(in, bytes);
}

HeightfieldColorData::HeightfieldColorData(Bitstream& in, int bytes, const HeightfieldColorDataPointer& reference) {
    if (!reference) {
        read(in, bytes);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    reference->setEncodedDelta(in.readAligned(bytes));
    reference->setDeltaData(DataBlockPointer(this));
    _contents = reference->getContents();
    QImage image = decodeHeightfieldImage(reference->getEncodedDelta());
    if (image.isNull()) {
        return;
    }
    QPoint offset = image.offset();
    image = image.convertToFormat(QImage::Format_RGB888);
    if (offset.x() == 0) {
        set(image);
        return;
    }
    int minX = offset.x() - 1;
    int minY = offset.y() - 1;
    int size = glm::sqrt(_contents.size() / (float)COLOR_BYTES);
    char* dest = _contents.data() + (minY * size + minX) * COLOR_BYTES;
    int destStride = size * COLOR_BYTES;
    int srcStride = image.width() * COLOR_BYTES;
    for (int y = 0; y < image.height(); y++) {
        memcpy(dest, image.constScanLine(y), srcStride);
        dest += destStride;
    }
}

HeightfieldColorData::HeightfieldColorData(Bitstream& in, int bytes, const HeightfieldColorDataPointer& ancestor,
        const glm::vec3& minimum, float size) {
    QMutexLocker locker(&_encodedSubdivisionsMutex);
    int index = (int)glm::round(glm::log(size) / glm::log(0.5f)) - 1;
    if (_encodedSubdivisions.size() <= index) {
        _encodedSubdivisions.resize(index + 1);
    }
    EncodedSubdivision& subdivision = _encodedSubdivisions[index];
    subdivision.data = in.readAligned(bytes);
    subdivision.ancestor = ancestor;
    QImage image = decodeHeightfieldImage(subdivision.data);
    if (image.isNull()) {
        return;
    }
    image = image.convertToFormat(QImage::Format_RGB888);
    int destSize = image.width();
    const uchar* src = image.constBits();
    const QByteArray& ancestorContents = ancestor->getContents();

    int ancestorSize = glm::sqrt(ancestorContents.size() / (float)COLOR_BYTES);
    float ancestorY = minimum.z * ancestorSize;
    float ancestorIncrement = size * ancestorSize / destSize;
    int ancestorStride = ancestorSize * COLOR_BYTES;
    
    _contents = QByteArray(destSize * destSize * COLOR_BYTES, 0);
    char* dest = _contents.data();    
    int stride = image.width() * COLOR_BYTES;
    
    for (int y = 0; y < destSize; y++, ancestorY += ancestorIncrement) {
        const uchar* lineRef = (const uchar*)ancestorContents.constData() + (int)ancestorY * ancestorStride;
        float ancestorX = minimum.x * ancestorSize;
        for (char* end = dest + stride; dest != end; ancestorX += ancestorIncrement) {
            const uchar* ref = lineRef + (int)ancestorX * COLOR_BYTES;
            *dest++ = *ref++ + *src++;
            *dest++ = *ref++ + *src++;
            *dest++ = *ref++ + *src++;
        }
    }
}

void HeightfieldColorData::write(Bitstream& out) {
    QMutexLocker locker(&_encodedMutex);
    if (_encoded.isEmpty()) {
        QImage image;        
        int size = glm::sqrt(_contents.size() / (float)COLOR_BYTES);
        image = QImage((uchar*)_contents.data(), size, size, QImage::Format_RGB888);
        _encoded = encodeHeightfieldImage(image);
    }
    out << _encoded.size();
    out.writeAligned(_encoded);
}

void HeightfieldColorData::writeDelta(Bitstream& out, const HeightfieldColorDataPointer& reference) {
    if (!reference || reference->getContents().size() != _contents.size()) {
        write(out);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    if (reference->getEncodedDelta().isEmpty() || reference->getDeltaData() != this) {
        QImage image;
        int size = glm::sqrt(_contents.size() / (float)COLOR_BYTES);
        int minX = size, minY = size;
        int maxX = -1, maxY = -1;
        const char* src = _contents.constData();
        const char* ref = reference->getContents().constData();
        for (int y = 0; y < size; y++) {
            bool difference = false;
            for (int x = 0; x < size; x++, src += COLOR_BYTES, ref += COLOR_BYTES) {
                if (src[0] != ref[0] || src[1] != ref[1] || src[2] != ref[2]) {
                    minX = qMin(minX, x);
                    maxX = qMax(maxX, x);
                    difference = true;
                }
            }
            if (difference) {
                minY = qMin(minY, y);
                maxY = qMax(maxY, y);
            }
        }
        if (maxX >= minX) {
            int width = maxX - minX + 1;
            int height = maxY - minY + 1;
            image = QImage(width, height, QImage::Format_RGB888);
            src = _contents.constData() + (minY * size + minX) * COLOR_BYTES;
            int srcStride = size * COLOR_BYTES;
            int destStride = width * COLOR_BYTES;
            for (int y = 0; y < height; y++) {
                memcpy(image.scanLine(y), src, destStride);
                src += srcStride;
            }
        }
        image.setOffset(QPoint(minX + 1, minY + 1));
        reference->setEncodedDelta(encodeHeightfieldImage(image));
        reference->setDeltaData(DataBlockPointer(this));
    }
    out << reference->getEncodedDelta().size();
    out.writeAligned(reference->getEncodedDelta());
}

void HeightfieldColorData::writeSubdivided(Bitstream& out, const HeightfieldColorDataPointer& ancestor,
        const glm::vec3& minimum, float size) {
    QMutexLocker locker(&_encodedSubdivisionsMutex);
    int index = (int)glm::round(glm::log(size) / glm::log(0.5f)) - 1;
    if (_encodedSubdivisions.size() <= index) {
        _encodedSubdivisions.resize(index + 1);
    }
    EncodedSubdivision& subdivision = _encodedSubdivisions[index];
    if (subdivision.data.isEmpty() || subdivision.ancestor != ancestor) {
        QImage image;
        const QByteArray& ancestorContents = ancestor->getContents();
        const uchar* src = (const uchar*)_contents.constData();
        
        int destSize = glm::sqrt(_contents.size() / (float)COLOR_BYTES);
        image = QImage(destSize, destSize, QImage::Format_RGB888);
        uchar* dest = image.bits();
        int stride = destSize * COLOR_BYTES;
        
        int ancestorSize = glm::sqrt(ancestorContents.size() / (float)COLOR_BYTES);
        float ancestorY = minimum.z * ancestorSize;
        float ancestorIncrement = size * ancestorSize / destSize;
        int ancestorStride = ancestorSize * COLOR_BYTES;
        
        for (int y = 0; y < destSize; y++, ancestorY += ancestorIncrement) {
            const uchar* lineRef = (const uchar*)ancestorContents.constData() + (int)ancestorY * ancestorStride;
            float ancestorX = minimum.x * ancestorSize;
            for (const uchar* end = src + stride; src != end; ancestorX += ancestorIncrement) {
                const uchar* ref = lineRef + (int)ancestorX * COLOR_BYTES;
                *dest++ = *src++ - *ref++;
                *dest++ = *src++ - *ref++;
                *dest++ = *src++ - *ref++;
            }
        }    
        subdivision.data = encodeHeightfieldImage(image, true);
        subdivision.ancestor = ancestor;
    }
    out << subdivision.data.size();
    out.writeAligned(subdivision.data);
}

void HeightfieldColorData::read(Bitstream& in, int bytes) {
    set(decodeHeightfieldImage(_encoded = in.readAligned(bytes)).convertToFormat(QImage::Format_RGB888));
}

void HeightfieldColorData::set(const QImage& image) {
    _contents.resize(image.width() * image.height() * COLOR_BYTES);
    memcpy(_contents.data(), image.constBits(), _contents.size());
}

const int HEIGHTFIELD_MATERIAL_HEADER_SIZE = sizeof(qint32) * 4;

static QByteArray encodeHeightfieldMaterial(int offsetX, int offsetY, int width, int height, const QByteArray& contents) {
    QByteArray inflated(HEIGHTFIELD_MATERIAL_HEADER_SIZE, 0);
    qint32* header = (qint32*)inflated.data();
    *header++ = offsetX;
    *header++ = offsetY;
    *header++ = width;
    *header++ = height;
    inflated.append(contents);
    return qCompress(inflated);
}

static QByteArray decodeHeightfieldMaterial(const QByteArray& encoded, int& offsetX, int& offsetY, int& width, int& height) {
    QByteArray inflated = qUncompress(encoded);
    const qint32* header = (const qint32*)inflated.constData();
    offsetX = *header++;
    offsetY = *header++;
    width = *header++;
    height = *header++;
    return inflated.mid(HEIGHTFIELD_MATERIAL_HEADER_SIZE);
}

HeightfieldMaterialData::HeightfieldMaterialData(const QByteArray& contents, const QVector<SharedObjectPointer>& materials) :
    _contents(contents),
    _materials(materials) {
}

HeightfieldMaterialData::HeightfieldMaterialData(Bitstream& in, int bytes) {
    read(in, bytes);
}

HeightfieldMaterialData::HeightfieldMaterialData(Bitstream& in, int bytes, const HeightfieldMaterialDataPointer& reference) {
    if (!reference) {
        read(in, bytes);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    reference->setEncodedDelta(in.readAligned(bytes));
    in.readDelta(_materials, reference->getMaterials());
    reference->setDeltaData(DataBlockPointer(this));
    _contents = reference->getContents();
    
    int offsetX, offsetY, width, height;
    QByteArray delta = decodeHeightfieldMaterial(reference->getEncodedDelta(), offsetX, offsetY, width, height);
    if (delta.isEmpty()) {
        return;
    }
    if (offsetX == 0) {
        _contents = delta;
        return;
    }
    int minX = offsetX - 1;
    int minY = offsetY - 1;
    int size = glm::sqrt((float)_contents.size());
    const char* src = delta.constData();
    char* dest = _contents.data() + minY * size + minX;
    for (int y = 0; y < height; y++, src += width, dest += size) {
        memcpy(dest, src, width);
    }
}

void HeightfieldMaterialData::write(Bitstream& out) {
    QMutexLocker locker(&_encodedMutex);
    if (_encoded.isEmpty()) {
        int size = glm::sqrt((float)_contents.size());
        _encoded = encodeHeightfieldMaterial(0, 0, size, size, _contents);
    }
    out << _encoded.size();
    out.writeAligned(_encoded);
    out << _materials;
}

void HeightfieldMaterialData::writeDelta(Bitstream& out, const HeightfieldMaterialDataPointer& reference) {
    if (!reference || reference->getContents().size() != _contents.size()) {
        write(out);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    if (reference->getEncodedDelta().isEmpty() || reference->getDeltaData() != this) {
        int size = glm::sqrt((float)_contents.size());
        int minX = size, minY = size;
        int maxX = -1, maxY = -1;
        const char* src = _contents.constData();
        const char* ref = reference->getContents().constData();
        for (int y = 0; y < size; y++) {
            bool difference = false;
            for (int x = 0; x < size; x++) {
                if (*src++ != *ref++) {
                    minX = qMin(minX, x);
                    maxX = qMax(maxX, x);
                    difference = true;
                }
            }
            if (difference) {
                minY = qMin(minY, y);
                maxY = qMax(maxY, y);
            }
        }
        QByteArray delta;
        int width = 0, height = 0;
        if (maxX >= minX) {
            width = maxX - minX + 1;
            height = maxY - minY + 1;
            delta = QByteArray(width * height, 0);
            char* dest = delta.data();
            src = _contents.constData() + minY * size + minX;
            for (int y = 0; y < height; y++, src += size, dest += width) {
                memcpy(dest, src, width);
            }
        }
        reference->setEncodedDelta(encodeHeightfieldMaterial(minX + 1, minY + 1, width, height, delta));
        reference->setDeltaData(DataBlockPointer(this));
    }
    out << reference->getEncodedDelta().size();
    out.writeAligned(reference->getEncodedDelta());
    out.writeDelta(_materials, reference->getMaterials());
}

void HeightfieldMaterialData::read(Bitstream& in, int bytes) {
    int offsetX, offsetY, width, height;
    _contents = decodeHeightfieldMaterial(_encoded = in.readAligned(bytes), offsetX, offsetY, width, height);
    in >> _materials;
}

MaterialObject::MaterialObject() :
    _scaleS(1.0f),
    _scaleT(1.0f) {
}

HeightfieldAttribute::HeightfieldAttribute(const QString& name) :
    InlineAttribute<HeightfieldHeightDataPointer>(name) {
}

void HeightfieldAttribute::read(Bitstream& in, void*& value, bool isLeaf) const {
    if (!isLeaf) {
        return;
    } 
    int size;
    in >> size;
    if (size == 0) {
        *(HeightfieldHeightDataPointer*)&value = HeightfieldHeightDataPointer();
    } else {
        *(HeightfieldHeightDataPointer*)&value = HeightfieldHeightDataPointer(new HeightfieldHeightData(in, size));
    }
}

void HeightfieldAttribute::write(Bitstream& out, void* value, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    HeightfieldHeightDataPointer data = decodeInline<HeightfieldHeightDataPointer>(value);
    if (data) {
        data->write(out);
    } else {
        out << 0;
    }
}

void HeightfieldAttribute::readDelta(Bitstream& in, void*& value, void* reference, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    int size;
    in >> size;
    if (size == 0) {
        *(HeightfieldHeightDataPointer*)&value = HeightfieldHeightDataPointer(); 
    } else {
        *(HeightfieldHeightDataPointer*)&value = HeightfieldHeightDataPointer(new HeightfieldHeightData(
            in, size, decodeInline<HeightfieldHeightDataPointer>(reference)));
    }
}

void HeightfieldAttribute::writeDelta(Bitstream& out, void* value, void* reference, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    HeightfieldHeightDataPointer data = decodeInline<HeightfieldHeightDataPointer>(value);
    if (data) {
        data->writeDelta(out, decodeInline<HeightfieldHeightDataPointer>(reference));
    } else {
        out << 0;
    }
}

bool HeightfieldAttribute::merge(void*& parent, void* children[], bool postRead) const {
    int maxSize = 0;
    for (int i = 0; i < MERGE_COUNT; i++) {
        HeightfieldHeightDataPointer pointer = decodeInline<HeightfieldHeightDataPointer>(children[i]);
        if (pointer) {
            maxSize = qMax(maxSize, pointer->getContents().size());
        }
    }
    if (maxSize == 0) {
        *(HeightfieldHeightDataPointer*)&parent = HeightfieldHeightDataPointer();
        return true;
    }
    int size = glm::sqrt((float)maxSize);
    QByteArray contents(size * size, 0);
    int halfSize = size / 2;
    for (int i = 0; i < MERGE_COUNT; i++) {
        HeightfieldHeightDataPointer child = decodeInline<HeightfieldHeightDataPointer>(children[i]);
        if (!child) {
            continue;
        }
        const QByteArray& childContents = child->getContents();
        int childSize = glm::sqrt((float)childContents.size());
        const int INDEX_MASK = 1;
        int xIndex = i & INDEX_MASK;
        const int Y_SHIFT = 1;
        int yIndex = (i >> Y_SHIFT) & INDEX_MASK;
        if (yIndex == 0 && decodeInline<HeightfieldHeightDataPointer>(children[i | (1 << Y_SHIFT)])) {
            continue; // bottom is overriden by top
        }
        const int HALF_RANGE = 128;
        int yOffset = yIndex * HALF_RANGE;
        int Z_SHIFT = 2;
        int zIndex = (i >> Z_SHIFT) & INDEX_MASK;
        char* dest = contents.data() + (zIndex * halfSize * size) + (xIndex * halfSize);
        uchar* src = (uchar*)childContents.data();
        int childSizePlusOne = childSize + 1;
        if (childSize == size) {
            // simple case: one destination value for four child values
            for (int z = 0; z < halfSize; z++) {
                for (char* end = dest + halfSize; dest != end; src += 2) {
                    int max = qMax(qMax(src[0], src[1]), qMax(src[childSize], src[childSizePlusOne]));
                    *dest++ = (max == 0) ? 0 : (yOffset + (max >> 1));
                }
                dest += halfSize;
                src += childSize;
            }
        } else {
            // more complex: N destination values for four child values
            int halfChildSize = childSize / 2;
            int destPerSrc = size / childSize;
            for (int z = 0; z < halfChildSize; z++) {
                for (uchar* end = src + childSize; src != end; src += 2) {
                    int max = qMax(qMax(src[0], src[1]), qMax(src[childSize], src[childSizePlusOne]));
                    memset(dest, (max == 0) ? 0 : (yOffset + (max >> 1)), destPerSrc);
                    dest += destPerSrc;
                }
                dest += halfSize;
                for (int j = 1; j < destPerSrc; j++) {
                    memcpy(dest, dest - size, halfSize);
                    dest += size;
                }
                src += childSize;
            }
        }
    }
    *(HeightfieldHeightDataPointer*)&parent = HeightfieldHeightDataPointer(new HeightfieldHeightData(contents));
    return false;
}

HeightfieldColorAttribute::HeightfieldColorAttribute(const QString& name) :
    InlineAttribute<HeightfieldColorDataPointer>(name) {
}

void HeightfieldColorAttribute::read(Bitstream& in, void*& value, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    int size;
    in >> size;
    if (size == 0) {
        *(HeightfieldColorDataPointer*)&value = HeightfieldColorDataPointer();
    } else {
        *(HeightfieldColorDataPointer*)&value = HeightfieldColorDataPointer(new HeightfieldColorData(in, size));
    }
}

void HeightfieldColorAttribute::write(Bitstream& out, void* value, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    HeightfieldColorDataPointer data = decodeInline<HeightfieldColorDataPointer>(value);
    if (data) {
        data->write(out);
    } else {
        out << 0;
    }
}

void HeightfieldColorAttribute::readDelta(Bitstream& in, void*& value, void* reference, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    int size;
    in >> size;
    if (size == 0) {
        *(HeightfieldColorDataPointer*)&value = HeightfieldColorDataPointer(); 
    } else {
        *(HeightfieldColorDataPointer*)&value = HeightfieldColorDataPointer(new HeightfieldColorData(
            in, size, decodeInline<HeightfieldColorDataPointer>(reference)));
    }
}

void HeightfieldColorAttribute::writeDelta(Bitstream& out, void* value, void* reference, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    HeightfieldColorDataPointer data = decodeInline<HeightfieldColorDataPointer>(value);
    if (data) {
        data->writeDelta(out, decodeInline<HeightfieldColorDataPointer>(reference));
    } else {
        out << 0;
    }
}

bool HeightfieldColorAttribute::merge(void*& parent, void* children[], bool postRead) const {
    int maxSize = 0;
    for (int i = 0; i < MERGE_COUNT; i++) {
        HeightfieldColorDataPointer pointer = decodeInline<HeightfieldColorDataPointer>(children[i]);
        if (pointer) {
            maxSize = qMax(maxSize, pointer->getContents().size());
        }
    }
    if (maxSize == 0) {
        *(HeightfieldColorDataPointer*)&parent = HeightfieldColorDataPointer();
        return true;
    }
    int size = glm::sqrt(maxSize / (float)DataBlock::COLOR_BYTES);
    QByteArray contents(size * size * DataBlock::COLOR_BYTES, 0);
    int halfSize = size / 2;
    for (int i = 0; i < MERGE_COUNT; i++) {
        HeightfieldColorDataPointer child = decodeInline<HeightfieldColorDataPointer>(children[i]);
        if (!child) {
            continue;
        }
        const QByteArray& childContents = child->getContents();
        int childSize = glm::sqrt(childContents.size() / (float)DataBlock::COLOR_BYTES);
        const int INDEX_MASK = 1;
        int xIndex = i & INDEX_MASK;
        const int Y_SHIFT = 1;
        int yIndex = (i >> Y_SHIFT) & INDEX_MASK;
        if (yIndex == 0 && decodeInline<HeightfieldColorDataPointer>(children[i | (1 << Y_SHIFT)])) {
            continue; // bottom is overriden by top
        }
        int Z_SHIFT = 2;
        int zIndex = (i >> Z_SHIFT) & INDEX_MASK;
        char* dest = contents.data() + ((zIndex * halfSize * size) + (xIndex * halfSize)) * DataBlock::COLOR_BYTES;
        uchar* src = (uchar*)childContents.data();
        int childStride = childSize * DataBlock::COLOR_BYTES;
        int stride = size * DataBlock::COLOR_BYTES;
        int halfStride = stride / 2;
        int childStep = 2 * DataBlock::COLOR_BYTES;
        int redOffset3 = childStride + DataBlock::COLOR_BYTES;
        int greenOffset1 = DataBlock::COLOR_BYTES + 1;
        int greenOffset2 = childStride + 1;
        int greenOffset3 = childStride + DataBlock::COLOR_BYTES + 1;
        int blueOffset1 = DataBlock::COLOR_BYTES + 2;
        int blueOffset2 = childStride + 2;
        int blueOffset3 = childStride + DataBlock::COLOR_BYTES + 2;
        if (childSize == size) {
            // simple case: one destination value for four child values
            for (int z = 0; z < halfSize; z++) {
                for (char* end = dest + halfSize * DataBlock::COLOR_BYTES; dest != end; src += childStep) {
                    *dest++ = ((int)src[0] + (int)src[DataBlock::COLOR_BYTES] +
                        (int)src[childStride] + (int)src[redOffset3]) >> 2;
                    *dest++ = ((int)src[1] + (int)src[greenOffset1] + (int)src[greenOffset2] + (int)src[greenOffset3]) >> 2;
                    *dest++ = ((int)src[2] + (int)src[blueOffset1] + (int)src[blueOffset2] + (int)src[blueOffset3]) >> 2;
                }
                dest += halfStride;
                src += childStride;
            }
        } else {
            // more complex: N destination values for four child values
            int halfChildSize = childSize / 2;
            int destPerSrc = size / childSize;
            for (int z = 0; z < halfChildSize; z++) {
                for (uchar* end = src + childSize * DataBlock::COLOR_BYTES; src != end; src += childStep) {
                    *dest++ = ((int)src[0] + (int)src[DataBlock::COLOR_BYTES] +
                        (int)src[childStride] + (int)src[redOffset3]) >> 2;
                    *dest++ = ((int)src[1] + (int)src[greenOffset1] + (int)src[greenOffset2] + (int)src[greenOffset3]) >> 2;
                    *dest++ = ((int)src[2] + (int)src[blueOffset1] + (int)src[blueOffset2] + (int)src[blueOffset3]) >> 2;
                    for (int j = 1; j < destPerSrc; j++) {
                        memcpy(dest, dest - DataBlock::COLOR_BYTES, DataBlock::COLOR_BYTES);
                        dest += DataBlock::COLOR_BYTES;
                    }
                }
                dest += halfStride;
                for (int j = 1; j < destPerSrc; j++) {
                    memcpy(dest, dest - stride, halfStride);
                    dest += stride;
                }
                src += childStride;
            }
        }
    }
    *(HeightfieldColorDataPointer*)&parent = HeightfieldColorDataPointer(new HeightfieldColorData(contents));
    return false;
}

HeightfieldMaterialAttribute::HeightfieldMaterialAttribute(const QString& name) :
    InlineAttribute<HeightfieldMaterialDataPointer>(name) {
}

void HeightfieldMaterialAttribute::read(Bitstream& in, void*& value, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    int size;
    in >> size;
    if (size == 0) {
        *(HeightfieldMaterialDataPointer*)&value = HeightfieldMaterialDataPointer();
    } else {
        *(HeightfieldMaterialDataPointer*)&value = HeightfieldMaterialDataPointer(new HeightfieldMaterialData(in, size));
    }
}

void HeightfieldMaterialAttribute::write(Bitstream& out, void* value, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    HeightfieldMaterialDataPointer data = decodeInline<HeightfieldMaterialDataPointer>(value);
    if (data) {
        data->write(out);
    } else {
        out << 0;
    }
}

void HeightfieldMaterialAttribute::readDelta(Bitstream& in, void*& value, void* reference, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    int size;
    in >> size;
    if (size == 0) {
        *(HeightfieldMaterialDataPointer*)&value = HeightfieldMaterialDataPointer(); 
    } else {
        *(HeightfieldMaterialDataPointer*)&value = HeightfieldMaterialDataPointer(new HeightfieldMaterialData(
            in, size, decodeInline<HeightfieldMaterialDataPointer>(reference)));
    }
}

void HeightfieldMaterialAttribute::writeDelta(Bitstream& out, void* value, void* reference, bool isLeaf) const {
    if (!isLeaf) {
        return;
    }
    HeightfieldMaterialDataPointer data = decodeInline<HeightfieldMaterialDataPointer>(value);
    if (data) {
        data->writeDelta(out, decodeInline<HeightfieldMaterialDataPointer>(reference));
    } else {
        out << 0;
    }
}

bool HeightfieldMaterialAttribute::merge(void*& parent, void* children[], bool postRead) const {
    int maxSize = 0;
    for (int i = 0; i < MERGE_COUNT; i++) {
        HeightfieldMaterialDataPointer pointer = decodeInline<HeightfieldMaterialDataPointer>(children[i]);
        if (pointer) {
            maxSize = qMax(maxSize, pointer->getContents().size());
        }
    }
    *(HeightfieldMaterialDataPointer*)&parent = HeightfieldMaterialDataPointer();
    return maxSize == 0;
}

VoxelColorData::VoxelColorData(const QVector<QRgb>& contents) :
    _contents(contents) {
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
}

void VoxelColorData::write(Bitstream& out) {
    QMutexLocker locker(&_encodedMutex);
    if (_encoded.isEmpty()) {
        
    }
    out << _encoded.size();
    out.writeAligned(_encoded);
}

void VoxelColorData::writeDelta(Bitstream& out, const VoxelColorDataPointer& reference) {
    if (!reference || reference->getContents().size() != _contents.size()) {
        write(out);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    if (reference->getEncodedDelta().isEmpty() || reference->getDeltaData() != this) {
        
    }
    out << reference->getEncodedDelta().size();
    out.writeAligned(reference->getEncodedDelta());
}

void VoxelColorData::read(Bitstream& in, int bytes) {
    
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
    return inflated.mid(HEIGHTFIELD_MATERIAL_HEADER_SIZE);
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
            maxSize = qMax(maxSize, pointer->getContents().size());
        }
    }
    *(VoxelMaterialDataPointer*)&parent = VoxelMaterialDataPointer();
    return maxSize == 0;
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
    forever {
        SharedObjectPointer object;
        state.base.stream >> object;
        if (!object) {
            break;
        }
        data.toggle(state.base.attribute, object);
    }
    // even if the root is empty, it should still exist
    if (!data.getRoot(state.base.attribute)) {
        data.createRoot(state.base.attribute);
    }
}

void SpannerSetAttribute::writeMetavoxelDelta(const MetavoxelNode& root,
        const MetavoxelNode& reference, MetavoxelStreamState& state) {
    state.base.visit = Spanner::getAndIncrementNextVisit();
    root.writeSpannerDelta(reference, state);
    state.base.stream << SharedObjectPointer();
}

void SpannerSetAttribute::readMetavoxelSubdivision(MetavoxelData& data, MetavoxelStreamState& state) {
    forever {
        SharedObjectPointer object;
        state.base.stream >> object;
        if (!object) {
            break;
        }
        data.insert(state.base.attribute, object);
    }
}

void SpannerSetAttribute::writeMetavoxelSubdivision(const MetavoxelNode& root, MetavoxelStreamState& state) {
    state.base.visit = Spanner::getAndIncrementNextVisit();
    root.writeSpannerSubdivision(state);
    state.base.stream << SharedObjectPointer();
}

bool SpannerSetAttribute::metavoxelRootsEqual(const MetavoxelNode& firstRoot, const MetavoxelNode& secondRoot,
    const glm::vec3& minimum, float size, const MetavoxelLOD& lod) {
    
    SharedObjectSet firstSet;
    firstRoot.getSpanners(this, minimum, size, lod, firstSet);
    SharedObjectSet secondSet;
    secondRoot.getSpanners(this, minimum, size, lod, secondSet);
    return setsEqual(firstSet, secondSet);
}
