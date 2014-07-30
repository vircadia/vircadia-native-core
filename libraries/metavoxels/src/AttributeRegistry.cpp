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
REGISTER_META_OBJECT(HeightfieldAttribute)
REGISTER_META_OBJECT(HeightfieldColorAttribute)
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
    _heightfieldColorAttribute(registerAttribute(new HeightfieldColorAttribute("heightfieldColor"))) {
    
    // our baseline LOD threshold is for voxels; spanners and heightfields are a different story
    const float SPANNER_LOD_THRESHOLD_MULTIPLIER = 8.0f;
    _spannersAttribute->setLODThresholdMultiplier(SPANNER_LOD_THRESHOLD_MULTIPLIER);
    
    const float HEIGHTFIELD_LOD_THRESHOLD_MULTIPLIER = 32.0f;
    _heightfieldAttribute->setLODThresholdMultiplier(HEIGHTFIELD_LOD_THRESHOLD_MULTIPLIER);
    _heightfieldColorAttribute->setLODThresholdMultiplier(HEIGHTFIELD_LOD_THRESHOLD_MULTIPLIER);
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

HeightfieldData::HeightfieldData(const QByteArray& contents) :
    _contents(contents) {
}

const int BYTES_PER_PIXEL = 3;

HeightfieldData::HeightfieldData(Bitstream& in, int bytes, bool color) :
    _encoded(in.readAligned(bytes)) {
    
    QImage image = QImage::fromData(_encoded).convertToFormat(QImage::Format_RGB888);
    if (color) {
        _contents.resize(image.width() * image.height() * BYTES_PER_PIXEL);
        memcpy(_contents.data(), image.constBits(), _contents.size());
        
    } else {
        _contents.resize(image.width() * image.height());
        char* dest = _contents.data();
        for (const uchar* src = image.constBits(), *end = src + _contents.size() * BYTES_PER_PIXEL;
                src != end; src += BYTES_PER_PIXEL) {
            *dest++ = *src;
        }
    }
}

void HeightfieldData::write(Bitstream& out, bool color) {
    QMutexLocker locker(&_encodedMutex);
    if (_encoded.isEmpty()) {
        QImage image;        
        if (color) {    
            int size = glm::sqrt(_contents.size() / (float)BYTES_PER_PIXEL);
            image = QImage((uchar*)_contents.data(), size, size, QImage::Format_RGB888);
        } else {
            int size = glm::sqrt((float)_contents.size());
            image = QImage(size, size, QImage::Format_RGB888);
            uchar* dest = image.bits();
            for (const char* src = _contents.constData(), *end = src + _contents.size(); src != end; src++) {
                *dest++ = *src;
                *dest++ = *src;
                *dest++ = *src;
            }
        }
        QBuffer buffer(&_encoded);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "JPG");
    }
    out << _encoded.size();
    out.writeAligned(_encoded);
}

HeightfieldAttribute::HeightfieldAttribute(const QString& name) :
    InlineAttribute<HeightfieldDataPointer>(name) {
}

void HeightfieldAttribute::read(Bitstream& in, void*& value, bool isLeaf) const {
    if (isLeaf) { 
        int size;
        in >> size;
        if (size == 0) {
            *(HeightfieldDataPointer*)&value = HeightfieldDataPointer();
        } else {
            *(HeightfieldDataPointer*)&value = HeightfieldDataPointer(new HeightfieldData(in, size, false));
        }
    }
}

void HeightfieldAttribute::write(Bitstream& out, void* value, bool isLeaf) const {
    if (isLeaf) {
        HeightfieldDataPointer data = decodeInline<HeightfieldDataPointer>(value);
        if (data) {
            data->write(out, false);
        } else {
            out << 0;
        }
    }
}

bool HeightfieldAttribute::merge(void*& parent, void* children[], bool postRead) const {
    int maxSize = 0;
    for (int i = 0; i < MERGE_COUNT; i++) {
        HeightfieldDataPointer pointer = decodeInline<HeightfieldDataPointer>(children[i]);
        if (pointer) {
            maxSize = qMax(maxSize, pointer->getContents().size());
        }
    }
    if (maxSize == 0) {
        *(HeightfieldDataPointer*)&parent = HeightfieldDataPointer();
        return true;
    }
    int size = glm::sqrt((float)maxSize);
    QByteArray contents(size * size, 0);
    int halfSize = size / 2;
    for (int i = 0; i < MERGE_COUNT; i++) {
        HeightfieldDataPointer child = decodeInline<HeightfieldDataPointer>(children[i]);
        if (!child) {
            continue;
        }
        const QByteArray& childContents = child->getContents();
        int childSize = glm::sqrt((float)childContents.size());
        if (childSize != size) {
            continue; // TODO: handle differently-sized children
        }
        const int INDEX_MASK = 1;
        int xIndex = i & INDEX_MASK;
        const int Y_SHIFT = 1;
        int yIndex = (i >> Y_SHIFT) & INDEX_MASK;
        if (yIndex == 0 && decodeInline<HeightfieldDataPointer>(children[i | (1 << Y_SHIFT)])) {
            continue; // bottom is overriden by top
        }
        const int HALF_RANGE = 128;
        int yOffset = yIndex * HALF_RANGE;
        int Z_SHIFT = 2;
        int zIndex = (i >> Z_SHIFT) & INDEX_MASK;
        char* dest = contents.data() + (zIndex * halfSize * size) + (xIndex * halfSize);
        uchar* src0 = (uchar*)childContents.data();
        uchar* src1 = src0 + 1;
        uchar* src2 = src0 + childSize;
        uchar* src3 = src2 + 1;
        for (int z = 0; z < halfSize; z++) {
            for (char* end = dest + halfSize; dest != end; ) {
                *dest++ = yOffset + (qMax(qMax(*src0++, *src1++), qMax(*src2++, *src3++)) >> 1);
            }
            dest += halfSize;
            src0 += childSize;
            src1 += childSize;
            src2 += childSize;
            src3 += childSize;
        }
    }
    *(HeightfieldDataPointer*)&parent = HeightfieldDataPointer(new HeightfieldData(contents));
    return false;
}

HeightfieldColorAttribute::HeightfieldColorAttribute(const QString& name) :
    InlineAttribute<HeightfieldDataPointer>(name) {
}

void HeightfieldColorAttribute::read(Bitstream& in, void*& value, bool isLeaf) const {
    if (isLeaf) {
        int size;
        in >> size;
        if (size == 0) {
            *(HeightfieldDataPointer*)&value = HeightfieldDataPointer();
        } else {
            *(HeightfieldDataPointer*)&value = HeightfieldDataPointer(new HeightfieldData(in, size, true));
        }
    }
}

void HeightfieldColorAttribute::write(Bitstream& out, void* value, bool isLeaf) const {
    if (isLeaf) {
        HeightfieldDataPointer data = decodeInline<HeightfieldDataPointer>(value);
        if (data) {
            data->write(out, true);
        } else {
            out << 0;
        }
    }
}

bool HeightfieldColorAttribute::merge(void*& parent, void* children[], bool postRead) const {
    int maxSize = 0;
    for (int i = 0; i < MERGE_COUNT; i++) {
        HeightfieldDataPointer pointer = decodeInline<HeightfieldDataPointer>(children[i]);
        if (pointer) {
            maxSize = qMax(maxSize, pointer->getContents().size());
        }
    }
    if (maxSize == 0) {
        *(HeightfieldDataPointer*)&parent = HeightfieldDataPointer();
        return true;
    }
    int size = glm::sqrt(maxSize / (float)BYTES_PER_PIXEL);
    QByteArray contents(size * size * BYTES_PER_PIXEL, 0);
    int halfSize = size / 2;
    for (int i = 0; i < MERGE_COUNT; i++) {
        HeightfieldDataPointer child = decodeInline<HeightfieldDataPointer>(children[i]);
        if (!child) {
            continue;
        }
        const QByteArray& childContents = child->getContents();
        int childSize = glm::sqrt(childContents.size() / (float)BYTES_PER_PIXEL);
        if (childSize != size) {
            continue; // TODO: handle differently-sized children
        }
        const int INDEX_MASK = 1;
        int xIndex = i & INDEX_MASK;
        const int Y_SHIFT = 1;
        int yIndex = (i >> Y_SHIFT) & INDEX_MASK;
        if (yIndex == 0 && decodeInline<HeightfieldDataPointer>(children[i | (1 << Y_SHIFT)])) {
            continue; // bottom is overriden by top
        }
        int Z_SHIFT = 2;
        int zIndex = (i >> Z_SHIFT) & INDEX_MASK;
        char* dest = contents.data() + ((zIndex * halfSize * size) + (xIndex * halfSize)) * BYTES_PER_PIXEL;
        uchar* src0 = (uchar*)childContents.data();
        uchar* src1 = src0 + BYTES_PER_PIXEL;
        int childStride = childSize * BYTES_PER_PIXEL;
        uchar* src2 = src0 + childStride;
        uchar* src3 = src2 + BYTES_PER_PIXEL;
        int halfStride = halfSize * BYTES_PER_PIXEL;
        for (int z = 0; z < halfSize; z++) {
            for (char* end = dest + halfSize * BYTES_PER_PIXEL; dest != end; ) {
                *dest++ = ((int)(*src0++) + (int)(*src1++) + (int)(*src2++) + (int)(*src3++)) >> 2;
                *dest++ = ((int)(*src0++) + (int)(*src1++) + (int)(*src2++) + (int)(*src3++)) >> 2;
                *dest++ = ((int)(*src0++) + (int)(*src1++) + (int)(*src2++) + (int)(*src3++)) >> 2;
            }
            dest += halfStride;
            src0 += childStride;
            src1 += childStride;
            src2 += childStride;
            src3 += childStride;
        }
    }
    *(HeightfieldDataPointer*)&parent = HeightfieldDataPointer(new HeightfieldData(contents));
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
