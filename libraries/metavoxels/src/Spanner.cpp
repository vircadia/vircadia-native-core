//
//  Spanner.cpp
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 11/10/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <limits>

#include <QBuffer>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QItemEditorFactory>
#include <QMessageBox>
#include <QPushButton>
#include <QThread>

#include <glm/gtx/transform.hpp>

#include <GeometryUtil.h>
#include <Settings.h>

#include "MetavoxelData.h"
#include "Spanner.h"

using namespace std;

REGISTER_META_OBJECT(Spanner)
REGISTER_META_OBJECT(Heightfield)
REGISTER_META_OBJECT(Sphere)
REGISTER_META_OBJECT(Cuboid)
REGISTER_META_OBJECT(StaticModel)
REGISTER_META_OBJECT(MaterialObject)

static int heightfieldHeightTypeId = registerSimpleMetaType<HeightfieldHeightPointer>();
static int heightfieldColorTypeId = registerSimpleMetaType<HeightfieldColorPointer>();
static int heightfieldMaterialTypeId = registerSimpleMetaType<HeightfieldMaterialPointer>();

static QItemEditorCreatorBase* createHeightfieldHeightEditorCreator() {
    QItemEditorCreatorBase* creator = new LazyItemEditorCreator<HeightfieldHeightEditor>();
    getItemEditorFactory()->registerEditor(qMetaTypeId<HeightfieldHeightPointer>(), creator);
    return creator;
}

static QItemEditorCreatorBase* createHeightfieldColorEditorCreator() {
    QItemEditorCreatorBase* creator = new LazyItemEditorCreator<HeightfieldColorEditor>();
    getItemEditorFactory()->registerEditor(qMetaTypeId<HeightfieldColorPointer>(), creator);
    return creator;
}

static QItemEditorCreatorBase* heightfieldHeightEditorCreator = createHeightfieldHeightEditorCreator();
static QItemEditorCreatorBase* heightfieldColorEditorCreator = createHeightfieldColorEditorCreator();

const float DEFAULT_PLACEMENT_GRANULARITY = 0.01f;
const float DEFAULT_VOXELIZATION_GRANULARITY = powf(2.0f, -3.0f);

namespace SettingHandles {
    const SettingHandle<QString> heightfieldDir("heightDir", QString());
}

Spanner::Spanner() :
    _renderer(NULL),
    _placementGranularity(DEFAULT_PLACEMENT_GRANULARITY),
    _voxelizationGranularity(DEFAULT_VOXELIZATION_GRANULARITY),
    _merged(false),
    _willBeVoxelized(false) {
}

void Spanner::setBounds(const Box& bounds) {
    if (_bounds == bounds) {
        return;
    }
    emit boundsWillChange();
    emit boundsChanged(_bounds = bounds);
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

bool Spanner::isHeightfield() const {
    return false;
}

float Spanner::getHeight(const glm::vec3& location) const {
    return -FLT_MAX;
}

bool Spanner::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    return _bounds.findRayIntersection(origin, direction, distance);
}

Spanner* Spanner::paintHeight(const glm::vec3& position, float radius, float height, bool set, bool erase, float granularity) {
    return this;
}

Spanner* Spanner::fillHeight(const glm::vec3& position, float radius, float granularity) {
    return this;
}

Spanner* Spanner::setMaterial(const SharedObjectPointer& spanner, const SharedObjectPointer& material,
        const QColor& color, bool paint, bool voxelize, float granularity) {
    return this;
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

void SpannerRenderer::render(const MetavoxelLOD& lod, bool contained, bool cursor) {
    // nothing by default
}

bool SpannerRenderer::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
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

bool Sphere::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
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

bool StaticModel::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    // delegate to renderer, if we have one
    return _renderer ? _renderer->findRayIntersection(origin, direction, distance) :
        Spanner::findRayIntersection(origin, direction, distance);
}

QByteArray StaticModel::getRendererClassName() const {
    return "StaticModelRenderer";
}

DataBlock::~DataBlock() {
}

const int HeightfieldData::SHARED_EDGE = 1;

HeightfieldData::HeightfieldData(int width) :
    _width(width) {
}

const int HEIGHTFIELD_DATA_HEADER_SIZE = sizeof(qint32) * 4;

static QByteArray encodeHeightfieldHeight(int offsetX, int offsetY, int width, int height, const QVector<quint16>& contents) {
    QByteArray inflated(HEIGHTFIELD_DATA_HEADER_SIZE, 0);
    qint32* header = (qint32*)inflated.data();
    *header++ = offsetX;
    *header++ = offsetY;
    *header++ = width;
    *header++ = height;
    if (!contents.isEmpty()) {
        // encode with Paeth filter (see http://en.wikipedia.org/wiki/Portable_Network_Graphics#Filtering)
        QVector<quint16> filteredContents(contents.size());
        const quint16* src = contents.constData();
        quint16* dest = filteredContents.data();
        *dest++ = *src++;
        for (quint16* end = dest + width - 1; dest != end; dest++, src++) {
            *dest = *src - src[-1];
        }
        for (int y = 1; y < height; y++) {
            *dest++ = *src - src[-width];
            src++;
            for (quint16* end = dest + width - 1; dest != end; dest++, src++) {
                int a = src[-1];
                int b = src[-width];
                int c = src[-width - 1];
                int p = a + b - c;
                int ad = abs(a - p);
                int bd = abs(b - p);
                int cd = abs(c - p);
                *dest = *src - (ad < bd ? (ad < cd ? a : c) : (bd < cd ? b : c));
            }
        }  
        inflated.append((const char*)filteredContents.constData(), filteredContents.size() * sizeof(quint16));
    }
    return qCompress(inflated);
}

static QVector<quint16> decodeHeightfieldHeight(const QByteArray& encoded, int& offsetX, int& offsetY,
        int& width, int& height) {
    QByteArray inflated = qUncompress(encoded);
    const qint32* header = (const qint32*)inflated.constData();
    offsetX = *header++;
    offsetY = *header++;
    width = *header++;
    height = *header++;
    int payloadSize = inflated.size() - HEIGHTFIELD_DATA_HEADER_SIZE;
    QVector<quint16> unfiltered(payloadSize / sizeof(quint16));
    if (!unfiltered.isEmpty()) {
        quint16* dest = unfiltered.data();
        const quint16* src = (const quint16*)(inflated.constData() + HEIGHTFIELD_DATA_HEADER_SIZE);
        *dest++ = *src++;
        for (quint16* end = dest + width - 1; dest != end; dest++, src++) {
            *dest = *src + dest[-1];
        }
        for (int y = 1; y < height; y++) {
            *dest = (*src++) + dest[-width];
            dest++;
            for (quint16* end = dest + width - 1; dest != end; dest++, src++) {
                int a = dest[-1];
                int b = dest[-width];
                int c = dest[-width - 1];
                int p = a + b - c;
                int ad = abs(a - p);
                int bd = abs(b - p);
                int cd = abs(c - p);
                *dest = *src + (ad < bd ? (ad < cd ? a : c) : (bd < cd ? b : c));
            }
        }
    }
    return unfiltered;
}

const int HeightfieldHeight::HEIGHT_BORDER = 1;
const int HeightfieldHeight::HEIGHT_EXTENSION = SHARED_EDGE + 2 * HEIGHT_BORDER;

HeightfieldHeight::HeightfieldHeight(int width, const QVector<quint16>& contents) :
    HeightfieldData(width),
    _contents(contents) {
}

HeightfieldHeight::HeightfieldHeight(Bitstream& in, int bytes) {
    read(in, bytes);
}

HeightfieldHeight::HeightfieldHeight(Bitstream& in, int bytes, const HeightfieldHeightPointer& reference) {
    if (!reference) {
        read(in, bytes);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    reference->setEncodedDelta(in.readAligned(bytes));
    reference->setDeltaData(DataBlockPointer(this));
    _width = reference->getWidth();
    _contents = reference->getContents();
    
    int offsetX, offsetY, width, height;
    QVector<quint16> delta = decodeHeightfieldHeight(reference->getEncodedDelta(), offsetX, offsetY, width, height);
    if (delta.isEmpty()) {
        return;
    }
    if (offsetX == 0) {
        _contents = delta;
        _width = width;
        return;
    }
    int minX = offsetX - 1;
    int minY = offsetY - 1;
    const quint16* src = delta.constData();
    quint16* dest = _contents.data() + minY * _width + minX;
    for (int y = 0; y < height; y++, src += width, dest += _width) {
        memcpy(dest, src, width * sizeof(quint16));
    }   
}

void HeightfieldHeight::write(Bitstream& out) {
    QMutexLocker locker(&_encodedMutex);
    if (_encoded.isEmpty()) {
        _encoded = encodeHeightfieldHeight(0, 0, _width, _contents.size() / _width, _contents);
    }
    out << _encoded.size();
    out.writeAligned(_encoded);
}

void HeightfieldHeight::writeDelta(Bitstream& out, const HeightfieldHeightPointer& reference) {
    if (!reference || reference->getWidth() != _width || reference->getContents().size() != _contents.size()) {
        write(out);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    if (reference->getEncodedDelta().isEmpty() || reference->getDeltaData() != this) {
        int height = _contents.size() / _width;
        int minX = _width, minY = height;
        int maxX = -1, maxY = -1;
        const quint16* src = _contents.constData();
        const quint16* ref = reference->getContents().constData();
        for (int y = 0; y < height; y++) {
            bool difference = false;
            for (int x = 0; x < _width; x++) {
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
        QVector<quint16> delta;
        int deltaWidth = 0, deltaHeight = 0;
        if (maxX >= minX) {
            deltaWidth = maxX - minX + 1;
            deltaHeight = maxY - minY + 1;
            delta = QVector<quint16>(deltaWidth * deltaHeight);
            quint16* dest = delta.data();
            src = _contents.constData() + minY * _width + minX;
            for (int y = 0; y < deltaHeight; y++, src += _width, dest += deltaWidth) {
                memcpy(dest, src, deltaWidth * sizeof(quint16));
            }
        }
        reference->setEncodedDelta(encodeHeightfieldHeight(minX + 1, minY + 1, deltaWidth, deltaHeight, delta));
        reference->setDeltaData(DataBlockPointer(this));
    }
    out << reference->getEncodedDelta().size();
    out.writeAligned(reference->getEncodedDelta());
}

void HeightfieldHeight::read(Bitstream& in, int bytes) {
    int offsetX, offsetY, height;
    _contents = decodeHeightfieldHeight(_encoded = in.readAligned(bytes), offsetX, offsetY, _width, height);
}

Bitstream& operator<<(Bitstream& out, const HeightfieldHeightPointer& value) {
    if (value) {
        value->write(out);
    } else {
        out << 0;
    }
    return out;
}

Bitstream& operator>>(Bitstream& in, HeightfieldHeightPointer& value) {
    int size;
    in >> size;
    if (size == 0) {
        value = HeightfieldHeightPointer();
    } else {
        value = new HeightfieldHeight(in, size);
    }
    return in;
}

template<> void Bitstream::writeRawDelta(const HeightfieldHeightPointer& value, const HeightfieldHeightPointer& reference) {
    if (value) {
        value->writeDelta(*this, reference);
    } else {
        *this << 0;
    }    
}

template<> void Bitstream::readRawDelta(HeightfieldHeightPointer& value, const HeightfieldHeightPointer& reference) {
    int size;
    *this >> size;
    if (size == 0) {
        value = HeightfieldHeightPointer(); 
    } else {
        value = new HeightfieldHeight(*this, size, reference);
    }
}

HeightfieldHeightEditor::HeightfieldHeightEditor(QWidget* parent) :
    QWidget(parent) {
    
    QHBoxLayout* layout = new QHBoxLayout();
    setLayout(layout);
    
    layout->addWidget(_select = new QPushButton("Select"));
    connect(_select, &QPushButton::clicked, this, &HeightfieldHeightEditor::select);
    
    layout->addWidget(_clear = new QPushButton("Clear"));
    connect(_clear, &QPushButton::clicked, this, &HeightfieldHeightEditor::clear);
    _clear->setEnabled(false);
}

void HeightfieldHeightEditor::setHeight(const HeightfieldHeightPointer& height) {
    if ((_height = height)) {
        _clear->setEnabled(true);
    } else {
        _clear->setEnabled(false);
    }
}

static int getHeightfieldSize(int size) {
    return (int)glm::pow(2.0f, glm::round(glm::log((float)size - HeightfieldData::SHARED_EDGE) /
        glm::log(2.0f))) + HeightfieldData::SHARED_EDGE;
}

void HeightfieldHeightEditor::select() {
    QString result = QFileDialog::getOpenFileName(this, "Select Height Image",
                                                  SettingHandles::heightfieldDir.get(),
                                                  "Images (*.png *.jpg *.bmp *.raw *.mdr)");
    if (result.isNull()) {
        return;
    }
    SettingHandles::heightfieldDir.set(QFileInfo(result).path());
    const quint16 CONVERSION_OFFSET = 1;
    QString lowerResult = result.toLower();
    bool isMDR = lowerResult.endsWith(".mdr");
    if (lowerResult.endsWith(".raw") || isMDR) {
        QFile input(result);
        input.open(QIODevice::ReadOnly);
        QDataStream in(&input);
        in.setByteOrder(QDataStream::LittleEndian);
        if (isMDR) {
            const int MDR_HEADER_SIZE = 1024;
            input.seek(MDR_HEADER_SIZE);
        }
        int available = input.bytesAvailable() / sizeof(quint16);
        QVector<quint16> rawContents(available);
        for (quint16* height = rawContents.data(), *end = height + available; height != end; height++) {
            in >> *height;
        }
        if (rawContents.isEmpty()) {
            QMessageBox::warning(this, "Invalid Image", "The selected image could not be read.");
            return;
        }
        int rawSize = glm::sqrt((float)rawContents.size());
        int size = getHeightfieldSize(rawSize) + 2 * HeightfieldHeight::HEIGHT_BORDER;
        QVector<quint16> contents(size * size);
        quint16* dest = contents.data() + (size + 1) * HeightfieldHeight::HEIGHT_BORDER;
        const quint16* src = rawContents.constData();
        const float CONVERSION_SCALE = 65534.0f / numeric_limits<quint16>::max();
        for (int i = 0; i < rawSize; i++, dest += size) {
            for (quint16* lineDest = dest, *end = dest + rawSize; lineDest != end; lineDest++, src++) {
                *lineDest = (quint16)(*src * CONVERSION_SCALE) + CONVERSION_OFFSET;
            }
        }
        emit heightChanged(_height = new HeightfieldHeight(size, contents));
        _clear->setEnabled(true);
        return;
    }
    QImage image;
    if (!image.load(result)) {
        QMessageBox::warning(this, "Invalid Image", "The selected image could not be read.");
        return;
    }
    image = image.convertToFormat(QImage::Format_ARGB32);
    int width = getHeightfieldSize(image.width()) + 2 * HeightfieldHeight::HEIGHT_BORDER;
    int height = getHeightfieldSize(image.height()) + 2 * HeightfieldHeight::HEIGHT_BORDER;
    QVector<quint16> contents(width * height);
    quint16* dest = contents.data() + (width + 1) * HeightfieldHeight::HEIGHT_BORDER;
    const float CONVERSION_SCALE = 65534.0f / numeric_limits<quint8>::max();
    for (int i = 0; i < image.height(); i++, dest += width) {
        const QRgb* src = (const QRgb*)image.constScanLine(i);
        for (quint16* lineDest = dest, *end = dest + image.width(); lineDest != end; lineDest++, src++) {
            *lineDest = (qAlpha(*src) < numeric_limits<qint8>::max()) ? 0 :
                (quint16)(qRed(*src) * CONVERSION_SCALE) + CONVERSION_OFFSET;
        }
    }
    emit heightChanged(_height = new HeightfieldHeight(width, contents));
    _clear->setEnabled(true);
}

void HeightfieldHeightEditor::clear() {
    emit heightChanged(_height = HeightfieldHeightPointer());
    _clear->setEnabled(false);
}

static QByteArray encodeHeightfieldColor(int offsetX, int offsetY, int width, int height, const QByteArray& contents) {
    QByteArray inflated(HEIGHTFIELD_DATA_HEADER_SIZE, 0);
    qint32* header = (qint32*)inflated.data();
    *header++ = offsetX;
    *header++ = offsetY;
    *header++ = width;
    *header++ = height;
    if (!contents.isEmpty()) {
        QBuffer buffer(&inflated);
        buffer.open(QIODevice::WriteOnly | QIODevice::Append);
        QImage((const uchar*)contents.constData(), width, height, width * DataBlock::COLOR_BYTES,
            QImage::Format_RGB888).save(&buffer, "JPG");
    }
    return qCompress(inflated);
}

static QByteArray decodeHeightfieldColor(const QByteArray& encoded, int& offsetX, int& offsetY, int& width, int& height) {
    QByteArray inflated = qUncompress(encoded);
    const qint32* header = (const qint32*)inflated.constData();
    offsetX = *header++;
    offsetY = *header++;
    width = *header++;
    height = *header++;
    int payloadSize = inflated.size() - HEIGHTFIELD_DATA_HEADER_SIZE;
    if (payloadSize == 0) {
        return QByteArray();
    }
    QImage image = QImage::fromData((const uchar*)inflated.constData() + HEIGHTFIELD_DATA_HEADER_SIZE, payloadSize, "JPG");
    if (image.format() != QImage::Format_RGB888) {
        image = image.convertToFormat(QImage::Format_RGB888);
    }
    QByteArray contents(width * height * DataBlock::COLOR_BYTES, 0);
    char* dest = contents.data();
    int stride = width * DataBlock::COLOR_BYTES;
    for (int y = 0; y < height; y++, dest += stride) {
        memcpy(dest, image.constScanLine(y), stride);
    }
    return contents;
}

HeightfieldColor::HeightfieldColor(int width, const QByteArray& contents) :
    HeightfieldData(width),
    _contents(contents) {
}

HeightfieldColor::HeightfieldColor(Bitstream& in, int bytes) {
    read(in, bytes);
}

HeightfieldColor::HeightfieldColor(Bitstream& in, int bytes, const HeightfieldColorPointer& reference) {
    if (!reference) {
        read(in, bytes);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    reference->setEncodedDelta(in.readAligned(bytes));
    reference->setDeltaData(DataBlockPointer(this));
    _width = reference->getWidth();
    _contents = reference->getContents();
    
    int offsetX, offsetY, width, height;
    QByteArray delta = decodeHeightfieldColor(reference->getEncodedDelta(), offsetX, offsetY, width, height);
    if (delta.isEmpty()) {
        return;
    }
    if (offsetX == 0) {
        _contents = delta;
        _width = width;
        return;
    }
    int minX = offsetX - 1;
    int minY = offsetY - 1;
    const char* src = delta.constData();
    char* dest = _contents.data() + (minY * _width + minX) * DataBlock::COLOR_BYTES;
    for (int y = 0; y < height; y++, src += width * DataBlock::COLOR_BYTES, dest += _width * DataBlock::COLOR_BYTES) {
        memcpy(dest, src, width * DataBlock::COLOR_BYTES);
    }   
}

void HeightfieldColor::write(Bitstream& out) {
    QMutexLocker locker(&_encodedMutex);
    if (_encoded.isEmpty()) {
        _encoded = encodeHeightfieldColor(0, 0, _width, _contents.size() / (_width * DataBlock::COLOR_BYTES), _contents);
    }
    out << _encoded.size();
    out.writeAligned(_encoded);
}

void HeightfieldColor::writeDelta(Bitstream& out, const HeightfieldColorPointer& reference) {
    if (!reference || reference->getWidth() != _width || reference->getContents().size() != _contents.size()) {
        write(out);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    if (reference->getEncodedDelta().isEmpty() || reference->getDeltaData() != this) {
        int height = _contents.size() / (_width * DataBlock::COLOR_BYTES);
        int minX = _width, minY = height;
        int maxX = -1, maxY = -1;
        const char* src = _contents.constData();
        const char* ref = reference->getContents().constData();
        for (int y = 0; y < height; y++) {
            bool difference = false;
            for (int x = 0; x < _width; x++, src += DataBlock::COLOR_BYTES, ref += DataBlock::COLOR_BYTES) {
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
        QByteArray delta;
        int deltaWidth = 0, deltaHeight = 0;
        if (maxX >= minX) {
            deltaWidth = maxX - minX + 1;
            deltaHeight = maxY - minY + 1;
            delta = QByteArray(deltaWidth * deltaHeight * DataBlock::COLOR_BYTES, 0);
            char* dest = delta.data();
            src = _contents.constData() + (minY * _width + minX) * DataBlock::COLOR_BYTES;
            for (int y = 0; y < deltaHeight; y++, src += _width * DataBlock::COLOR_BYTES,
                    dest += deltaWidth * DataBlock::COLOR_BYTES) {
                memcpy(dest, src, deltaWidth * DataBlock::COLOR_BYTES);
            }
        }
        reference->setEncodedDelta(encodeHeightfieldColor(minX + 1, minY + 1, deltaWidth, deltaHeight, delta));
        reference->setDeltaData(DataBlockPointer(this));
    }
    out << reference->getEncodedDelta().size();
    out.writeAligned(reference->getEncodedDelta());
}

void HeightfieldColor::read(Bitstream& in, int bytes) {
    int offsetX, offsetY, height;
    _contents = decodeHeightfieldColor(_encoded = in.readAligned(bytes), offsetX, offsetY, _width, height);
}

Bitstream& operator<<(Bitstream& out, const HeightfieldColorPointer& value) {
    if (value) {
        value->write(out);
    } else {
        out << 0;
    }
    return out;
}

Bitstream& operator>>(Bitstream& in, HeightfieldColorPointer& value) {
    int size;
    in >> size;
    if (size == 0) {
        value = HeightfieldColorPointer();
    } else {
        value = new HeightfieldColor(in, size);
    }
    return in;
}

template<> void Bitstream::writeRawDelta(const HeightfieldColorPointer& value, const HeightfieldColorPointer& reference) {
    if (value) {
        value->writeDelta(*this, reference);
    } else {
        *this << 0;
    }    
}

template<> void Bitstream::readRawDelta(HeightfieldColorPointer& value, const HeightfieldColorPointer& reference) {
    int size;
    *this >> size;
    if (size == 0) {
        value = HeightfieldColorPointer(); 
    } else {
        value = new HeightfieldColor(*this, size, reference);
    }
}

HeightfieldColorEditor::HeightfieldColorEditor(QWidget* parent) :
    QWidget(parent) {
    
    QHBoxLayout* layout = new QHBoxLayout();
    setLayout(layout);
    
    layout->addWidget(_select = new QPushButton("Select"));
    connect(_select, &QPushButton::clicked, this, &HeightfieldColorEditor::select);
    
    layout->addWidget(_clear = new QPushButton("Clear"));
    connect(_clear, &QPushButton::clicked, this, &HeightfieldColorEditor::clear);
    _clear->setEnabled(false);
}

void HeightfieldColorEditor::setColor(const HeightfieldColorPointer& color) {
    if ((_color = color)) {
        _clear->setEnabled(true);
    } else {
        _clear->setEnabled(false);
    }
}

void HeightfieldColorEditor::select() {
    QString result = QFileDialog::getOpenFileName(this, "Select Color Image",
                                                  SettingHandles::heightfieldDir.get(),
        "Images (*.png *.jpg *.bmp)");
    if (result.isNull()) {
        return;
    }
    SettingHandles::heightfieldDir.get(QFileInfo(result).path());
    QImage image;
    if (!image.load(result)) {
        QMessageBox::warning(this, "Invalid Image", "The selected image could not be read.");
        return;
    }
    image = image.convertToFormat(QImage::Format_RGB888);
    int width = getHeightfieldSize(image.width());
    int height = getHeightfieldSize(image.height());
    QByteArray contents(width * height * DataBlock::COLOR_BYTES, 0);
    char* dest = contents.data();
    for (int i = 0; i < image.height(); i++, dest += width * DataBlock::COLOR_BYTES) {
        memcpy(dest, image.constScanLine(i), image.width() * DataBlock::COLOR_BYTES);
    }
    emit colorChanged(_color = new HeightfieldColor(width, contents));
    _clear->setEnabled(true);
}

void HeightfieldColorEditor::clear() {
    emit colorChanged(_color = HeightfieldColorPointer());
    _clear->setEnabled(false);
}

static QByteArray encodeHeightfieldMaterial(int offsetX, int offsetY, int width, int height, const QByteArray& contents) {
    QByteArray inflated(HEIGHTFIELD_DATA_HEADER_SIZE, 0);
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
    return inflated.mid(HEIGHTFIELD_DATA_HEADER_SIZE);
}

HeightfieldMaterial::HeightfieldMaterial(int width, const QByteArray& contents,
        const QVector<SharedObjectPointer>& materials) :
    HeightfieldData(width),
    _contents(contents),
    _materials(materials) {
}

HeightfieldMaterial::HeightfieldMaterial(Bitstream& in, int bytes) {
    read(in, bytes);
}

HeightfieldMaterial::HeightfieldMaterial(Bitstream& in, int bytes, const HeightfieldMaterialPointer& reference) {
    if (!reference) {
        read(in, bytes);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    reference->setEncodedDelta(in.readAligned(bytes));
    in.readDelta(_materials, reference->getMaterials());
    reference->setDeltaData(DataBlockPointer(this));
    _width = reference->getWidth();
    _contents = reference->getContents();
 
    int offsetX, offsetY, width, height;
    QByteArray delta = decodeHeightfieldMaterial(reference->getEncodedDelta(), offsetX, offsetY, width, height);
    if (delta.isEmpty()) {
        return;
    }
    if (offsetX == 0) {
        _contents = delta;
        _width = width;
        return;
    }
    int minX = offsetX - 1;
    int minY = offsetY - 1;
    const char* src = delta.constData();
    char* dest = _contents.data() + minY * _width + minX;
    for (int y = 0; y < height; y++, src += width, dest += _width) {
        memcpy(dest, src, width);
    }   
}

void HeightfieldMaterial::write(Bitstream& out) {
    QMutexLocker locker(&_encodedMutex);
    if (_encoded.isEmpty()) {
        _encoded = encodeHeightfieldMaterial(0, 0, _width, _contents.size() / _width, _contents);
    }
    out << _encoded.size();
    out.writeAligned(_encoded);
    out << _materials;
}

void HeightfieldMaterial::writeDelta(Bitstream& out, const HeightfieldMaterialPointer& reference) {
    if (!reference) {
        write(out);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    if (reference->getEncodedDelta().isEmpty() || reference->getDeltaData() != this) {
        if (reference->getWidth() != _width || reference->getContents().size() != _contents.size()) {
            reference->setEncodedDelta(encodeHeightfieldMaterial(0, 0, _width, _contents.size() / _width, _contents));
               
        } else {
            int height = _contents.size() / _width;
            int minX = _width, minY = height;
            int maxX = -1, maxY = -1;
            const char* src = _contents.constData();
            const char* ref = reference->getContents().constData();
            for (int y = 0; y < height; y++) {
                bool difference = false;
                for (int x = 0; x < _width; x++) {
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
            int deltaWidth = 0, deltaHeight = 0;
            if (maxX >= minX) {
                deltaWidth = maxX - minX + 1;
                deltaHeight = maxY - minY + 1;
                delta = QByteArray(deltaWidth * deltaHeight, 0);
                char* dest = delta.data();
                src = _contents.constData() + minY * _width + minX;
                for (int y = 0; y < deltaHeight; y++, src += _width, dest += deltaWidth) {
                    memcpy(dest, src, deltaWidth);
                }
            }
            reference->setEncodedDelta(encodeHeightfieldMaterial(minX + 1, minY + 1, deltaWidth, deltaHeight, delta));
        }
        reference->setDeltaData(DataBlockPointer(this));
    }
    out << reference->getEncodedDelta().size();
    out.writeAligned(reference->getEncodedDelta());
    out.writeDelta(_materials, reference->getMaterials());
}

void HeightfieldMaterial::read(Bitstream& in, int bytes) {
    int offsetX, offsetY, height;
    _contents = decodeHeightfieldMaterial(_encoded = in.readAligned(bytes), offsetX, offsetY, _width, height);
    in >> _materials;
}

Bitstream& operator<<(Bitstream& out, const HeightfieldMaterialPointer& value) {
    if (value) {
        value->write(out);
    } else {
        out << 0;
    }
    return out;
}

Bitstream& operator>>(Bitstream& in, HeightfieldMaterialPointer& value) {
    int size;
    in >> size;
    if (size == 0) {
        value = HeightfieldMaterialPointer();
    } else {
        value = new HeightfieldMaterial(in, size);
    }
    return in;
}

template<> void Bitstream::writeRawDelta(const HeightfieldMaterialPointer& value,
        const HeightfieldMaterialPointer& reference) {
    if (value) {
        value->writeDelta(*this, reference);
    } else {
        *this << 0;
    }    
}

template<> void Bitstream::readRawDelta(HeightfieldMaterialPointer& value, const HeightfieldMaterialPointer& reference) {
    int size;
    *this >> size;
    if (size == 0) {
        value = HeightfieldMaterialPointer(); 
    } else {
        value = new HeightfieldMaterial(*this, size, reference);
    }
}

MaterialObject::MaterialObject() :
    _scaleS(1.0f),
    _scaleT(1.0f) {
}

int getMaterialIndex(const SharedObjectPointer& material, QVector<SharedObjectPointer>& materials) {
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
    if (materials.size() < numeric_limits<quint8>::max()) {
        materials.append(material);
        return materials.size();
    }
    return -1;
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

static uchar getMaterialIndex(const SharedObjectPointer& material, QVector<SharedObjectPointer>& materials,
        QByteArray& contents) {
    int index = getMaterialIndex(material, materials);
    if (index != -1) {
        return index;
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

static void clearUnusedMaterials(QVector<SharedObjectPointer>& materials, const QByteArray& contents) {
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

static QHash<uchar, int> countIndices(const QVector<StackArray>& contents) {
    QHash<uchar, int> counts;
    foreach (const StackArray& array, contents) {
        if (array.isEmpty()) {
            continue;
        }
        for (const StackArray::Entry* entry = array.getEntryData(), *end = entry + array.getEntryCount();
                entry != end; entry++) {
            if (entry->material != 0) {
                counts[entry->material]++;
            }
        }
    }
    return counts;
}

static uchar getMaterialIndex(const SharedObjectPointer& material, QVector<SharedObjectPointer>& materials,
        QVector<StackArray>& contents) {
    int index = getMaterialIndex(material, materials);
    if (index != -1) {
        return index;
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
    for (StackArray* array = contents.data(), *end = array + contents.size(); array != end; array++) {        
        if (array->isEmpty()) {
            continue;
        }
        for (StackArray::Entry* entry = array->getEntryData(), *end = entry + array->getEntryCount();
                entry != end; entry++) {
            if (entry->material == materialIndex) {
                entry->material = 0;
            }
        }
    }
    return materialIndex;
}

static void clearUnusedMaterials(QVector<SharedObjectPointer>& materials, const QVector<StackArray>& contents) {
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

static QByteArray encodeHeightfieldStack(int offsetX, int offsetY, int width, int height,
        const QVector<StackArray>& contents) {
    QByteArray inflated(HEIGHTFIELD_DATA_HEADER_SIZE, 0);
    qint32* header = (qint32*)inflated.data();
    *header++ = offsetX;
    *header++ = offsetY;
    *header++ = width;
    *header++ = height;
    foreach (const StackArray& stack, contents) {
        quint16 entries = stack.getEntryCount();
        inflated.append((const char*)&entries, sizeof(quint16));
        inflated.append(stack);
    }
    return qCompress(inflated);
}

static QVector<StackArray> decodeHeightfieldStack(const QByteArray& encoded,
        int& offsetX, int& offsetY, int& width, int& height) {
    QByteArray inflated = qUncompress(encoded);
    const qint32* header = (const qint32*)inflated.constData();
    offsetX = *header++;
    offsetY = *header++;
    width = *header++;
    height = *header++;
    const char* src = inflated.constData() + HEIGHTFIELD_DATA_HEADER_SIZE;
    QVector<StackArray> contents(width * height);
    for (StackArray* dest = contents.data(), *end = dest + contents.size(); dest != end; dest++) {
        int entries = *(const quint16*)src;
        src += sizeof(quint16);
        if (entries > 0) {
            int bytes = StackArray::getSize(entries);
            *dest = StackArray(src, bytes);
            src += bytes;
        }
    }
    return contents;
}

StackArray::Entry::Entry() :
    color(0),
    material(0),
    hermiteX(0),
    hermiteY(0),
    hermiteZ(0) {
}

bool StackArray::Entry::isZero() const {
    return color == 0 && material == 0 && hermiteX == 0 && hermiteY == 0 && hermiteZ == 0;
}

bool StackArray::Entry::isMergeable(const Entry& other) const {
    return color == other.color && material == other.material && hermiteX == 0 && hermiteY == 0 && hermiteZ == 0;
}

static inline void setHermite(quint32& value, const glm::vec3& normal, float position) {
    value = qRgba(normal.x * numeric_limits<qint8>::max(), normal.y * numeric_limits<qint8>::max(),
        normal.z * numeric_limits<qint8>::max(), position * numeric_limits<quint8>::max());
}

static inline float getHermite(QRgb value, glm::vec3& normal) {
    normal.x = (char)qRed(value) / (float)numeric_limits<qint8>::max();
    normal.y = (char)qGreen(value) / (float)numeric_limits<qint8>::max();
    normal.z = (char)qBlue(value) / (float)numeric_limits<qint8>::max();
    float length = glm::length(normal);
    if (length > 0.0f) {
        normal /= length;
    }
    return qAlpha(value) / (float)numeric_limits<quint8>::max();   
}

void StackArray::Entry::setHermiteX(const glm::vec3& normal, float position) {
    setHermite(hermiteX, normal, position);
}

float StackArray::Entry::getHermiteX(glm::vec3& normal) const {
    return getHermite(hermiteX, normal);
}

void StackArray::Entry::setHermiteY(const glm::vec3& normal, float position) {
    setHermite(hermiteY, normal, position);
}

float StackArray::Entry::getHermiteY(glm::vec3& normal) const {
    return getHermite(hermiteY, normal);
}

void StackArray::Entry::setHermiteZ(const glm::vec3& normal, float position) {
    setHermite(hermiteZ, normal, position);
}

float StackArray::Entry::getHermiteZ(glm::vec3& normal) const {
    return getHermite(hermiteZ, normal);
}

int StackArray::getEntryAlpha(int y, float heightfieldHeight) const {
    int count = getEntryCount();
    if (count != 0) {
        int relative = y - getPosition();
        if (relative < count && (relative >= 0 || heightfieldHeight == 0.0f || y < heightfieldHeight)) {
            return qAlpha(getEntryData()[qMax(relative, 0)].color);
        }
    }
    return (heightfieldHeight != 0.0f && y <= heightfieldHeight) ? numeric_limits<quint8>::max() : 0;
}

StackArray::Entry& StackArray::getEntry(int y, float heightfieldHeight) {
    static Entry emptyEntry;
    int count = getEntryCount();
    if (count != 0) {
        int relative = y - getPosition();
        if (relative < count && (relative >= 0 || heightfieldHeight == 0.0f || y < heightfieldHeight)) {
            return getEntryData()[qMax(relative, 0)];
        }
    }
    return emptyEntry;
}

const StackArray::Entry& StackArray::getEntry(int y, float heightfieldHeight) const {
    static Entry emptyEntry;
    int count = getEntryCount();
    if (count != 0) {
        int relative = y - getPosition();
        if (relative < count && (relative >= 0 || heightfieldHeight == 0.0f || y < heightfieldHeight)) {
            return getEntryData()[qMax(relative, 0)];
        }
    }
    return emptyEntry;
}

void StackArray::getExtents(int& minimumY, int& maximumY) const {
    int count = getEntryCount();
    if (count > 0) {
        int position = getPosition();
        minimumY = qMin(minimumY, position);
        maximumY = qMax(maximumY, position + count - 1);
    }
}

bool StackArray::hasSetEntries() const {
    int count = getEntryCount();
    if (count > 0) {
        for (const Entry* entry = getEntryData(), *end = entry + count; entry != end; entry++) {
            if (entry->isSet()) {
                return true;
            }
        }
    }
    return false;
}

HeightfieldStack::HeightfieldStack(int width, const QVector<StackArray>& contents,
        const QVector<SharedObjectPointer>& materials) :
    HeightfieldData(width),
    _contents(contents),
    _materials(materials) {
}

HeightfieldStack::HeightfieldStack(Bitstream& in, int bytes) {
    read(in, bytes);
}

HeightfieldStack::HeightfieldStack(Bitstream& in, int bytes, const HeightfieldStackPointer& reference) {
    if (!reference) {
        read(in, bytes);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    reference->setEncodedDelta(in.readAligned(bytes));
    in.readDelta(_materials, reference->getMaterials());
    reference->setDeltaData(DataBlockPointer(this));
    _width = reference->getWidth();
    _contents = reference->getContents();
 
    int offsetX, offsetY, width, height;
    QVector<StackArray> delta = decodeHeightfieldStack(reference->getEncodedDelta(), offsetX, offsetY, width, height);
    if (delta.isEmpty()) {
        return;
    }
    if (offsetX == 0) {
        _contents = delta;
        _width = width;
        return;
    }
    int minX = offsetX - 1;
    int minY = offsetY - 1;
    const StackArray* src = delta.constData();
    StackArray* dest = _contents.data() + minY * _width + minX;
    for (int y = 0; y < height; y++, src += width, dest += _width) {
        const StackArray* lineSrc = src;
        for (StackArray* lineDest = dest, *end = dest + width; lineDest != end; lineDest++, lineSrc++) {
            *lineDest = *lineSrc;
        }
    }   
}

void HeightfieldStack::write(Bitstream& out) {
    QMutexLocker locker(&_encodedMutex);
    if (_encoded.isEmpty()) {
        _encoded = encodeHeightfieldStack(0, 0, _width, _contents.size() / _width, _contents);
    }
    out << _encoded.size();
    out.writeAligned(_encoded);
    out << _materials;
}

void HeightfieldStack::writeDelta(Bitstream& out, const HeightfieldStackPointer& reference) {
    if (!reference) {
        write(out);
        return;
    }
    QMutexLocker locker(&reference->getEncodedDeltaMutex());
    if (reference->getEncodedDelta().isEmpty() || reference->getDeltaData() != this) {
        if (reference->getWidth() != _width || reference->getContents().size() != _contents.size()) {
            reference->setEncodedDelta(encodeHeightfieldStack(0, 0, _width, _contents.size() / _width, _contents));
               
        } else {
            int height = _contents.size() / _width;
            int minX = _width, minY = height;
            int maxX = -1, maxY = -1;
            const StackArray* src = _contents.constData();
            const StackArray* ref = reference->getContents().constData();
            for (int y = 0; y < height; y++) {
                bool difference = false;
                for (int x = 0; x < _width; x++) {
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
            QVector<StackArray> delta;
            int deltaWidth = 0, deltaHeight = 0;
            if (maxX >= minX) {
                deltaWidth = maxX - minX + 1;
                deltaHeight = maxY - minY + 1;
                delta = QVector<StackArray>(deltaWidth * deltaHeight);
                StackArray* dest = delta.data();
                src = _contents.constData() + minY * _width + minX;
                for (int y = 0; y < deltaHeight; y++, src += _width, dest += deltaWidth) {
                    const StackArray* lineSrc = src;
                    for (StackArray* lineDest = dest, *end = dest + deltaWidth; lineDest != end; lineDest++, lineSrc++) {
                        *lineDest = *lineSrc;
                    }
                }
            }
            reference->setEncodedDelta(encodeHeightfieldStack(minX + 1, minY + 1, deltaWidth, deltaHeight, delta));
        }
        reference->setDeltaData(DataBlockPointer(this));
    }
    out << reference->getEncodedDelta().size();
    out.writeAligned(reference->getEncodedDelta());
    out.writeDelta(_materials, reference->getMaterials());
}

void HeightfieldStack::read(Bitstream& in, int bytes) {
    int offsetX, offsetY, height;
    _contents = decodeHeightfieldStack(_encoded = in.readAligned(bytes), offsetX, offsetY, _width, height);
    in >> _materials;
}

Bitstream& operator<<(Bitstream& out, const HeightfieldStackPointer& value) {
    if (value) {
        value->write(out);
    } else {
        out << 0;
    }
    return out;
}

Bitstream& operator>>(Bitstream& in, HeightfieldStackPointer& value) {
    int size;
    in >> size;
    if (size == 0) {
        value = HeightfieldStackPointer();
    } else {
        value = new HeightfieldStack(in, size);
    }
    return in;
}

template<> void Bitstream::writeRawDelta(const HeightfieldStackPointer& value, const HeightfieldStackPointer& reference) {
    if (value) {
        value->writeDelta(*this, reference);
    } else {
        *this << 0;
    }    
}

template<> void Bitstream::readRawDelta(HeightfieldStackPointer& value, const HeightfieldStackPointer& reference) {
    int size;
    *this >> size;
    if (size == 0) {
        value = HeightfieldStackPointer(); 
    } else {
        value = new HeightfieldStack(*this, size, reference);
    }
}

bool HeightfieldStreamState::shouldSubdivide() const {
    return base.lod.shouldSubdivide(minimum, size);
}

bool HeightfieldStreamState::shouldSubdivideReference() const {
    return base.referenceLOD.shouldSubdivide(minimum, size);
}

bool HeightfieldStreamState::becameSubdivided() const {
    return base.lod.becameSubdivided(minimum, size, base.referenceLOD);
}

bool HeightfieldStreamState::becameSubdividedOrCollapsed() const {
    return base.lod.becameSubdividedOrCollapsed(minimum, size, base.referenceLOD);
}

const int X_MAXIMUM_FLAG = 1;
const int Y_MAXIMUM_FLAG = 2;

static glm::vec2 getNextMinimum(const glm::vec2& minimum, float nextSize, int index) {
    return minimum + glm::vec2(
        (index & X_MAXIMUM_FLAG) ? nextSize : 0.0f,
        (index & Y_MAXIMUM_FLAG) ? nextSize : 0.0f);
}

void HeightfieldStreamState::setMinimum(const glm::vec2& lastMinimum, int index) {
    minimum = getNextMinimum(lastMinimum, size, index);
}

HeightfieldNode::HeightfieldNode(const HeightfieldHeightPointer& height, const HeightfieldColorPointer& color,
        const HeightfieldMaterialPointer& material, const HeightfieldStackPointer& stack) :
    _height(height),
    _color(color),
    _material(material),
    _stack(stack),
    _renderer(NULL) {
}

HeightfieldNode::HeightfieldNode(const HeightfieldNode& other) :
    _height(other.getHeight()),
    _color(other.getColor()),
    _material(other.getMaterial()),
    _stack(other.getStack()),
    _renderer(NULL) {
    
    for (int i = 0; i < CHILD_COUNT; i++) {
        _children[i] = other.getChild(i);
    }
}

HeightfieldNode::~HeightfieldNode() {
    delete _renderer;
}

const int HEIGHT_LEAF_SIZE = 256 + HeightfieldHeight::HEIGHT_EXTENSION;

void HeightfieldNode::setContents(const HeightfieldHeightPointer& height, const HeightfieldColorPointer& color,
        const HeightfieldMaterialPointer& material, const HeightfieldStackPointer& stack) {
    clearChildren();
    
    int heightWidth = height->getWidth();
    if (heightWidth <= HEIGHT_LEAF_SIZE) {
        _height = height;
        _color = color;
        _material = material;
        return;
    }
    int heightHeight = height->getContents().size() / heightWidth;
    int innerChildHeightWidth = (heightWidth - HeightfieldHeight::HEIGHT_EXTENSION) / 2;
    int innerChildHeightHeight = (heightHeight - HeightfieldHeight::HEIGHT_EXTENSION) / 2;
    int childHeightWidth = innerChildHeightWidth + HeightfieldHeight::HEIGHT_EXTENSION;
    int childHeightHeight = innerChildHeightHeight + HeightfieldHeight::HEIGHT_EXTENSION;
    
    for (int i = 0; i < CHILD_COUNT; i++) {
        QVector<quint16> childHeightContents(childHeightWidth * childHeightHeight);
        quint16* heightDest = childHeightContents.data();
        bool maximumX = (i & X_MAXIMUM_FLAG), maximumY = (i & Y_MAXIMUM_FLAG);
        const quint16* heightSrc = height->getContents().constData() + (maximumY ? innerChildHeightHeight * heightWidth : 0) +
            (maximumX ? innerChildHeightWidth : 0);
        for (int z = 0; z < childHeightHeight; z++, heightDest += childHeightWidth, heightSrc += heightWidth) {
            memcpy(heightDest, heightSrc, childHeightWidth * sizeof(quint16));
        }
        
        HeightfieldColorPointer childColor;
        if (color) {
            int colorWidth = color->getWidth();
            int colorHeight = color->getContents().size() / (colorWidth * DataBlock::COLOR_BYTES);
            int innerChildColorWidth = (colorWidth - HeightfieldData::SHARED_EDGE) / 2;
            int innerChildColorHeight = (colorHeight - HeightfieldData::SHARED_EDGE) / 2;
            int childColorWidth = innerChildColorWidth + HeightfieldData::SHARED_EDGE;
            int childColorHeight = innerChildColorHeight + HeightfieldData::SHARED_EDGE;
            QByteArray childColorContents(childColorWidth * childColorHeight * DataBlock::COLOR_BYTES, 0);
            char* dest = childColorContents.data();
            const char* src = color->getContents().constData() + ((maximumY ? innerChildColorHeight * colorWidth : 0) +
                (maximumX ? innerChildColorWidth : 0)) * DataBlock::COLOR_BYTES;
            for (int z = 0; z < childColorHeight; z++, dest += childColorWidth * DataBlock::COLOR_BYTES,
                    src += colorWidth * DataBlock::COLOR_BYTES) {
                memcpy(dest, src, childColorWidth * DataBlock::COLOR_BYTES);
            }
            childColor = new HeightfieldColor(childColorWidth, childColorContents);
        }
        
        HeightfieldMaterialPointer childMaterial;
        if (material) {
            int materialWidth = material->getWidth();
            int materialHeight = material->getContents().size() / materialWidth;
            int innerChildMaterialWidth = (materialWidth - HeightfieldData::SHARED_EDGE) / 2;
            int innerChildMaterialHeight = (materialHeight - HeightfieldData::SHARED_EDGE) / 2;
            int childMaterialWidth = innerChildMaterialWidth + HeightfieldData::SHARED_EDGE;
            int childMaterialHeight = innerChildMaterialHeight + HeightfieldData::SHARED_EDGE;
            QByteArray childMaterialContents(childMaterialWidth * childMaterialHeight, 0);
            QVector<SharedObjectPointer> childMaterials;
            uchar* dest = (uchar*)childMaterialContents.data();
            const uchar* src = (const uchar*)material->getContents().data() +
                (maximumY ? innerChildMaterialHeight * materialWidth : 0) + (maximumX ? innerChildMaterialWidth : 0);
            QHash<int, int> materialMap;
            for (int z = 0; z < childMaterialHeight; z++, dest += childMaterialWidth, src += materialWidth) {
                const uchar* lineSrc = src;
                for (uchar* lineDest = dest, *end = dest + childMaterialWidth; lineDest != end; lineDest++, lineSrc++) {
                    int value = *lineSrc;
                    if (value != 0) {
                        int& mapping = materialMap[value];
                        if (mapping == 0) {
                            childMaterials.append(material->getMaterials().at(value - 1));
                            mapping = childMaterials.size();
                        }
                        value = mapping;
                    }
                    *lineDest = value;
                }
            }
            childMaterial = new HeightfieldMaterial(childMaterialWidth, childMaterialContents, childMaterials);
        }
    
        HeightfieldStackPointer childStack;
        if (stack) {
        }
    
        _children[i] = new HeightfieldNode();
        _children[i]->setContents(HeightfieldHeightPointer(new HeightfieldHeight(childHeightWidth, childHeightContents)),
            childColor, childMaterial, childStack);
    }
    
    mergeChildren();
}

bool HeightfieldNode::isLeaf() const {
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (_children[i]) {
            return false;
        }
    }
    return true;
}

bool HeightfieldNode::findRayIntersection(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale,
        const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    glm::quat inverseRotation = glm::inverse(rotation);
    glm::vec3 inverseScale = 1.0f / scale;
    glm::vec3 transformedOrigin = inverseRotation * (origin - translation) * inverseScale; 
    glm::vec3 transformedDirection = inverseRotation * direction * inverseScale;
    float boundsDistance;
    if (!Box(glm::vec3(), glm::vec3(1.0f, 1.0f, 1.0f)).findRayIntersection(transformedOrigin, transformedDirection,
            boundsDistance)) {
        return false;
    }
    if (!isLeaf()) {
        float closestDistance = FLT_MAX;
        for (int i = 0; i < CHILD_COUNT; i++) {
            glm::vec3 nextScale = scale * glm::vec3(0.5f, 1.0f, 0.5f);    
            float childDistance;
            if (_children[i]->findRayIntersection(translation +
                    rotation * glm::vec3(i & X_MAXIMUM_FLAG ? nextScale.x : 0.0f, 0.0f,
                        i & Y_MAXIMUM_FLAG ? nextScale.z : 0.0f), rotation,
                    nextScale, origin, direction, childDistance)) {
                closestDistance = qMin(closestDistance, childDistance);
            }
        }
        if (closestDistance == FLT_MAX) {
            return false;
        }
        distance = closestDistance;
        return true;
    }
    float shortestDistance = FLT_MAX;
    float heightfieldDistance;
    if (findHeightfieldRayIntersection(transformedOrigin, transformedDirection, boundsDistance, heightfieldDistance)) {
        shortestDistance = heightfieldDistance;
    }
    float rendererDistance;
    if (_renderer && _renderer->findRayIntersection(translation, rotation, scale, origin, direction, boundsDistance,
            rendererDistance)) {
        shortestDistance = qMin(shortestDistance, rendererDistance);
    }
    if (shortestDistance == FLT_MAX) {
        return false;
    }
    distance = shortestDistance;
    return true;
}

const float HERMITE_GRANULARITY = 1.0f / numeric_limits<quint8>::max();

void HeightfieldNode::getRangeAfterHeightPaint(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale,
        const glm::vec3& position, float radius, float height, float& minimum, float& maximum) const {
    if (!_height) {
        return;
    }
    int heightWidth = _height->getWidth();
    int heightHeight = _height->getContents().size() / heightWidth;
    int innerHeightWidth = heightWidth - HeightfieldHeight::HEIGHT_EXTENSION;
    int innerHeightHeight = heightHeight - HeightfieldHeight::HEIGHT_EXTENSION;
    int highestHeightX = heightWidth - 1;
    int highestHeightZ = heightHeight - 1;
    
    glm::vec3 inverseScale(innerHeightWidth / scale.x, numeric_limits<quint16>::max() / scale.y, innerHeightHeight / scale.z);
    glm::vec3 center = glm::inverse(rotation) * (position - translation) * inverseScale + glm::vec3(1.0f, 0.0f, 1.0f);
    glm::vec3 extents = radius * inverseScale;
    
    if (center.x + extents.x < 0.0f || center.z + extents.z < 0.0f ||
            center.x - extents.x > highestHeightX || center.z - extents.z > highestHeightZ) {
        return;
    }
    if (!isLeaf()) {
        for (int i = 0; i < CHILD_COUNT; i++) {
            glm::vec3 nextScale = scale * glm::vec3(0.5f, 1.0f, 0.5f);
            _children[i]->getRangeAfterHeightPaint(translation +
                rotation * glm::vec3(i & X_MAXIMUM_FLAG ? nextScale.x : 0.0f, 0.0f,
                    i & Y_MAXIMUM_FLAG ? nextScale.z : 0.0f), rotation,
                nextScale, position, radius, height, minimum, maximum);
        }
        return;
    }
    glm::vec3 start = glm::clamp(glm::floor(center - extents), glm::vec3(),
        glm::vec3((float)highestHeightX, 0.0f, (float)highestHeightZ));
    glm::vec3 end = glm::clamp(glm::ceil(center + extents), glm::vec3(),
        glm::vec3((float)highestHeightX, 0.0f, (float)highestHeightZ));
    
    const quint16* lineDest = _height->getContents().constData() + (int)start.z * heightWidth + (int)start.x;
    float squaredRadius = extents.x * extents.x;
    float squaredRadiusReciprocal = 1.0f / squaredRadius;
    float multiplierZ = extents.x / extents.z;
    float relativeHeight = height * numeric_limits<quint16>::max() / scale.y;
    for (float z = start.z; z <= end.z; z += 1.0f) {
        const quint16* dest = lineDest;
        for (float x = start.x; x <= end.x; x += 1.0f, dest++) {
            float dx = x - center.x, dz = (z - center.z) * multiplierZ;
            float distanceSquared = dx * dx + dz * dz;
            if (distanceSquared <= squaredRadius) {
                // height falls off towards edges
                int value = *dest;
                if (value != 0) {
                    value += relativeHeight * (squaredRadius - distanceSquared) * squaredRadiusReciprocal;
                    minimum = qMin(minimum, (float)value);
                    maximum = qMax(maximum, (float)value);
                }
            }
        }
        lineDest += heightWidth;
    }
    
    // make sure we increment in multiples of the voxel size
    float voxelStep = scale.x / innerHeightWidth;
    float heightIncrement = (1.0f - minimum) * scale.y / numeric_limits<quint16>::max();
    float incrementSteps = heightIncrement / voxelStep;
    if (glm::abs(incrementSteps - glm::round(incrementSteps)) > HERMITE_GRANULARITY) {
        minimum = 1.0f - voxelStep * glm::ceil(incrementSteps) * numeric_limits<quint16>::max() / scale.y;
    }
}

HeightfieldNode* HeightfieldNode::paintHeight(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale,
        const glm::vec3& position, float radius, float height, bool set, bool erase,
        float normalizeScale, float normalizeOffset, float granularity) {
    if (!_height) {
        return this;
    }
    int heightWidth = _height->getWidth();
    int heightHeight = _height->getContents().size() / heightWidth;
    int innerHeightWidth = heightWidth - HeightfieldHeight::HEIGHT_EXTENSION;
    int innerHeightHeight = heightHeight - HeightfieldHeight::HEIGHT_EXTENSION;
    int highestHeightX = heightWidth - 1;
    int highestHeightZ = heightHeight - 1;
    
    glm::vec3 inverseScale(innerHeightWidth / scale.x, numeric_limits<quint16>::max() / scale.y, innerHeightHeight / scale.z);
    glm::vec3 center = glm::inverse(rotation) * (position - translation) * inverseScale + glm::vec3(1.0f, 0.0f, 1.0f);
    glm::vec3 extents = radius * inverseScale;
    
    bool intersects = (center.x + extents.x >= 0.0f && center.z + extents.z >= 0.0f &&
        center.x - extents.x <= highestHeightX && center.z - extents.z <= highestHeightZ);
    if (!intersects && normalizeScale == 1.0f && normalizeOffset == 0.0f) {
        return this;
    }
    if (!isLeaf()) {
        HeightfieldNode* newNode = this;
        for (int i = 0; i < CHILD_COUNT; i++) {
            glm::vec3 nextScale = scale * glm::vec3(0.5f, 1.0f, 0.5f);
            HeightfieldNode* newChild = _children[i]->paintHeight(translation +
                rotation * glm::vec3(i & X_MAXIMUM_FLAG ? nextScale.x : 0.0f, 0.0f,
                    i & Y_MAXIMUM_FLAG ? nextScale.z : 0.0f), rotation,
                nextScale, position, radius, height, set, erase, normalizeScale, normalizeOffset, granularity);
            if (_children[i] != newChild) {
                if (newNode == this) {
                    newNode = new HeightfieldNode(*this);
                }
                newNode->setChild(i, HeightfieldNodePointer(newChild));
            }
        }
        if (newNode != this) {
            newNode->mergeChildren(true, false);
        }
        return newNode;
    }
    QVector<quint16> newHeightContents = _height->getContents();
    
    int stackWidth = innerHeightWidth + HeightfieldData::SHARED_EDGE;
    QVector<StackArray> newStackContents;
    QVector<SharedObjectPointer> newStackMaterials;
    if (_stack) {
        stackWidth = _stack->getWidth();
        newStackContents = _stack->getContents();
        newStackMaterials = _stack->getMaterials();   
    }
    int innerStackWidth = stackWidth - HeightfieldData::SHARED_EDGE;

    // renormalize if necessary
    maybeRenormalize(scale, normalizeScale, normalizeOffset, innerStackWidth, newHeightContents, newStackContents);
    if (!intersects) {
        return new HeightfieldNode(HeightfieldHeightPointer(new HeightfieldHeight(heightWidth, newHeightContents)),
            _color, _material, HeightfieldStackPointer(newStackContents.isEmpty() ? NULL :
                new HeightfieldStack(stackWidth, newStackContents, newStackMaterials)));
    }
    
    // if the granularity is insufficient, we must subdivide
    if (scale.x / innerHeightWidth > granularity || scale.z / innerHeightHeight > granularity) {
        HeightfieldNodePointer newNode(subdivide(newHeightContents, newStackContents));
        return newNode->paintHeight(translation, rotation, scale, position, radius, height, set,
            erase, 1.0f, 0.0f, granularity);
    }
    
    // now apply the actual change
    glm::vec3 start = glm::clamp(glm::floor(center - extents), glm::vec3(),
        glm::vec3((float)highestHeightX, 0.0f, (float)highestHeightZ));
    glm::vec3 end = glm::clamp(glm::ceil(center + extents), glm::vec3(),
        glm::vec3((float)highestHeightX, 0.0f, (float)highestHeightZ));
    
    quint16* lineDest = newHeightContents.data() + (int)start.z * heightWidth + (int)start.x;
    float squaredRadius = extents.x * extents.x;
    float squaredRadiusReciprocal = 1.0f / squaredRadius;
    float multiplierZ = extents.x / extents.z;
    float relativeHeight = height * numeric_limits<quint16>::max() / scale.y;
    quint16 heightValue = erase ? 0 : relativeHeight;
    for (float z = start.z; z <= end.z; z += 1.0f) {
        quint16* dest = lineDest;
        for (float x = start.x; x <= end.x; x += 1.0f, dest++) {
            float dx = x - center.x, dz = (z - center.z) * multiplierZ;
            float distanceSquared = dx * dx + dz * dz;
            if (distanceSquared <= squaredRadius) {
                if (erase || set) {
                    *dest = heightValue;
                    
                } else {
                    // height falls off towards edges
                    int value = *dest;
                    if (value != 0) {
                        *dest = value + relativeHeight * (squaredRadius - distanceSquared) * squaredRadiusReciprocal;
                    }
                }
            }
        }
        lineDest += heightWidth;
    }
    
    return new HeightfieldNode(HeightfieldHeightPointer(new HeightfieldHeight(heightWidth, newHeightContents)),
        _color, _material, HeightfieldStackPointer(newStackContents.isEmpty() ? NULL :
            new HeightfieldStack(stackWidth, newStackContents, newStackMaterials)));
}

HeightfieldNode* HeightfieldNode::fillHeight(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale,
        const glm::vec3& position, float radius, float granularity) {
    if (!_height) {
        return this;
    }
    int heightWidth = _height->getWidth();
    int heightHeight = _height->getContents().size() / heightWidth;
    int innerHeightWidth = heightWidth - HeightfieldHeight::HEIGHT_EXTENSION;
    int innerHeightHeight = heightHeight - HeightfieldHeight::HEIGHT_EXTENSION;
    int highestHeightX = heightWidth - 1;
    int highestHeightZ = heightHeight - 1;
    
    glm::vec3 inverseScale(innerHeightWidth / scale.x, numeric_limits<quint16>::max() / scale.y, innerHeightHeight / scale.z);
    glm::vec3 center = glm::inverse(rotation) * (position - translation) * inverseScale + glm::vec3(1.0f, 0.0f, 1.0f);
    glm::vec3 extents = radius * inverseScale;
    
    if (center.x + extents.x < 0.0f || center.z + extents.z < 0.0f || center.x - extents.x > highestHeightX ||
            center.z - extents.z > highestHeightZ) {
        return this;
    }
    if (!isLeaf()) {
        HeightfieldNode* newNode = this;
        for (int i = 0; i < CHILD_COUNT; i++) {
            glm::vec3 nextScale = scale * glm::vec3(0.5f, 1.0f, 0.5f);
            HeightfieldNode* newChild = _children[i]->fillHeight(translation +
                rotation * glm::vec3(i & X_MAXIMUM_FLAG ? nextScale.x : 0.0f, 0.0f,
                    i & Y_MAXIMUM_FLAG ? nextScale.z : 0.0f), rotation,
                nextScale, position, radius, granularity);
            if (_children[i] != newChild) {
                if (newNode == this) {
                    newNode = new HeightfieldNode(*this);
                }
                newNode->setChild(i, HeightfieldNodePointer(newChild));
            }
        }
        if (newNode != this) {
            newNode->mergeChildren(true, false);
        }
        return newNode;
    }
    if (!_stack) {
        return this;
    }
    
    // if the granularity is insufficient, we must subdivide
    QVector<quint16> newHeightContents = _height->getContents();
    QVector<StackArray> newStackContents = _stack->getContents();
    if (scale.x / innerHeightWidth > granularity || scale.z / innerHeightHeight > granularity) {
        HeightfieldNodePointer newNode(subdivide(newHeightContents, newStackContents));
        return newNode->fillHeight(translation, rotation, scale, position, radius, granularity);
    }
    
    int stackWidth = _stack->getWidth();
    int stackHeight = newStackContents.size() / stackWidth;
    QVector<SharedObjectPointer> newStackMaterials = _stack->getMaterials();
    
    int colorWidth, colorHeight;
    QByteArray newColorContents;
    if (_color) {
        colorWidth = _color->getWidth();
        colorHeight = _color->getContents().size() / (colorWidth * DataBlock::COLOR_BYTES);
        newColorContents = _color->getContents();
        
    } else {
        colorWidth = innerHeightWidth + HeightfieldData::SHARED_EDGE;
        colorHeight = innerHeightHeight + HeightfieldData::SHARED_EDGE;
        newColorContents = QByteArray(colorWidth * colorHeight * DataBlock::COLOR_BYTES, 0xFF);
    }
    int innerColorWidth = colorWidth - HeightfieldData::SHARED_EDGE;
    int innerColorHeight = colorHeight - HeightfieldData::SHARED_EDGE;
    
    int materialWidth, materialHeight;
    QByteArray newMaterialContents;
    QVector<SharedObjectPointer> newMaterialMaterials;
    if (_material) {
        materialWidth = _material->getWidth();
        materialHeight = _material->getContents().size() / materialWidth;
        newMaterialContents = _material->getContents();
        newMaterialMaterials = _material->getMaterials();
        
    } else {
        materialWidth = colorWidth;
        materialHeight = colorHeight;
        newMaterialContents = QByteArray(materialWidth * materialHeight, 0);
    }
    int innerMaterialWidth = materialWidth - HeightfieldData::SHARED_EDGE;
    int innerMaterialHeight = materialHeight - HeightfieldData::SHARED_EDGE;
    
    glm::vec3 start = glm::clamp(glm::floor(center - extents), glm::vec3(),
        glm::vec3((float)highestHeightX, 0.0f, (float)highestHeightZ));
    glm::vec3 end = glm::clamp(glm::ceil(center + extents), glm::vec3(),
        glm::vec3((float)highestHeightX, 0.0f, (float)highestHeightZ));
    float voxelStep = scale.x / innerHeightWidth;
    float voxelScale = scale.y / (numeric_limits<quint16>::max() * voxelStep);
    
    quint16* lineDest = newHeightContents.data() + (int)start.z * heightWidth + (int)start.x;
    float squaredRadius = extents.x * extents.x;
    float multiplierZ = extents.x / extents.z;
    float colorStepX = (float)innerColorWidth / innerHeightWidth;
    float colorStepZ = (float)innerColorHeight / innerHeightHeight;
    float materialStepX = (float)innerMaterialWidth / innerHeightWidth;
    float materialStepZ = (float)innerMaterialHeight / innerHeightHeight;
    QHash<int, int> materialMap;
    for (float z = start.z; z <= end.z; z += 1.0f) {
        quint16* dest = lineDest;
        for (float x = start.x; x <= end.x; x += 1.0f, dest++) {
            float dx = x - center.x, dz = (z - center.z) * multiplierZ;
            float distanceSquared = dx * dx + dz * dz;
            if (distanceSquared <= squaredRadius && x >= 1.0f && z >= 1.0f && x <= stackWidth && z <= stackHeight) {
                int stackX = (int)x - 1, stackZ = (int)z - 1;
                StackArray* stackDest = newStackContents.data() + stackZ * stackWidth + stackX;
                if (stackDest->isEmpty()) {
                    continue;
                }
                int y = stackDest->getPosition() + stackDest->getEntryCount() - 1;
                for (const StackArray::Entry* entry = stackDest->getEntryData() + stackDest->getEntryCount() - 1;
                        entry >= stackDest->getEntryData(); entry--, y--) {
                    if (!entry->isSet()) {
                        continue;
                    }
                    glm::vec3 normal;
                    int newHeight = qMax((int)((y + entry->getHermiteY(normal)) / voxelScale), 1); 
                    if (newHeight < *dest) {
                        break;
                    }
                    *dest = newHeight;
                    for (int colorZ = stackZ * colorStepZ; (int)(colorZ / colorStepZ) == stackZ; colorZ++) {
                        for (int colorX = stackX * colorStepX; (int)(colorX / colorStepX) == stackX; colorX++) {
                            uchar* colorDest = (uchar*)newColorContents.data() +
                                (colorZ * colorWidth + colorX) * DataBlock::COLOR_BYTES;
                            colorDest[0] = qRed(entry->color);
                            colorDest[1] = qGreen(entry->color);
                            colorDest[2] = qBlue(entry->color);
                        }
                    }
                    for (int materialZ = stackZ * materialStepZ; (int)(materialZ / materialStepZ) == stackZ; materialZ++) {
                        for (int materialX = stackX * materialStepX; (int)(materialX / materialStepX) == stackX;
                                materialX++) {
                            int material = entry->material;
                            if (material != 0) {
                                int& mapping = materialMap[material];
                                if (mapping == 0) {
                                    mapping = getMaterialIndex(newStackMaterials.at(material - 1),
                                        newMaterialMaterials, newMaterialContents);
                                }
                                material = mapping;
                            }
                            newMaterialContents[materialZ * materialWidth + materialX] = material;
                        }
                    }
                    break;
                }
                stackDest->clear();
            }
        }
        lineDest += heightWidth;
    }
    clearUnusedMaterials(newMaterialMaterials, newMaterialContents);
    clearUnusedMaterials(newStackMaterials, newStackContents);    
    
    return new HeightfieldNode(HeightfieldHeightPointer(new HeightfieldHeight(heightWidth, newHeightContents)),
        HeightfieldColorPointer(new HeightfieldColor(colorWidth, newColorContents)), 
        HeightfieldMaterialPointer(new HeightfieldMaterial(materialWidth, newMaterialContents, newMaterialMaterials)),
        HeightfieldStackPointer(new HeightfieldStack(stackWidth, newStackContents, newStackMaterials)));
}

void HeightfieldNode::getRangeAfterEdit(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale,
        const Box& editBounds, float& minimum, float& maximum) const {
    if (!_height) {
        return;
    }
    int heightWidth = _height->getWidth();
    int heightHeight = _height->getContents().size() / heightWidth;
    int innerHeightWidth = heightWidth - HeightfieldHeight::HEIGHT_EXTENSION;
    int innerHeightHeight = heightHeight - HeightfieldHeight::HEIGHT_EXTENSION;
    glm::mat4 baseInverseTransform = glm::mat4_cast(glm::inverse(rotation)) * glm::translate(-translation);
    glm::vec3 inverseScale(innerHeightWidth / scale.x, numeric_limits<quint16>::max() / scale.y, innerHeightHeight / scale.z);
    glm::mat4 inverseTransform = glm::translate(glm::vec3(1.0f, 0.0f, 1.0f)) * glm::scale(inverseScale) * baseInverseTransform;
    Box transformedBounds = inverseTransform * editBounds;
    if (transformedBounds.maximum.x < 0.0f || transformedBounds.maximum.z < 0.0f ||
            transformedBounds.minimum.x > heightWidth - 1 || transformedBounds.minimum.z > heightHeight - 1) {
        return;
    }
    if (!isLeaf()) {
        for (int i = 0; i < CHILD_COUNT; i++) {
            glm::vec3 nextScale = scale * glm::vec3(0.5f, 1.0f, 0.5f);
            _children[i]->getRangeAfterEdit(translation +
                rotation * glm::vec3(i & X_MAXIMUM_FLAG ? nextScale.x : 0.0f, 0.0f,
                    i & Y_MAXIMUM_FLAG ? nextScale.z : 0.0f), rotation,
                nextScale, editBounds, minimum, maximum);
        }
        return;
    }
    glm::vec3 start = glm::floor(transformedBounds.minimum);
    glm::vec3 end = glm::ceil(transformedBounds.maximum);
    
    minimum = qMin(minimum, start.y);
    maximum = qMax(maximum, end.y);
    
    // make sure we increment in multiples of the voxel size
    float voxelStep = scale.x / innerHeightWidth;
    float heightIncrement = (1.0f - minimum) * scale.y / numeric_limits<quint16>::max();
    float incrementSteps = heightIncrement / voxelStep;
    if (glm::abs(incrementSteps - glm::round(incrementSteps)) > HERMITE_GRANULARITY) {
        minimum = 1.0f - voxelStep * glm::ceil(incrementSteps) * numeric_limits<quint16>::max() / scale.y;
    }
}

HeightfieldNode* HeightfieldNode::setMaterial(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale,
        Spanner* spanner, const SharedObjectPointer& material, const QColor& color, bool paint, bool voxelize,
        float normalizeScale, float normalizeOffset, float granularity) {
    if (!_height) {
        return this;
    }
    int heightWidth = _height->getWidth();
    int heightHeight = _height->getContents().size() / heightWidth;
    int innerHeightWidth = heightWidth - HeightfieldHeight::HEIGHT_EXTENSION;
    int innerHeightHeight = heightHeight - HeightfieldHeight::HEIGHT_EXTENSION;
    glm::vec3 expansion(1.0f / innerHeightWidth, 0.0f, 1.0f / innerHeightHeight);
    Box bounds = glm::translate(translation) * glm::mat4_cast(rotation) * Box(-expansion, scale + expansion);
    bool intersects = bounds.intersects(spanner->getBounds());
    if (!intersects && normalizeScale == 1.0f && normalizeOffset == 0.0f) {
        return this;
    }
    if (!isLeaf()) {
        HeightfieldNode* newNode = this;
        for (int i = 0; i < CHILD_COUNT; i++) {
            glm::vec3 nextScale = scale * glm::vec3(0.5f, 1.0f, 0.5f);
            HeightfieldNode* newChild = _children[i]->setMaterial(translation +
                rotation * glm::vec3(i & X_MAXIMUM_FLAG ? nextScale.x : 0.0f, 0.0f,
                    i & Y_MAXIMUM_FLAG ? nextScale.z : 0.0f), rotation, nextScale, spanner,
                material, color, paint, voxelize, normalizeScale, normalizeOffset, granularity);
            if (_children[i] != newChild) {
                if (newNode == this) {
                    newNode = new HeightfieldNode(*this);
                }
                newNode->setChild(i, HeightfieldNodePointer(newChild));
            }
        }
        if (newNode != this) {
            newNode->mergeChildren();
        }
        return newNode;
    }
    int highestHeightX = heightWidth - 1;
    int highestHeightZ = heightHeight - 1;
    QVector<quint16> newHeightContents = _height->getContents();
    
    int stackWidth, stackHeight;
    QVector<StackArray> newStackContents;
    QVector<SharedObjectPointer> newStackMaterials;
    if (_stack) {
        stackWidth = _stack->getWidth();
        stackHeight = _stack->getContents().size() / stackWidth;
        newStackContents = _stack->getContents();
        newStackMaterials = _stack->getMaterials();
        
    } else {
        stackWidth = innerHeightWidth + HeightfieldData::SHARED_EDGE;
        stackHeight = innerHeightHeight + HeightfieldData::SHARED_EDGE;
        newStackContents = QVector<StackArray>(stackWidth * stackHeight);
    }
    int innerStackWidth = stackWidth - HeightfieldData::SHARED_EDGE;
    int innerStackHeight = stackHeight - HeightfieldData::SHARED_EDGE;
    
    // renormalize if necessary
    maybeRenormalize(scale, normalizeScale, normalizeOffset, innerStackWidth, newHeightContents, newStackContents);
    if (!intersects) {
        return new HeightfieldNode(HeightfieldHeightPointer(new HeightfieldHeight(heightWidth, newHeightContents)),
            _color, _material, HeightfieldStackPointer(new HeightfieldStack(stackWidth, newStackContents, newStackMaterials)));
    }
    
    // if the granularity is insufficient, we must subdivide
    if (scale.x / innerHeightWidth > granularity || scale.z / innerHeightHeight > granularity) {
        HeightfieldNodePointer newNode(subdivide(newHeightContents, newStackContents));
        return newNode->setMaterial(translation, rotation, scale, spanner, material, color,
            paint, voxelize, 1.0f, 0.0f, granularity);
    }
    
    QVector<quint16> oldHeightContents = newHeightContents;
    QVector<StackArray> oldStackContents = newStackContents;
    
    int colorWidth, colorHeight;
    QByteArray newColorContents;
    if (_color) {
        colorWidth = _color->getWidth();
        colorHeight = _color->getContents().size() / (colorWidth * DataBlock::COLOR_BYTES);
        newColorContents = _color->getContents();
        
    } else {
        colorWidth = innerHeightWidth + HeightfieldData::SHARED_EDGE;
        colorHeight = innerHeightHeight + HeightfieldData::SHARED_EDGE;
        newColorContents = QByteArray(colorWidth * colorHeight * DataBlock::COLOR_BYTES, 0xFF);
    }
    int innerColorWidth = colorWidth - HeightfieldData::SHARED_EDGE;
    int innerColorHeight = colorHeight - HeightfieldData::SHARED_EDGE;
    
    int materialWidth, materialHeight;
    QByteArray newMaterialContents;
    QVector<SharedObjectPointer> newMaterialMaterials;
    if (_material) {
        materialWidth = _material->getWidth();
        materialHeight = _material->getContents().size() / materialWidth;
        newMaterialContents = _material->getContents();
        newMaterialMaterials = _material->getMaterials();
        
    } else {
        materialWidth = colorWidth;
        materialHeight = colorHeight;
        newMaterialContents = QByteArray(materialWidth * materialHeight, 0);
    }
    int innerMaterialWidth = materialWidth - HeightfieldData::SHARED_EDGE;
    int innerMaterialHeight = materialHeight - HeightfieldData::SHARED_EDGE;
    
    glm::mat4 baseInverseTransform = glm::mat4_cast(glm::inverse(rotation)) * glm::translate(-translation);
    glm::vec3 inverseScale(innerHeightWidth / scale.x, numeric_limits<quint16>::max() / scale.y, innerHeightHeight / scale.z);
    glm::mat4 inverseTransform = glm::translate(glm::vec3(1.0f, 0.0f, 1.0f)) * glm::scale(inverseScale) * baseInverseTransform;
    Box transformedBounds = inverseTransform * spanner->getBounds();
    glm::mat4 transform = glm::inverse(inverseTransform);
    
    glm::vec3 start = glm::ceil(transformedBounds.maximum);
    glm::vec3 end = glm::floor(transformedBounds.minimum);
    
    float stepX = 1.0f, stepZ = 1.0f;
    if (paint) {
        stepX = (float)innerHeightWidth / qMax(innerHeightWidth, qMax(innerColorWidth, innerMaterialWidth));
        stepZ = (float)innerHeightHeight / qMax(innerHeightHeight, qMax(innerColorHeight, innerMaterialHeight));
    
    } else {
        start.x += 1.0f;
        start.z += 1.0f;
        end.x -= 1.0f;
        end.z -= 1.0f;
    }
    
    float startX = glm::clamp(start.x, 0.0f, (float)highestHeightX), endX = glm::clamp(end.x, 0.0f, (float)highestHeightX);
    float startZ = glm::clamp(start.z, 0.0f, (float)highestHeightZ), endZ = glm::clamp(end.z, 0.0f, (float)highestHeightZ);
    float voxelStep = scale.x / innerHeightWidth;
    float voxelScale = scale.y / (numeric_limits<quint16>::max() * voxelStep);
    int newTop = start.y * voxelScale;
    int newBottom = end.y * voxelScale;
    glm::vec3 worldStart = glm::vec3(transform * glm::vec4(startX, paint ? 0.0f : newTop / voxelScale, startZ, 1.0f));
    glm::vec3 worldStepX = glm::vec3(transform * glm::vec4(stepX, 0.0f, 0.0f, 0.0f));
    glm::vec3 worldStepY = glm::vec3(transform * glm::vec4(0.0f, 1.0f / voxelScale, 0.0f, 0.0f));
    glm::vec3 worldStepZ = glm::vec3(transform * glm::vec4(0.0f, 0.0f, stepZ, 0.0f));
    QRgb rgba = color.rgba();
    bool erase = (color.alpha() == 0);
    uchar materialMaterialIndex = getMaterialIndex(material, newMaterialMaterials, newMaterialContents);
    uchar stackMaterialIndex = getMaterialIndex(material, newStackMaterials, newStackContents);
    bool hasOwnColors = spanner->hasOwnColors();
    bool hasOwnMaterials = spanner->hasOwnMaterials();
    QHash<int, int> materialMappings;
    for (float z = startZ; z >= endZ; z -= stepZ, worldStart -= worldStepZ) {
        quint16* heightDest = newHeightContents.data() + (int)z * heightWidth;
        glm::vec3 worldPos = worldStart;
        for (float x = startX; x >= endX; x -= stepX, worldPos -= worldStepX) {
            quint16* heightLineDest = heightDest + (int)x;
            float distance;
            glm::vec3 normal;
            
            float colorX = (x - HeightfieldHeight::HEIGHT_BORDER) * innerColorWidth / innerHeightWidth;
            float colorZ = (z - HeightfieldHeight::HEIGHT_BORDER) * innerColorHeight / innerHeightHeight;
            uchar* colorDest = (colorX >= 0.0f && colorX <= innerColorWidth && colorZ >= 0.0f && colorZ <= innerColorHeight) ?
                ((uchar*)newColorContents.data() + ((int)colorZ * colorWidth + (int)colorX) * DataBlock::COLOR_BYTES) : NULL;
                            
            float materialX = (x - HeightfieldHeight::HEIGHT_BORDER) * innerMaterialWidth / innerHeightWidth;
            float materialZ = (z - HeightfieldHeight::HEIGHT_BORDER) * innerMaterialHeight / innerHeightHeight;
            char* materialDest = (materialX >= 0.0f && materialX <= innerMaterialWidth && materialZ >= 0.0f &&
                materialZ <= innerMaterialHeight) ? (newMaterialContents.data() + 
                    (int)materialZ * materialWidth + (int)materialX) : NULL;
            
            if (paint && *heightLineDest != 0 && spanner->contains(worldPos + worldStepY * (*heightLineDest * voxelScale))) {
                if (colorDest) {
                    colorDest[0] = qRed(rgba);
                    colorDest[1] = qGreen(rgba);
                    colorDest[2] = qBlue(rgba);
                }
                if (materialDest) {
                    *materialDest = materialMaterialIndex;
                }
            }
            
            float stackX = (x - HeightfieldHeight::HEIGHT_BORDER) * innerStackWidth / innerHeightWidth;
            float stackZ = (z - HeightfieldHeight::HEIGHT_BORDER) * innerStackHeight / innerHeightHeight;
            
            if (stackX >= 0.0f && stackX <= innerStackWidth && stackZ >= 0.0f && stackZ <= innerStackHeight) {
                StackArray* stackDest = newStackContents.data() + (int)stackZ * stackWidth + (int)stackX;
                if (paint) {
                    if (stackDest->isEmpty() || glm::fract(x) != 0.0f || glm::fract(z) != 0.0f) {
                        continue;
                    }
                    glm::vec3 pos = worldPos + worldStepY * (float)stackDest->getPosition();
                    for (StackArray::Entry* entryDest = stackDest->getEntryData(), *end = entryDest +
                            stackDest->getEntryCount(); entryDest != end; entryDest++, pos += worldStepY) {
                        if (entryDest->isSet() && spanner->contains(pos)) {
                            entryDest->color = rgba;
                            entryDest->material = stackMaterialIndex;
                        }
                    }
                    continue;
                }
                int prepend = 0, append = 0;
                if (!stackDest->isEmpty()) {
                    int oldBottom = stackDest->getPosition();
                    int oldTop = oldBottom + stackDest->getEntryCount() - 1;
                    prepend = qMax(0, oldBottom - newBottom);
                    append = qMax(0, newTop - oldTop);
                    if (prepend != 0 || append != 0) {
                        StackArray newStack(prepend + stackDest->getEntryCount() + append);
                        memcpy(newStack.getEntryData() + prepend, stackDest->getEntryData(),
                            stackDest->getEntryCount() * sizeof(StackArray::Entry));
                        for (StackArray::Entry* entryDest = newStack.getEntryData(), *end = entryDest + prepend;
                                entryDest != end; entryDest++) {
                            entryDest->color = end->color;
                            entryDest->material = end->material;
                        }
                        *stackDest = newStack;
                        stackDest->setPosition(qMin(oldBottom, newBottom));
                    }
                } else {
                    *stackDest = StackArray(newTop - newBottom + 1);
                    stackDest->setPosition(newBottom);
                    prepend = stackDest->getEntryCount();
                }
                const quint16* oldHeightLineDest = oldHeightContents.constData() + (int)z * heightWidth + (int)x;
                if (*heightLineDest != 0) {
                    float voxelHeight = *heightLineDest * voxelScale;
                    float left = oldHeightLineDest[-1] * voxelScale;
                    float right = oldHeightLineDest[1] * voxelScale;
                    float down = oldHeightLineDest[-heightWidth] * voxelScale;
                    float up = oldHeightLineDest[heightWidth] * voxelScale;
                    float deltaX = (left == 0.0f || right == 0.0f) ? 0.0f : (left - right);
                    float deltaZ = (up == 0.0f || down == 0.0f) ? 0.0f : (down - up);
                    for (int i = 0, total = prepend + append; i < total; i++) {
                        int offset = (i < prepend) ? i : stackDest->getEntryCount() - append + (i - prepend);
                        int y = stackDest->getPosition() + offset;
                        StackArray::Entry* entryDest = stackDest->getEntryData() + offset;
                        if (y > voxelHeight) {
                            if (y <= right) {
                                entryDest->setHermiteX(glm::normalize(glm::vec3(voxelHeight - right, 1.0f, deltaZ * 0.5f)),
                                    (right == voxelHeight) ? 0.5f : (y - voxelHeight) / (right - voxelHeight));   
                            }
                            if (y <= up) {
                                entryDest->setHermiteZ(glm::normalize(glm::vec3(deltaX * 0.5f, 1.0f, voxelHeight - up)),
                                    (up == voxelHeight) ? 0.5f : (y - voxelHeight) / (up - voxelHeight));   
                            }
                        } else {
                            if (right != 0.0f && y > right) {
                                entryDest->setHermiteX(glm::normalize(glm::vec3(voxelHeight - right, 1.0f, deltaZ * 0.5f)),
                                    (right == voxelHeight) ? 0.5f : (y - voxelHeight) / (right - voxelHeight));   
                            }
                            if (up != 0.0f && y > up) {
                                entryDest->setHermiteZ(glm::normalize(glm::vec3(deltaX * 0.5f, 1.0f, voxelHeight - up)),
                                    (up == voxelHeight) ? 0.5f : (y - voxelHeight) / (up - voxelHeight));   
                            }
                            if (colorDest) {
                                entryDest->color = qRgb(colorDest[0], colorDest[1], colorDest[2]);
                            }
                            if (materialDest) {
                                int index = *materialDest;
                                if (index != 0) {
                                    int& mapping = materialMappings[index];
                                    if (mapping == 0) {
                                        mapping = getMaterialIndex(newMaterialMaterials.at(index - 1),
                                            newStackMaterials, newStackContents);
                                    }
                                    index = mapping;
                                }
                                entryDest->material = index;
                            }
                            if (y + 1 > voxelHeight) {
                                *heightLineDest = 0;
                                entryDest->setHermiteY(glm::normalize(glm::vec3(deltaX, 2.0f, deltaZ)), voxelHeight - y);
                            }
                        }
                    }
                }
                StackArray::Entry* entryDest = stackDest->getEntryData() + (newTop - stackDest->getPosition());
                glm::vec3 pos = worldPos;
                float voxelHeight = *heightLineDest * voxelScale;
                float nextVoxelHeightX = heightLineDest[1] * voxelScale;
                float nextVoxelHeightZ = heightLineDest[heightWidth] * voxelScale;
                float oldVoxelHeight = *oldHeightLineDest * voxelScale;
                float oldNextVoxelHeightX = oldHeightLineDest[1] * voxelScale;
                float oldNextVoxelHeightZ = oldHeightLineDest[heightWidth] * voxelScale;
                // skip the actual set if voxelizing
                for (int y = voxelize ? newBottom - 1 : newTop; y >= newBottom; y--, entryDest--, pos -= worldStepY) {
                    int oldCurrentAlpha = stackDest->getEntryAlpha(y, oldVoxelHeight);
                    if (spanner->contains(pos)) {
                        if (hasOwnColors && !erase) {
                            entryDest->color = spanner->getColorAt(pos);
                            
                        } else {
                            entryDest->color = rgba;
                        }
                        if (hasOwnMaterials && !erase) {
                            int index = spanner->getMaterialAt(pos);
                            if (index != 0) {
                                int& mapping = materialMappings[index];
                                if (mapping == 0) {
                                    mapping = getMaterialIndex(spanner->getMaterials().at(index - 1),
                                        newStackMaterials, newStackContents);
                                }
                                index = mapping;
                            }
                            entryDest->material = index;
                            
                        } else {
                            entryDest->material = stackMaterialIndex;
                        }
                    }
                    
                    int currentAlpha = stackDest->getEntryAlpha(y, voxelHeight);
                    bool flipped = (color.alpha() == currentAlpha);
                    int nextStackX = (int)stackX + 1;
                    if (nextStackX <= innerStackWidth) {
                        int nextAlphaX = newStackContents.at((int)stackZ * stackWidth + nextStackX).getEntryAlpha(
                            y, nextVoxelHeightX);
                        if (nextAlphaX == currentAlpha) {
                            entryDest->hermiteX = 0;
                            
                        } else {
                            float oldDistance = flipped ? 0.0f : 1.0f;
                            if (currentAlpha == oldCurrentAlpha && nextAlphaX == oldStackContents.at((int)stackZ * stackWidth +
                                    nextStackX).getEntryAlpha(y, oldNextVoxelHeightX) && entryDest->hermiteX != 0) {
                                oldDistance = qAlpha(entryDest->hermiteX) / (float)numeric_limits<quint8>::max();
                            }
                            if (flipped ? (spanner->intersects(pos + worldStepX, pos, distance, normal) &&
                                    (distance = 1.0f - distance) >= oldDistance) :
                                (spanner->intersects(pos, pos + worldStepX, distance, normal) &&
                                    distance <= oldDistance)) {
                                entryDest->setHermiteX(erase ? -normal : normal, distance);
                            }
                        }
                    }
                    bool nextAlphaY = stackDest->getEntryAlpha(y + 1, voxelHeight);
                    if (nextAlphaY == currentAlpha) {
                        entryDest->hermiteY = 0;
                    
                    } else {
                        float oldDistance = flipped ? 0.0f : 1.0f;
                        if (currentAlpha == oldCurrentAlpha && nextAlphaY == oldStackContents.at((int)stackZ * stackWidth +
                                (int)stackX).getEntryAlpha(y + 1, oldVoxelHeight) && entryDest->hermiteY != 0) {
                            oldDistance = qAlpha(entryDest->hermiteY) / (float)numeric_limits<quint8>::max();
                        }
                        if (flipped ? (spanner->intersects(pos + worldStepY, pos, distance, normal) &&
                                (distance = 1.0f - distance) >= oldDistance) :
                            (spanner->intersects(pos, pos + worldStepY, distance, normal) &&
                                distance <= oldDistance)) {
                            entryDest->setHermiteY(erase ? -normal : normal, distance);
                        }
                    }
                    int nextStackZ = (int)stackZ + 1;
                    if (nextStackZ <= innerStackHeight) {
                        bool nextAlphaZ = newStackContents.at(nextStackZ * stackWidth + (int)stackX).getEntryAlpha(
                            y, nextVoxelHeightZ);
                        if (nextAlphaZ == currentAlpha) {
                            entryDest->hermiteZ = 0;
                            
                        } else {
                            float oldDistance = flipped ? 0.0f : 1.0f;
                            if (currentAlpha == oldCurrentAlpha && nextAlphaZ == oldStackContents.at(nextStackZ * stackWidth +
                                    (int)stackX).getEntryAlpha(y, oldNextVoxelHeightZ) && entryDest->hermiteZ != 0) {
                                oldDistance = qAlpha(entryDest->hermiteZ) / (float)numeric_limits<quint8>::max();
                            }
                            if (flipped ? (spanner->intersects(pos + worldStepZ, pos, distance, normal) &&
                                    (distance = 1.0f - distance) >= oldDistance) :
                                (spanner->intersects(pos, pos + worldStepZ, distance, normal) &&
                                    distance <= oldDistance)) {
                                entryDest->setHermiteZ(erase ? -normal : normal, distance);
                            }
                        }
                    }
                }
                
                // prune zero entries from end, repeated entries from beginning
                int endPruneCount = 0;
                for (int i = stackDest->getEntryCount() - 1; i >= 0 && stackDest->getEntryData()[i].isZero(); i--) {
                    endPruneCount++;
                }
                if (endPruneCount == stackDest->getEntryCount()) {
                    stackDest->clear();
                    
                } else {
                    stackDest->removeEntries(stackDest->getEntryCount() - endPruneCount, endPruneCount);
                    int beginningPruneCount = 0;
                    for (int i = 0; i < stackDest->getEntryCount() - 1 && stackDest->getEntryData()[i].isMergeable(
                            stackDest->getEntryData()[i + 1]); i++) {
                        beginningPruneCount++;
                    }
                    stackDest->removeEntries(0, beginningPruneCount);
                    stackDest->getPositionRef() += beginningPruneCount;
                }
            }
        }
    }
    clearUnusedMaterials(newMaterialMaterials, newMaterialContents);
    clearUnusedMaterials(newStackMaterials, newStackContents);    
    
    return new HeightfieldNode(paint ? _height : HeightfieldHeightPointer(
            new HeightfieldHeight(heightWidth, newHeightContents)),
        HeightfieldColorPointer(new HeightfieldColor(colorWidth, newColorContents)),
        HeightfieldMaterialPointer(new HeightfieldMaterial(materialWidth, newMaterialContents, newMaterialMaterials)),
        HeightfieldStackPointer(new HeightfieldStack(stackWidth, newStackContents, newStackMaterials)));
}

void HeightfieldNode::read(HeightfieldStreamState& state) {
    clearChildren();
    
    if (!state.shouldSubdivide()) {
        state.base.stream >> _height >> _color >> _material >> _stack;
        return;
    }
    bool leaf;
    state.base.stream >> leaf;
    if (leaf) {
        state.base.stream >> _height >> _color >> _material >> _stack;
        
    } else {
        HeightfieldStreamState nextState = { state.base, glm::vec2(), state.size * 0.5f };
        for (int i = 0; i < CHILD_COUNT; i++) {
            nextState.setMinimum(state.minimum, i);
            _children[i] = new HeightfieldNode();
            _children[i]->read(nextState);
        }
        mergeChildren();
    }
}

void HeightfieldNode::write(HeightfieldStreamState& state) const {
    if (!state.shouldSubdivide()) {
        state.base.stream << _height << _color << _material << _stack;
        return;
    }
    bool leaf = isLeaf();
    state.base.stream << leaf;
    if (leaf) {
        state.base.stream << _height << _color << _material << _stack;
        
    } else {
        HeightfieldStreamState nextState = { state.base, glm::vec2(), state.size * 0.5f };
        for (int i = 0; i < CHILD_COUNT; i++) {
            nextState.setMinimum(state.minimum, i);
            _children[i]->write(nextState);
        }
    }
}

void HeightfieldNode::readDelta(const HeightfieldNodePointer& reference, HeightfieldStreamState& state) {
    clearChildren();

    if (!state.shouldSubdivide()) {
        state.base.stream.readDelta(_height, reference->getHeight());
        state.base.stream.readDelta(_color, reference->getColor());
        state.base.stream.readDelta(_material, reference->getMaterial());
        state.base.stream.readDelta(_stack, reference->getStack());
        return;
    }
    bool leaf;
    state.base.stream >> leaf;
    if (leaf) {
        state.base.stream.readDelta(_height, reference->getHeight());
        state.base.stream.readDelta(_color, reference->getColor());
        state.base.stream.readDelta(_material, reference->getMaterial());
        state.base.stream.readDelta(_stack, reference->getStack());
        
    } else {
        HeightfieldStreamState nextState = { state.base, glm::vec2(), state.size * 0.5f };
        if (reference->isLeaf() || !state.shouldSubdivideReference()) {
            for (int i = 0; i < CHILD_COUNT; i++) {
                nextState.setMinimum(state.minimum, i);
                _children[i] = new HeightfieldNode();
                _children[i]->read(nextState);
            }
        } else {
            for (int i = 0; i < CHILD_COUNT; i++) {
                nextState.setMinimum(state.minimum, i);
                bool changed;
                state.base.stream >> changed;
                if (changed) {    
                    _children[i] = new HeightfieldNode();
                    _children[i]->readDelta(reference->getChild(i), nextState);
                } else {
                    if (nextState.becameSubdividedOrCollapsed()) {
                        _children[i] = reference->getChild(i)->readSubdivision(nextState);
                        
                    } else {
                        _children[i] = reference->getChild(i);    
                    }
                }
            }
        }
        mergeChildren();
    }
}

void HeightfieldNode::writeDelta(const HeightfieldNodePointer& reference, HeightfieldStreamState& state) const {
    if (!state.shouldSubdivide()) {
        state.base.stream.writeDelta(_height, reference->getHeight());
        state.base.stream.writeDelta(_color, reference->getColor());
        state.base.stream.writeDelta(_material, reference->getMaterial());
        state.base.stream.writeDelta(_stack, reference->getStack());
        return;    
    }
    bool leaf = isLeaf();
    state.base.stream << leaf;
    if (leaf) {
        state.base.stream.writeDelta(_height, reference->getHeight());
        state.base.stream.writeDelta(_color, reference->getColor());
        state.base.stream.writeDelta(_material, reference->getMaterial());
        state.base.stream.writeDelta(_stack, reference->getStack());
        
    } else {
        HeightfieldStreamState nextState = { state.base, glm::vec2(), state.size * 0.5f };
        if (reference->isLeaf() || !state.shouldSubdivideReference()) {
            for (int i = 0; i < CHILD_COUNT; i++) {
                nextState.setMinimum(state.minimum, i);
                _children[i]->write(nextState);
            }
        } else {
            for (int i = 0; i < CHILD_COUNT; i++) {
                nextState.setMinimum(state.minimum, i);
                if (_children[i] == reference->getChild(i)) {
                    state.base.stream << false;
                    if (nextState.becameSubdivided()) {
                        _children[i]->writeSubdivision(nextState);
                    }
                } else {                    
                    state.base.stream << true;
                    _children[i]->writeDelta(reference->getChild(i), nextState);
                }
            }
        }
    }
}

HeightfieldNode* HeightfieldNode::readSubdivision(HeightfieldStreamState& state) {
    if (state.shouldSubdivide()) {
        if (!state.shouldSubdivideReference()) {
            bool leaf;
            state.base.stream >> leaf;
            if (leaf) {
                return isLeaf() ? this : new HeightfieldNode(_height, _color, _material, _stack);
                
            } else {
                HeightfieldNode* newNode = new HeightfieldNode(_height, _color, _material, _stack);
                HeightfieldStreamState nextState = { state.base, glm::vec2(), state.size * 0.5f };
                for (int i = 0; i < CHILD_COUNT; i++) {
                    nextState.setMinimum(state.minimum, i);
                    newNode->_children[i] = new HeightfieldNode();
                    newNode->_children[i]->readSubdivided(nextState, state, this);
                }
                return newNode;
            }
        } else if (!isLeaf()) {
            HeightfieldNode* node = this;
            HeightfieldStreamState nextState = { state.base, glm::vec2(), state.size * 0.5f };
            for (int i = 0; i < CHILD_COUNT; i++) {
                nextState.setMinimum(state.minimum, i);
                if (nextState.becameSubdividedOrCollapsed()) {
                    HeightfieldNode* child = _children[i]->readSubdivision(nextState);
                    if (_children[i] != child) {
                        if (node == this) {
                            node = new HeightfieldNode(*this);
                        }
                        node->_children[i] = child;
                    }
                }
            }
            if (node != this) {
                node->mergeChildren();
            }
            return node;
        }
    } else if (!isLeaf()) {
        return new HeightfieldNode(_height, _color, _material, _stack);
    }
    return this;
}

void HeightfieldNode::writeSubdivision(HeightfieldStreamState& state) const {
    if (!state.shouldSubdivide()) {
        return;
    }
    bool leaf = isLeaf();
    if (!state.shouldSubdivideReference()) {
        state.base.stream << leaf;
        if (!leaf) {
            HeightfieldStreamState nextState = { state.base, glm::vec2(), state.size * 0.5f };
            for (int i = 0; i < CHILD_COUNT; i++) {
                nextState.setMinimum(state.minimum, i);
                _children[i]->writeSubdivided(nextState, state, this);
            }
        }
    } else if (!leaf) {
        HeightfieldStreamState nextState = { state.base, glm::vec2(), state.size * 0.5f };
        for (int i = 0; i < CHILD_COUNT; i++) {
            nextState.setMinimum(state.minimum, i);
            if (nextState.becameSubdivided()) {
                _children[i]->writeSubdivision(nextState);
            }
        }
    }
}

void HeightfieldNode::readSubdivided(HeightfieldStreamState& state, const HeightfieldStreamState& ancestorState,
        const HeightfieldNode* ancestor) {
    clearChildren();
    
    if (!state.shouldSubdivide()) {
        // TODO: subdivision encoding
        state.base.stream >> _height >> _color >> _material >> _stack;
        return;
    }
    bool leaf;
    state.base.stream >> leaf;
    if (leaf) {
        state.base.stream >> _height >> _color >> _material >> _stack;
    
    } else {
        HeightfieldStreamState nextState = { state.base, glm::vec2(), state.size * 0.5f };
        for (int i = 0; i < CHILD_COUNT; i++) {
            nextState.setMinimum(state.minimum, i);
            _children[i] = new HeightfieldNode();
            _children[i]->readSubdivided(nextState, ancestorState, ancestor);
        }
        mergeChildren();
    }
}

void HeightfieldNode::writeSubdivided(HeightfieldStreamState& state, const HeightfieldStreamState& ancestorState,
        const HeightfieldNode* ancestor) const {
    if (!state.shouldSubdivide()) {
        // TODO: subdivision encoding
        state.base.stream << _height << _color << _material << _stack;
        return;
    }
    bool leaf = isLeaf();
    state.base.stream << leaf;
    if (leaf) {
        state.base.stream << _height << _color << _material << _stack;
        
    } else {
        HeightfieldStreamState nextState = { state.base, glm::vec2(), state.size * 0.5f };
        for (int i = 0; i < CHILD_COUNT; i++) {
            nextState.setMinimum(state.minimum, i);
            _children[i]->writeSubdivided(nextState, ancestorState, ancestor);
        }
    }
}

void HeightfieldNode::clearChildren() {
    for (int i = 0; i < CHILD_COUNT; i++) {
        _children[i].reset();
    }
}

void HeightfieldNode::mergeChildren(bool height, bool colorMaterial) {
    if (isLeaf()) {
        return;
    }
    int heightWidth = 0;
    int heightHeight = 0;
    int colorWidth = 0;
    int colorHeight = 0;
    int materialWidth = 0;
    int materialHeight = 0;
    int stackWidth = 0;
    int stackHeight = 0;
    for (int i = 0; i < CHILD_COUNT; i++) {
        HeightfieldHeightPointer childHeight = _children[i]->getHeight();
        if (childHeight) {
            int childHeightWidth = childHeight->getWidth();
            int childHeightHeight = childHeight->getContents().size() / childHeightWidth;
            heightWidth = qMax(heightWidth, childHeightWidth);
            heightHeight = qMax(heightHeight, childHeightHeight);
        }
        HeightfieldColorPointer childColor = _children[i]->getColor();
        if (childColor) {
            int childColorWidth = childColor->getWidth();
            int childColorHeight = childColor->getContents().size() / (childColorWidth * DataBlock::COLOR_BYTES);
            colorWidth = qMax(colorWidth, childColorWidth);
            colorHeight = qMax(colorHeight, childColorHeight);
        }
        HeightfieldMaterialPointer childMaterial = _children[i]->getMaterial();
        if (childMaterial) {
            int childMaterialWidth = childMaterial->getWidth();
            int childMaterialHeight = childMaterial->getContents().size() / childMaterialWidth;
            materialWidth = qMax(materialWidth, childMaterialWidth);
            materialHeight = qMax(materialHeight, childMaterialHeight);
        }
        HeightfieldStackPointer childStack = _children[i]->getStack();
        if (childStack) {
            int childStackWidth = childStack->getWidth();
            int childStackHeight = childStack->getContents().size() / childStackWidth;
            stackWidth = qMax(stackWidth, childStackWidth);
            stackHeight = qMax(stackHeight, childStackHeight);
        }
    }
    if (heightWidth > 0 && height) {
        QVector<quint16> heightContents(heightWidth * heightHeight);
        for (int i = 0; i < CHILD_COUNT; i++) {
            HeightfieldHeightPointer childHeight = _children[i]->getHeight();
            if (!childHeight) {
                continue;
            }
            int childHeightWidth = childHeight->getWidth();
            int childHeightHeight = childHeight->getContents().size() / childHeightWidth;
            if (childHeightWidth != heightWidth || childHeightHeight != heightHeight) {
                qWarning() << "Height dimension mismatch [heightWidth=" << heightWidth << ", heightHeight=" << heightHeight <<
                    ", childHeightWidth=" << childHeightWidth << ", childHeightHeight=" << childHeightHeight << "]";
                continue;
            }
            int innerHeightWidth = heightWidth - HeightfieldHeight::HEIGHT_EXTENSION;
            int innerHeightHeight = heightHeight - HeightfieldHeight::HEIGHT_EXTENSION;
            int innerQuadrantHeightWidth = innerHeightWidth / 2;
            int innerQuadrantHeightHeight = innerHeightHeight / 2;
            int quadrantHeightWidth = innerQuadrantHeightWidth + HeightfieldHeight::HEIGHT_EXTENSION - 1;
            int quadrantHeightHeight = innerQuadrantHeightHeight + HeightfieldHeight::HEIGHT_EXTENSION - 1;
            quint16* dest = heightContents.data() + (i & Y_MAXIMUM_FLAG ? (innerQuadrantHeightHeight + 1) * heightWidth : 0) +
                (i & X_MAXIMUM_FLAG ? innerQuadrantHeightWidth + 1 : 0);
            const quint16* src = childHeight->getContents().constData();
            for (int z = 0; z < quadrantHeightHeight; z++, dest += heightWidth, src += heightWidth * 2) {
                const quint16* lineSrc = src;
                for (quint16* lineDest = dest, *end = dest + quadrantHeightWidth; lineDest != end; lineDest++, lineSrc += 2) {
                    *lineDest = *lineSrc;
                }
            }
        }
        _height = new HeightfieldHeight(heightWidth, heightContents);
        
    } else if (height) {
        _height.reset();
    }
    if (colorWidth > 0 && colorMaterial) {
        QByteArray colorContents(colorWidth * colorHeight * DataBlock::COLOR_BYTES, 0xFF);
        for (int i = 0; i < CHILD_COUNT; i++) {
            HeightfieldColorPointer childColor = _children[i]->getColor();
            if (!childColor) {
                continue;
            }
            int childColorWidth = childColor->getWidth();
            int childColorHeight = childColor->getContents().size() / (childColorWidth * DataBlock::COLOR_BYTES);
            if (childColorWidth != colorWidth || childColorHeight != colorHeight) {
                qWarning() << "Color dimension mismatch [colorWidth=" << colorWidth << ", colorHeight=" << colorHeight <<
                    ", childColorWidth=" << childColorWidth << ", childColorHeight=" << childColorHeight << "]";
                continue;
            }
            int innerColorWidth = colorWidth - HeightfieldData::SHARED_EDGE;
            int innerColorHeight = colorHeight - HeightfieldData::SHARED_EDGE;
            int innerQuadrantColorWidth = innerColorWidth / 2;
            int innerQuadrantColorHeight = innerColorHeight / 2;
            int quadrantColorWidth = innerQuadrantColorWidth + HeightfieldData::SHARED_EDGE;
            int quadrantColorHeight = innerQuadrantColorHeight + HeightfieldData::SHARED_EDGE;
            char* dest = colorContents.data() + ((i & Y_MAXIMUM_FLAG ? innerQuadrantColorHeight * colorWidth : 0) +
                (i & X_MAXIMUM_FLAG ? innerQuadrantColorWidth : 0)) * DataBlock::COLOR_BYTES;
            const uchar* src = (const uchar*)childColor->getContents().constData();
            for (int z = 0; z < quadrantColorHeight; z++, dest += colorWidth * DataBlock::COLOR_BYTES,
                    src += colorWidth * DataBlock::COLOR_BYTES * 2) {
                const uchar* lineSrc = src;
                for (char* lineDest = dest, *end = dest + quadrantColorWidth * DataBlock::COLOR_BYTES;
                        lineDest != end; lineDest += DataBlock::COLOR_BYTES, lineSrc += DataBlock::COLOR_BYTES * 2) {
                    lineDest[0] = lineSrc[0];
                    lineDest[1] = lineSrc[1];
                    lineDest[2] = lineSrc[2];
                }
            }
        }
        _color = new HeightfieldColor(colorWidth, colorContents);
    
    } else {
        _color.reset();
    }
    if (materialWidth > 0 && colorMaterial) {
        QByteArray materialContents(materialWidth * materialHeight, 0);
        QVector<SharedObjectPointer> materials;
        for (int i = 0; i < CHILD_COUNT; i++) {
            HeightfieldMaterialPointer childMaterial = _children[i]->getMaterial();
            if (!childMaterial) {
                continue;
            }
            int childMaterialWidth = childMaterial->getWidth();
            int childMaterialHeight = childMaterial->getContents().size() / childMaterialWidth;
            if (childMaterialWidth != materialWidth || childMaterialHeight != materialHeight) {
                qWarning() << "Material dimension mismatch [materialWidth=" << materialWidth << ", materialHeight=" <<
                    materialHeight << ", childMaterialWidth=" << childMaterialWidth << ", childMaterialHeight=" <<
                    childMaterialHeight << "]";
                continue;
            }
            int innerMaterialWidth = materialWidth - HeightfieldData::SHARED_EDGE;
            int innerMaterialHeight = materialHeight - HeightfieldData::SHARED_EDGE;
            int innerQuadrantMaterialWidth = innerMaterialWidth / 2;
            int innerQuadrantMaterialHeight = innerMaterialHeight / 2;
            int quadrantMaterialWidth = innerQuadrantMaterialWidth + HeightfieldData::SHARED_EDGE;
            int quadrantMaterialHeight = innerQuadrantMaterialHeight + HeightfieldData::SHARED_EDGE;
            uchar* dest = (uchar*)materialContents.data() +
                (i & Y_MAXIMUM_FLAG ? innerQuadrantMaterialHeight * materialWidth : 0) +
                    (i & X_MAXIMUM_FLAG ? innerQuadrantMaterialWidth : 0);
            const uchar* src = (const uchar*)childMaterial->getContents().constData();
            QHash<int, int> materialMap;
            for (int z = 0; z < quadrantMaterialHeight; z++, dest += materialWidth, src += materialWidth * 2) {
                const uchar* lineSrc = src;
                for (uchar* lineDest = dest, *end = dest + quadrantMaterialWidth; lineDest != end; lineDest++, lineSrc += 2) {
                    int value = *lineSrc;
                    if (value != 0) {
                        int& mapping = materialMap[value];
                        if (mapping == 0) {
                            mapping = getMaterialIndex(childMaterial->getMaterials().at(value - 1),
                                materials, materialContents);
                        }
                        value = mapping;
                    }
                    *lineDest = value;
                }
            }
        }
        _material = new HeightfieldMaterial(materialWidth, materialContents, materials);
        
    } else {
        _material.reset();
    }
    if (stackWidth > 0) {
        QVector<StackArray> stackContents(stackWidth * stackHeight);
        QVector<SharedObjectPointer> stackMaterials;
        for (int i = 0; i < CHILD_COUNT; i++) {
            HeightfieldStackPointer childStack = _children[i]->getStack();
            if (!childStack) {
                continue;
            }
            int childStackWidth = childStack->getWidth();
            int childStackHeight = childStack->getContents().size() / childStackWidth;
            if (childStackWidth != stackWidth || childStackHeight != stackHeight) {
                qWarning() << "Stack dimension mismatch [stackWidth=" << stackWidth << ", stackHeight=" << stackHeight <<
                    ", childStackWidth=" << childStackWidth << ", childStackHeight=" << childStackHeight << "]";
                continue;
            }
            int innerStackWidth = stackWidth - HeightfieldData::SHARED_EDGE;
            int innerStackHeight = stackHeight - HeightfieldData::SHARED_EDGE;
            int innerQuadrantStackWidth = innerStackWidth / 2;
            int innerQuadrantStackHeight = innerStackHeight / 2;
            int quadrantStackWidth = innerQuadrantStackWidth + HeightfieldData::SHARED_EDGE;
            int quadrantStackHeight = innerQuadrantStackHeight + HeightfieldData::SHARED_EDGE;
            StackArray* dest = stackContents.data() + (i & Y_MAXIMUM_FLAG ? innerQuadrantStackHeight * stackWidth : 0) +
                (i & X_MAXIMUM_FLAG ? innerQuadrantStackWidth : 0);
            const StackArray* src = childStack->getContents().constData();
            QHash<int, int> materialMap;
            for (int z = 0; z < quadrantStackHeight; z++, dest += stackWidth, src += stackWidth * 2) {
                const StackArray* lineSrc = src;
                StackArray* lineDest = dest;
                for (int x = 0; x < quadrantStackWidth; x++, lineDest++, lineSrc += 2) {
                    if (lineSrc->isEmpty()) {
                        continue;
                    }
                    int minimumY = lineSrc->getPosition();
                    int maximumY = lineSrc->getPosition() + lineSrc->getEntryCount() - 1;
                    int newMinimumY = minimumY / 2;
                    int newMaximumY = maximumY / 2;
                    *lineDest = StackArray(newMaximumY - newMinimumY + 1);
                    lineDest->setPosition(newMinimumY);
                    int y = newMinimumY;
                    for (StackArray::Entry* destEntry = lineDest->getEntryData(), *end = destEntry + lineDest->getEntryCount();
                            destEntry != end; destEntry++, y++) {
                        int srcY = y * 2;
                        const StackArray::Entry& srcEntry = lineSrc->getEntry(srcY);
                        destEntry->color = srcEntry.color;
                        destEntry->material = srcEntry.material;
                        if (destEntry->material != 0) {
                            int& mapping = materialMap[destEntry->material];
                            if (mapping == 0) {
                                mapping = getMaterialIndex(childStack->getMaterials().at(destEntry->material - 1),
                                    stackMaterials, stackContents);
                            }
                            destEntry->material = mapping;
                        }
                        int srcAlpha = qAlpha(srcEntry.color);
                        glm::vec3 normal;
                        if (srcAlpha != lineSrc->getEntryAlpha(srcY + 2)) {
                            const StackArray::Entry& nextSrcEntry = lineSrc->getEntry(srcY + 1);
                            if (qAlpha(nextSrcEntry.color) == srcAlpha) {
                                float distance = nextSrcEntry.getHermiteY(normal);
                                destEntry->setHermiteY(normal, distance * 0.5f + 0.5f);
                            } else {
                                float distance = srcEntry.getHermiteY(normal);
                                destEntry->setHermiteY(normal, distance * 0.5f);
                            }
                        }
                        if (x != quadrantStackWidth - 1 && srcAlpha != lineSrc[2].getEntryAlpha(srcY)) {
                            const StackArray::Entry& nextSrcEntry = lineSrc[1].getEntry(srcY);
                            if (qAlpha(nextSrcEntry.color) == srcAlpha) {
                                float distance = nextSrcEntry.getHermiteX(normal);
                                destEntry->setHermiteX(normal, distance * 0.5f + 0.5f);
                            } else {
                                float distance = srcEntry.getHermiteX(normal);
                                destEntry->setHermiteX(normal, distance * 0.5f);
                            }
                        }
                        if (z != quadrantStackHeight - 1 && srcAlpha != lineSrc[2 * stackWidth].getEntryAlpha(srcY)) {
                            const StackArray::Entry& nextSrcEntry = lineSrc[stackWidth].getEntry(srcY);
                            if (qAlpha(nextSrcEntry.color) == srcAlpha) {
                                float distance = nextSrcEntry.getHermiteZ(normal);
                                destEntry->setHermiteZ(normal, distance * 0.5f + 0.5f);
                            } else {
                                float distance = srcEntry.getHermiteZ(normal);
                                destEntry->setHermiteZ(normal, distance * 0.5f);
                            }
                        }
                    }
                }
            }
        }
        _stack = new HeightfieldStack(stackWidth, stackContents, stackMaterials);
        
    } else {
        _stack.reset();
    }
}

QRgb HeightfieldNode::getColorAt(const glm::vec3& location) const {
    if (location.x < 0.0f || location.z < 0.0f || location.x > 1.0f || location.z > 1.0f) {
        return 0;
    }
    int width = _color->getWidth();
    const QByteArray& contents = _color->getContents();
    const uchar* src = (const uchar*)contents.constData();
    int height = contents.size() / (width * DataBlock::COLOR_BYTES);
    int innerWidth = width - HeightfieldData::SHARED_EDGE;
    int innerHeight = height - HeightfieldData::SHARED_EDGE;
    
    glm::vec3 relative = location * glm::vec3((float)innerWidth, 1.0f, (float)innerHeight);
    glm::vec3 floors = glm::floor(relative);
    glm::vec3 ceils = glm::ceil(relative);
    glm::vec3 fracts = glm::fract(relative);
    int floorX = (int)floors.x;
    int floorZ = (int)floors.z;
    int ceilX = (int)ceils.x;
    int ceilZ = (int)ceils.z;
    const uchar* upperLeft = src + (floorZ * width + floorX) * DataBlock::COLOR_BYTES;
    const uchar* lowerRight = src + (ceilZ * width + ceilX) * DataBlock::COLOR_BYTES;
    glm::vec3 interpolatedColor = glm::mix(glm::vec3(upperLeft[0], upperLeft[1], upperLeft[2]),
        glm::vec3(lowerRight[0], lowerRight[1], lowerRight[2]), fracts.z);
    
    // the final vertex (and thus which triangle we check) depends on which half we're on
    if (fracts.x >= fracts.z) {
        const uchar* upperRight = src + (floorZ * width + ceilX) * DataBlock::COLOR_BYTES;
        interpolatedColor = glm::mix(interpolatedColor, glm::mix(glm::vec3(upperRight[0], upperRight[1], upperRight[2]),
            glm::vec3(lowerRight[0], lowerRight[1], lowerRight[2]), fracts.z), (fracts.x - fracts.z) / (1.0f - fracts.z));
        
    } else {
        const uchar* lowerLeft = src + (ceilZ * width + floorX) * DataBlock::COLOR_BYTES;
        interpolatedColor = glm::mix(glm::mix(glm::vec3(upperLeft[0], upperLeft[1], upperLeft[2]),
            glm::vec3(lowerLeft[0], lowerLeft[1], lowerLeft[2]), fracts.z), interpolatedColor, fracts.x / fracts.z);
    }
    return qRgb(interpolatedColor.r, interpolatedColor.g, interpolatedColor.b);
}
    
int HeightfieldNode::getMaterialAt(const glm::vec3& location) const {
    if (location.x < 0.0f || location.z < 0.0f || location.x > 1.0f || location.z > 1.0f) {
        return -1;
    }
    int width = _material->getWidth();
    const QByteArray& contents = _material->getContents();
    const uchar* src = (const uchar*)contents.constData();
    int height = contents.size() / width;
    int innerWidth = width - HeightfieldData::SHARED_EDGE;
    int innerHeight = height - HeightfieldData::SHARED_EDGE;
    
    glm::vec3 relative = location * glm::vec3((float)innerWidth, 1.0f, (float)innerHeight);
    return src[(int)glm::round(relative.z) * width + (int)glm::round(relative.x)];
}

void HeightfieldNode::maybeRenormalize(const glm::vec3& scale, float normalizeScale, float normalizeOffset,
        int innerStackWidth, QVector<quint16>& heightContents, QVector<StackArray>& stackContents) {
    if (normalizeScale == 1.0f && normalizeOffset == 0.0f) {
        return;
    }
    for (quint16* dest = heightContents.data(), *end = dest + heightContents.size(); dest != end; dest++) {
        int value = *dest;
        if (value != 0) {
            *dest = (value + normalizeOffset) * normalizeScale;
        }
    }
    if (stackContents.isEmpty()) {
        return;
    }
    int stackOffset = glm::round(scale.y * normalizeOffset * normalizeScale * innerStackWidth /
        (numeric_limits<quint16>::max() * scale.x));
    for (StackArray* dest = stackContents.data(), *end = dest + stackContents.size(); dest != end; dest++) {
        if (!dest->isEmpty()) {
            dest->getPositionRef() += stackOffset;
        }
    }
}

bool HeightfieldNode::findHeightfieldRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        float boundsDistance, float& distance) const {
    if (!_height) {
        return false;
    }
    int width = _height->getWidth();
    const QVector<quint16>& contents = _height->getContents();
    const quint16* src = contents.constData();
    int height = contents.size() / width;
    int innerWidth = width - HeightfieldHeight::HEIGHT_EXTENSION;
    int innerHeight = height - HeightfieldHeight::HEIGHT_EXTENSION;
    int highestX = innerWidth + HeightfieldHeight::HEIGHT_BORDER;
    int highestZ = innerHeight + HeightfieldHeight::HEIGHT_BORDER;
    
    glm::vec3 heightScale((float)innerWidth, (float)numeric_limits<quint16>::max(), (float)innerHeight);
    glm::vec3 dir = direction * heightScale;
    glm::vec3 entry = origin * heightScale + dir * boundsDistance;
    
    entry.x += HeightfieldHeight::HEIGHT_BORDER;
    entry.z += HeightfieldHeight::HEIGHT_BORDER;
    glm::vec3 floors = glm::floor(entry);
    glm::vec3 ceils = glm::ceil(entry);
    if (floors.x == ceils.x) {
        if (dir.x > 0.0f) {
            ceils.x += 1.0f;
        } else {
            floors.x -= 1.0f;
        } 
    }
    if (floors.z == ceils.z) {
        if (dir.z > 0.0f) {
            ceils.z += 1.0f;
        } else {
            floors.z -= 1.0f;
        }
    }
    
    bool withinBounds = true;
    float accumulatedDistance = 0.0f;
    while (withinBounds) {
        // find the heights at the corners of the current cell
        int floorX = qMin(qMax((int)floors.x, HeightfieldHeight::HEIGHT_BORDER), highestX);
        int floorZ = qMin(qMax((int)floors.z, HeightfieldHeight::HEIGHT_BORDER), highestZ);
        int ceilX = qMin(qMax((int)ceils.x, HeightfieldHeight::HEIGHT_BORDER), highestX);
        int ceilZ = qMin(qMax((int)ceils.z, HeightfieldHeight::HEIGHT_BORDER), highestZ);
        float upperLeft = src[floorZ * width + floorX];
        float upperRight = src[floorZ * width + ceilX];
        float lowerLeft = src[ceilZ * width + floorX];
        float lowerRight = src[ceilZ * width + ceilX];
        
        // find the distance to the next x coordinate
        float xDistance = FLT_MAX;
        if (dir.x > 0.0f) {
            xDistance = (ceils.x - entry.x) / dir.x;
        } else if (dir.x < 0.0f) {
            xDistance = (floors.x - entry.x) / dir.x;
        }
        
        // and the distance to the next z coordinate
        float zDistance = FLT_MAX;
        if (dir.z > 0.0f) {
            zDistance = (ceils.z - entry.z) / dir.z;
        } else if (dir.z < 0.0f) {
            zDistance = (floors.z - entry.z) / dir.z;
        }
        
        // the exit distance is the lower of those two
        float exitDistance = qMin(xDistance, zDistance);
        glm::vec3 exit, nextFloors = floors, nextCeils = ceils;
        if (exitDistance == FLT_MAX) {
            if (dir.y > 0.0f) {
                return false; // line points upwards; no collisions possible
            }    
            withinBounds = false; // line points downwards; check this cell only
            
        } else {
            // find the exit point and the next cell, and determine whether it's still within the bounds
            exit = entry + exitDistance * dir;
            withinBounds = (exit.y >= 0.0f && exit.y <= numeric_limits<quint16>::max());
            if (exitDistance == xDistance) {
                if (dir.x > 0.0f) {
                    nextFloors.x += 1.0f;
                    withinBounds &= (nextCeils.x += 1.0f) <= highestX;
                } else {
                    withinBounds &= (nextFloors.x -= 1.0f) >= HeightfieldHeight::HEIGHT_BORDER;
                    nextCeils.x -= 1.0f;
                }
            }
            if (exitDistance == zDistance) {
                if (dir.z > 0.0f) {
                    nextFloors.z += 1.0f;
                    withinBounds &= (nextCeils.z += 1.0f) <= highestZ;
                } else {
                    withinBounds &= (nextFloors.z -= 1.0f) >= HeightfieldHeight::HEIGHT_BORDER;
                    nextCeils.z -= 1.0f;
                }
            }
            // check the vertical range of the ray against the ranges of the cell heights
            if (upperLeft == 0 || upperRight == 0 || lowerLeft == 0 || lowerRight == 0 ||
                    qMin(entry.y, exit.y) > qMax(qMax(upperLeft, upperRight), qMax(lowerLeft, lowerRight)) ||
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
        float lowerProduct = glm::dot(lowerNormal, dir);
        if (lowerProduct < 0.0f) {
            float planeDistance = -glm::dot(lowerNormal, relativeEntry) / lowerProduct;
            glm::vec3 intersection = relativeEntry + planeDistance * dir;
            if (intersection.x >= 0.0f && intersection.x <= 1.0f && intersection.z >= 0.0f && intersection.z <= 1.0f &&
                    intersection.z >= intersection.x) {
                distance = boundsDistance + accumulatedDistance + planeDistance;
                return true;
            }
        }
        
        // then the one with the X+ segment
        glm::vec3 upperNormal(upperLeft - upperRight, 1.0f, upperRight - lowerRight);
        float upperProduct = glm::dot(upperNormal, dir);
        if (upperProduct < 0.0f) {
            float planeDistance = -glm::dot(upperNormal, relativeEntry) / upperProduct;
            glm::vec3 intersection = relativeEntry + planeDistance * dir;
            if (intersection.x >= 0.0f && intersection.x <= 1.0f && intersection.z >= 0.0f && intersection.z <= 1.0f &&
                    intersection.x >= intersection.z) {
                distance = boundsDistance + accumulatedDistance + planeDistance;
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

static inline float mixHeights(float firstHeight, float secondHeight, float t) {
    return (firstHeight == 0.0f) ? secondHeight : (secondHeight == 0.0f ? firstHeight :
        glm::mix(firstHeight, secondHeight, t));
}

HeightfieldNode* HeightfieldNode::subdivide(const QVector<quint16>& heightContents,
        const QVector<StackArray>& stackContents) const {
    HeightfieldNode* newNode = new HeightfieldNode(*this);
    int heightWidth = _height->getWidth();
    int heightHeight = heightContents.size() / heightWidth;
    newNode->setHeight(HeightfieldHeightPointer(new HeightfieldHeight(heightWidth, heightContents)));
    int stackWidth = 0, stackHeight = 0;
    QVector<SharedObjectPointer> stackMaterials;
    if (_stack) {
        stackWidth = _stack->getWidth();
        stackHeight = stackContents.size() / stackWidth;
        stackMaterials = _stack->getMaterials();
        newNode->setStack(HeightfieldStackPointer(new HeightfieldStack(stackWidth, stackContents, stackMaterials)));
    }
    int colorWidth = 0, colorHeight = 0;
    if (_color) {
        colorWidth = _color->getWidth();
        colorHeight = _color->getContents().size() / (colorWidth * DataBlock::COLOR_BYTES);
    }
    int materialWidth = 0, materialHeight = 0;
    QVector<SharedObjectPointer> materialMaterials;
    if (_material) {
        materialWidth = _material->getWidth();
        materialHeight = _material->getContents().size() / materialWidth;
        materialMaterials = _material->getMaterials();
    }
    for (int i = 0; i < CHILD_COUNT; i++) {
        QVector<quint16> childHeightContents(heightWidth * heightHeight);
        QByteArray childColorContents(colorWidth * colorHeight * DataBlock::COLOR_BYTES, 0xFF);
        QByteArray childMaterialContents(materialWidth * materialHeight, 0);
        QVector<StackArray> childStackContents(stackWidth * stackHeight);
        
        quint16* heightDest = childHeightContents.data();
        const quint16* heightSrc = heightContents.constData() + (i & Y_MAXIMUM_FLAG ? (heightHeight / 2) * heightWidth : 0) +
            (i & X_MAXIMUM_FLAG ? heightWidth / 2 : 0);
        for (int z = 0; z < heightHeight; z++) {
            float srcZ = z * 0.5f + 0.5f;
            float fractZ = glm::fract(srcZ);
            const quint16* heightSrcZ = heightSrc + (int)srcZ * heightWidth;
            for (int x = 0; x < heightWidth; x++) {
                float srcX = x * 0.5f + 0.5f;
                float fractX = glm::fract(srcX);
                const quint16* heightSrcX = heightSrcZ + (int)srcX;
                if (fractZ == 0.0f) {
                    if (fractX == 0.0f) {
                        *heightDest++ = heightSrcX[0];
                    } else {
                        *heightDest++ = mixHeights(heightSrcX[0], heightSrcX[1], fractX);
                    }
                } else {
                    if (fractX == 0.0f) {
                        *heightDest++ = mixHeights(heightSrcX[0], heightSrcX[heightWidth], fractZ);
                    } else {
                        *heightDest++ = mixHeights(mixHeights(heightSrcX[0], heightSrcX[1], fractX),
                            mixHeights(heightSrcX[heightWidth], heightSrcX[heightWidth + 1], fractX), fractZ);
                    }
                } 
            }
        }
        
        if (colorWidth != 0) {
            char* colorDest = childColorContents.data();
            const uchar* colorSrc = (const uchar*)_color->getContents().constData() +
                ((i & Y_MAXIMUM_FLAG ? (colorHeight / 2) * colorWidth : 0) +
                (i & X_MAXIMUM_FLAG ? colorWidth / 2 : 0)) * DataBlock::COLOR_BYTES;
            for (int z = 0; z < colorHeight; z++) {
                float srcZ = z * 0.5f;
                float fractZ = glm::fract(srcZ);
                const uchar* colorSrcZ = colorSrc + (int)srcZ * colorWidth * DataBlock::COLOR_BYTES;
                for (int x = 0; x < colorWidth; x++) {
                    float srcX = x * 0.5f;
                    float fractX = glm::fract(srcX);
                    const uchar* colorSrcX = colorSrcZ + (int)srcX * DataBlock::COLOR_BYTES;
                    const uchar* nextColorSrcX = colorSrcX + colorWidth * DataBlock::COLOR_BYTES;
                    if (fractZ == 0.0f) {
                        if (fractX == 0.0f) {
                            *colorDest++ = colorSrcX[0];
                            *colorDest++ = colorSrcX[1];
                            *colorDest++ = colorSrcX[2];
                        } else {
                            *colorDest++ = glm::mix(colorSrcX[0], colorSrcX[3], fractX);
                            *colorDest++ = glm::mix(colorSrcX[1], colorSrcX[4], fractX);
                            *colorDest++ = glm::mix(colorSrcX[2], colorSrcX[5], fractX);
                        }
                    } else {
                        if (fractX == 0.0f) {
                            *colorDest++ = glm::mix(colorSrcX[0], nextColorSrcX[0], fractZ);
                            *colorDest++ = glm::mix(colorSrcX[1], nextColorSrcX[1], fractZ);
                            *colorDest++ = glm::mix(colorSrcX[2], nextColorSrcX[2], fractZ);
                        } else {
                            *colorDest++ = glm::mix(glm::mix(colorSrcX[0], colorSrcX[3], fractX),
                                glm::mix(nextColorSrcX[0], nextColorSrcX[3], fractX), fractZ);
                            *colorDest++ = glm::mix(glm::mix(colorSrcX[1], colorSrcX[4], fractX),
                                glm::mix(nextColorSrcX[1], nextColorSrcX[4], fractX), fractZ);
                            *colorDest++ = glm::mix(glm::mix(colorSrcX[2], colorSrcX[5], fractX),
                                glm::mix(nextColorSrcX[2], nextColorSrcX[5], fractX), fractZ);
                        }
                    }
                }
            }
        }
        
        if (materialWidth != 0) {
            char* materialDest = childMaterialContents.data();
            const char* materialSrc = _material->getContents().constData() +
                (i & Y_MAXIMUM_FLAG ? (materialHeight / 2) * materialWidth : 0) +
                (i & X_MAXIMUM_FLAG ? materialWidth / 2 : 0);
            for (int z = 0; z < materialHeight; z++) {
                float srcZ = z * 0.5f;
                const char* materialSrcZ = materialSrc + (int)srcZ * materialWidth;
                for (int x = 0; x < materialWidth; x++) {
                    float srcX = x * 0.5f;
                    const char* materialSrcX = materialSrcZ + (int)srcX;
                    *materialDest++ = *materialSrcX;
                }
            }
        }
        
        if (stackWidth != 0) {
            StackArray* stackDest = childStackContents.data();
            const StackArray* stackSrc = _stack->getContents().constData() +
                (i & Y_MAXIMUM_FLAG ? (stackHeight / 2) * stackWidth : 0) +
                (i & X_MAXIMUM_FLAG ? stackWidth / 2 : 0);
            for (int z = 0; z < stackHeight; z++) {
                float srcZ = z * 0.5f;
                float fractZ = glm::fract(srcZ);
                const StackArray* stackSrcZ = stackSrc + (int)srcZ * stackWidth;
                for (int x = 0; x < stackWidth; x++) {
                    float srcX = x * 0.5f;
                    float fractX = glm::fract(srcX);
                    const StackArray* stackSrcX = stackSrcZ + (int)srcX;
                    if (stackSrcX->isEmpty()) {
                        stackDest++;
                        continue;
                    }
                    int minimumY = stackSrcX->getPosition() * 2;
                    int maximumY = (stackSrcX->getPosition() + stackSrcX->getEntryCount() - 1) * 2;
                    *stackDest = StackArray(maximumY - minimumY + 1);
                    stackDest->setPosition(minimumY);
                    for (int y = minimumY; y <= maximumY; y++) {
                        float srcY = y * 0.5f;
                        float fractY = glm::fract(srcY);
                        const StackArray::Entry& srcEntry = stackSrcX->getEntry((int)srcY);
                        StackArray::Entry& destEntry = stackDest->getEntry(y);
                        destEntry.color = srcEntry.color;
                        destEntry.material = srcEntry.material;
                        if (srcEntry.hermiteX != 0) {
                            glm::vec3 normal;
                            float distance = srcEntry.getHermiteX(normal);
                            if (distance < fractX) {
                                const StackArray::Entry& nextSrcEntryX = stackSrcX[1].getEntry((int)srcY);
                                destEntry.color = nextSrcEntryX.color;
                                destEntry.material = nextSrcEntryX.material;
                            
                            } else {
                                destEntry.setHermiteX(normal, (distance - fractX) / 0.5f);
                            }
                        }
                        if (srcEntry.hermiteY != 0) {
                            glm::vec3 normal;
                            float distance = srcEntry.getHermiteY(normal);
                            if (distance < fractY) {
                                const StackArray::Entry& nextSrcEntryY = stackSrcX->getEntry((int)srcY + 1);
                                destEntry.color = nextSrcEntryY.color;
                                destEntry.material = nextSrcEntryY.material;
                            
                            } else {
                                destEntry.setHermiteY(normal, (distance - fractY) / 0.5f);
                            }
                        }
                        if (srcEntry.hermiteZ != 0) {
                            glm::vec3 normal;
                            float distance = srcEntry.getHermiteZ(normal);
                            if (distance < fractZ) {
                                const StackArray::Entry& nextSrcEntryZ = stackSrcX[stackWidth].getEntry((int)srcY);
                                destEntry.color = nextSrcEntryZ.color;
                                destEntry.material = nextSrcEntryZ.material;
                            
                            } else {
                                destEntry.setHermiteZ(normal, (distance - fractZ) / 0.5f);
                            }
                        }
                    }
                    stackDest++;
                }
            }
        }
        
        newNode->setChild(i, HeightfieldNodePointer(new HeightfieldNode(
            HeightfieldHeightPointer(new HeightfieldHeight(heightWidth, childHeightContents)),
            HeightfieldColorPointer(colorWidth == 0 ? NULL : new HeightfieldColor(colorWidth, childColorContents)),
            HeightfieldMaterialPointer(materialWidth == 0 ? NULL :
                new HeightfieldMaterial(materialWidth, childMaterialContents, materialMaterials)),
            HeightfieldStackPointer(stackWidth == 0 ? NULL :
                new HeightfieldStack(stackWidth, childStackContents, stackMaterials)))));
    }
    return newNode;
}

AbstractHeightfieldNodeRenderer::~AbstractHeightfieldNodeRenderer() {
}

bool AbstractHeightfieldNodeRenderer::findRayIntersection(const glm::vec3& translation,
        const glm::quat& rotation, const glm::vec3& scale, const glm::vec3& origin, const glm::vec3& direction,
        float boundsDistance, float& distance) const {
    return false;
}

Heightfield::Heightfield() :
    _aspectY(1.0f),
    _aspectZ(1.0f) {
    
    connect(this, &Heightfield::translationChanged, this, &Heightfield::updateBounds);
    connect(this, &Heightfield::rotationChanged, this, &Heightfield::updateBounds);
    connect(this, &Heightfield::scaleChanged, this, &Heightfield::updateBounds);
    connect(this, &Heightfield::aspectYChanged, this, &Heightfield::updateBounds);
    connect(this, &Heightfield::aspectZChanged, this, &Heightfield::updateBounds);
    updateBounds();
    
    connect(this, &Heightfield::heightChanged, this, &Heightfield::updateRoot);
    connect(this, &Heightfield::colorChanged, this, &Heightfield::updateRoot);
    connect(this, &Heightfield::materialChanged, this, &Heightfield::updateRoot);
    connect(this, &Heightfield::stackChanged, this, &Heightfield::updateRoot);
    updateRoot();
}

void Heightfield::setAspectY(float aspectY) {
    if (_aspectY != aspectY) {
        emit aspectYChanged(_aspectY = aspectY);
    }
}

void Heightfield::setAspectZ(float aspectZ) {
    if (_aspectZ != aspectZ) {
        emit aspectZChanged(_aspectZ = aspectZ);
    }
}

void Heightfield::setHeight(const HeightfieldHeightPointer& height) {
    if (_height != height) {
        emit heightChanged(_height = height);
    }
}

void Heightfield::setColor(const HeightfieldColorPointer& color) {
    if (_color != color) {
        emit colorChanged(_color = color);
    }
}

void Heightfield::setMaterial(const HeightfieldMaterialPointer& material) {
    if (_material != material) {
        emit materialChanged(_material = material);
    }
}

void Heightfield::setStack(const HeightfieldStackPointer& stack) {
    if (_stack != stack) {
        emit stackChanged(_stack = stack);
    }
}

MetavoxelLOD Heightfield::transformLOD(const MetavoxelLOD& lod) const {
    // after transforming into unit space, we scale the threshold in proportion to vertical distance
    glm::vec3 inverseScale(1.0f / getScale(), 1.0f / (getScale() * _aspectY), 1.0f / (getScale() * _aspectZ));
    glm::vec3 position = glm::inverse(getRotation()) * (lod.position - getTranslation()) * inverseScale;
    const float THRESHOLD_MULTIPLIER = 256.0f;
    return MetavoxelLOD(glm::vec3(position.x, position.z, 0.0f), lod.threshold *
        qMax(0.5f, glm::abs(position.y * _aspectY - 0.5f)) * THRESHOLD_MULTIPLIER);
}

SharedObject* Heightfield::clone(bool withID, SharedObject* target) const {
    Heightfield* newHeightfield = static_cast<Heightfield*>(Spanner::clone(withID, target));
    newHeightfield->setHeight(_height);
    newHeightfield->setColor(_color);
    newHeightfield->setMaterial(_material);
    newHeightfield->setRoot(_root);
    return newHeightfield;
}

bool Heightfield::isHeightfield() const {
    return true;
}

float Heightfield::getHeight(const glm::vec3& location) const {
    float distance;
    glm::vec3 down = getRotation() * glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 origin = location - down * (glm::dot(down, location) + getScale() * _aspectY - glm::dot(down, getTranslation()));
    if (findRayIntersection(origin, down, distance)) {
        return origin.y + distance * down.y;
    }
    return -FLT_MAX;
}

bool Heightfield::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    return _root->findRayIntersection(getTranslation(), getRotation(), glm::vec3(getScale(), getScale() * _aspectY,
        getScale() * _aspectZ), origin, direction, distance);
}

Spanner* Heightfield::paintHeight(const glm::vec3& position, float radius, float height,
        bool set, bool erase, float granularity) {
    // first see if we're going to exceed the range limits
    float minimumValue = 1.0f, maximumValue = numeric_limits<quint16>::max();
    if (set) {
        float heightValue = height * numeric_limits<quint16>::max() / (getScale() * _aspectY);
        minimumValue = qMin(minimumValue, heightValue);    
        maximumValue = qMax(maximumValue, heightValue);
    
    } else if (!erase) {
        _root->getRangeAfterHeightPaint(getTranslation(), getRotation(), glm::vec3(getScale(), getScale() * _aspectY,
            getScale() * _aspectZ), position, radius, height, minimumValue, maximumValue);
    }
    
    // normalize if necessary
    float normalizeScale, normalizeOffset;
    Heightfield* newHeightfield = prepareEdit(minimumValue, maximumValue, normalizeScale, normalizeOffset);
    newHeightfield->setRoot(HeightfieldNodePointer(_root->paintHeight(newHeightfield->getTranslation(), getRotation(),
        glm::vec3(getScale(), getScale() * newHeightfield->getAspectY(), getScale() * _aspectZ), position, radius, height,
        set, erase, normalizeScale, normalizeOffset, granularity)));
    return newHeightfield;
}

Spanner* Heightfield::fillHeight(const glm::vec3& position, float radius, float granularity) {
    Heightfield* newHeightfield = static_cast<Heightfield*>(clone(true));
    newHeightfield->setRoot(HeightfieldNodePointer(_root->fillHeight(getTranslation(), getRotation(),
        glm::vec3(getScale(), getScale() * _aspectY, getScale() * _aspectZ), position, radius, granularity)));
    return newHeightfield;
}

Spanner* Heightfield::setMaterial(const SharedObjectPointer& spanner, const SharedObjectPointer& material,
        const QColor& color, bool paint, bool voxelize, float granularity) {
    // first see if we're going to exceed the range limits, normalizing if necessary
    Spanner* spannerData = static_cast<Spanner*>(spanner.data());
    float normalizeScale = 1.0f, normalizeOffset = 0.0f;
    Heightfield* newHeightfield;
    if (paint) {
        newHeightfield = static_cast<Heightfield*>(clone(true));
    } else {    
        float minimumValue = 1.0f, maximumValue = numeric_limits<quint16>::max();
        _root->getRangeAfterEdit(getTranslation(), getRotation(), glm::vec3(getScale(), getScale() * _aspectY,
            getScale() * _aspectZ), spannerData->getBounds(), minimumValue, maximumValue);
        newHeightfield = prepareEdit(minimumValue, maximumValue, normalizeScale, normalizeOffset);
    }
    newHeightfield->setRoot(HeightfieldNodePointer(_root->setMaterial(newHeightfield->getTranslation(), getRotation(),
        glm::vec3(getScale(), getScale() * newHeightfield->getAspectY(), getScale() * _aspectZ), spannerData,
        material, color, paint, voxelize, normalizeScale, normalizeOffset, granularity)));
    return newHeightfield;
}

bool Heightfield::hasOwnColors() const {
    return _color;
}

bool Heightfield::hasOwnMaterials() const {
    return _material;
}

QRgb Heightfield::getColorAt(const glm::vec3& point) {
    int width = _color->getWidth();
    const QByteArray& contents = _color->getContents();
    const uchar* src = (const uchar*)contents.constData();
    int height = contents.size() / (width * DataBlock::COLOR_BYTES);
    int innerWidth = width - HeightfieldData::SHARED_EDGE;
    int innerHeight = height - HeightfieldData::SHARED_EDGE;
    
    glm::vec3 relative = glm::inverse(getRotation()) * (point - getTranslation()) * glm::vec3(innerWidth / getScale(),
        1.0f, innerHeight / (getScale() * _aspectZ));
    if (relative.x < 0.0f || relative.z < 0.0f || relative.x > width - 1 || relative.z > height - 1) {
        return 0;
    }
    glm::vec3 floors = glm::floor(relative);
    glm::vec3 ceils = glm::ceil(relative);
    glm::vec3 fracts = glm::fract(relative);
    int floorX = (int)floors.x;
    int floorZ = (int)floors.z;
    int ceilX = (int)ceils.x;
    int ceilZ = (int)ceils.z;
    const uchar* upperLeft = src + (floorZ * width + floorX) * DataBlock::COLOR_BYTES;
    const uchar* lowerRight = src + (ceilZ * width + ceilX) * DataBlock::COLOR_BYTES;
    glm::vec3 interpolatedColor = glm::mix(glm::vec3(upperLeft[0], upperLeft[1], upperLeft[2]),
        glm::vec3(lowerRight[0], lowerRight[1], lowerRight[2]), fracts.z);
    
    // the final vertex (and thus which triangle we check) depends on which half we're on
    if (fracts.x >= fracts.z) {
        const uchar* upperRight = src + (floorZ * width + ceilX) * DataBlock::COLOR_BYTES;
        interpolatedColor = glm::mix(interpolatedColor, glm::mix(glm::vec3(upperRight[0], upperRight[1], upperRight[2]),
            glm::vec3(lowerRight[0], lowerRight[1], lowerRight[2]), fracts.z), (fracts.x - fracts.z) / (1.0f - fracts.z));
        
    } else {
        const uchar* lowerLeft = src + (ceilZ * width + floorX) * DataBlock::COLOR_BYTES;
        interpolatedColor = glm::mix(glm::mix(glm::vec3(upperLeft[0], upperLeft[1], upperLeft[2]),
            glm::vec3(lowerLeft[0], lowerLeft[1], lowerLeft[2]), fracts.z), interpolatedColor, fracts.x / fracts.z);
    }
    return qRgb(interpolatedColor.r, interpolatedColor.g, interpolatedColor.b);
}

int Heightfield::getMaterialAt(const glm::vec3& point) {
    int width = _material->getWidth();
    const QByteArray& contents = _material->getContents();
    const uchar* src = (const uchar*)contents.constData();
    int height = contents.size() / width;
    int innerWidth = width - HeightfieldData::SHARED_EDGE;
    int innerHeight = height - HeightfieldData::SHARED_EDGE;
    
    glm::vec3 relative = glm::inverse(getRotation()) * (point - getTranslation()) * glm::vec3(innerWidth / getScale(),
        1.0f, innerHeight / (getScale() * _aspectZ));
    if (relative.x < 0.0f || relative.z < 0.0f || relative.x > width - 1 || relative.z > height - 1) {
        return -1;
    }
    return src[(int)glm::round(relative.z) * width + (int)glm::round(relative.x)];
}

QVector<SharedObjectPointer>& Heightfield::getMaterials() {
    return _material->getMaterials();
}

bool Heightfield::contains(const glm::vec3& point) {
    if (!_height) {
        return false;
    }
    int width = _height->getWidth();
    const QVector<quint16>& contents = _height->getContents();
    const quint16* src = contents.constData();
    int height = contents.size() / width;
    int innerWidth = width - HeightfieldHeight::HEIGHT_EXTENSION;
    int innerHeight = height - HeightfieldHeight::HEIGHT_EXTENSION;
    int highestX = innerWidth + HeightfieldHeight::HEIGHT_BORDER;
    int highestZ = innerHeight + HeightfieldHeight::HEIGHT_BORDER;
    
    glm::vec3 relative = glm::inverse(getRotation()) * (point - getTranslation()) * glm::vec3(innerWidth / getScale(),
        numeric_limits<quint16>::max() / (getScale() * _aspectY), innerHeight / (getScale() * _aspectZ));
    if (relative.x < 0.0f || relative.y < 0.0f || relative.z < 0.0f || relative.x > innerWidth ||
            relative.y > numeric_limits<quint16>::max() || relative.z > innerHeight) {
        return false;
    }
    relative.x += HeightfieldHeight::HEIGHT_BORDER;
    relative.z += HeightfieldHeight::HEIGHT_BORDER;
    
    // find the bounds of the cell containing the point and the shared vertex heights
    glm::vec3 floors = glm::floor(relative);
    glm::vec3 ceils = glm::ceil(relative);
    glm::vec3 fracts = glm::fract(relative);
    int floorX = qMin(qMax((int)floors.x, HeightfieldHeight::HEIGHT_BORDER), highestX);
    int floorZ = qMin(qMax((int)floors.z, HeightfieldHeight::HEIGHT_BORDER), highestZ);
    int ceilX = qMin(qMax((int)ceils.x, HeightfieldHeight::HEIGHT_BORDER), highestX);
    int ceilZ = qMin(qMax((int)ceils.z, HeightfieldHeight::HEIGHT_BORDER), highestZ);
    float upperLeft = src[floorZ * width + floorX];
    float lowerRight = src[ceilZ * width + ceilX];
    float interpolatedHeight = glm::mix(upperLeft, lowerRight, fracts.z);
    
    // the final vertex (and thus which triangle we check) depends on which half we're on
    if (fracts.x >= fracts.z) {
        float upperRight = src[floorZ * width + ceilX];
        interpolatedHeight = glm::mix(interpolatedHeight, glm::mix(upperRight, lowerRight, fracts.z),
            (fracts.x - fracts.z) / (1.0f - fracts.z));
        
    } else {
        float lowerLeft = src[ceilZ * width + floorX];
        interpolatedHeight = glm::mix(glm::mix(upperLeft, lowerLeft, fracts.z), interpolatedHeight, fracts.x / fracts.z);
    }
    if (interpolatedHeight == 0.0f) {
        return false; // ignore zero values
    }
    
    // compare
    return relative.y <= interpolatedHeight;
}

bool Heightfield::intersects(const glm::vec3& start, const glm::vec3& end, float& distance, glm::vec3& normal) {
    int width = _height->getWidth();
    const QVector<quint16>& contents = _height->getContents();
    const quint16* src = contents.constData();
    int height = contents.size() / width;
    int innerWidth = width - HeightfieldHeight::HEIGHT_EXTENSION;
    int innerHeight = height - HeightfieldHeight::HEIGHT_EXTENSION;
    int highestX = innerWidth + HeightfieldHeight::HEIGHT_BORDER;
    int highestZ = innerHeight + HeightfieldHeight::HEIGHT_BORDER;
    
    glm::quat inverseRotation = glm::inverse(getRotation());
    glm::vec3 inverseScale(innerWidth / getScale(),  numeric_limits<quint16>::max() / (getScale() * _aspectY),
        innerHeight / (getScale() * _aspectZ));
    glm::vec3 direction = end - start;
    glm::vec3 dir = inverseRotation * direction * inverseScale;
    glm::vec3 entry = inverseRotation * (start - getTranslation()) * inverseScale;
    
    float boundsDistance;
    if (!Box(glm::vec3(), glm::vec3((float)innerWidth, (float)numeric_limits<quint16>::max(),
            (float)innerHeight)).findRayIntersection(entry, dir, boundsDistance) || boundsDistance > 1.0f) {
        return false;
    }
    entry += dir * boundsDistance;
    
    const float DISTANCE_THRESHOLD = 0.001f;
    if (glm::abs(entry.x - 0.0f) < DISTANCE_THRESHOLD) {
        normal = getRotation() * glm::vec3(-1.0f, 0.0f, 0.0f);
        distance = boundsDistance;
        return true;
        
    } else if (glm::abs(entry.x - innerWidth) < DISTANCE_THRESHOLD) {
        normal = getRotation() * glm::vec3(1.0f, 0.0f, 0.0f);
        distance = boundsDistance;
        return true;
        
    } else if (glm::abs(entry.y - 0.0f) < DISTANCE_THRESHOLD) {
        normal = getRotation() * glm::vec3(0.0f, -1.0f, 0.0f);
        distance = boundsDistance;
        return true;
        
    } else if (glm::abs(entry.y - numeric_limits<quint16>::max()) < DISTANCE_THRESHOLD) {
        normal = getRotation() * glm::vec3(0.0f, 1.0f, 0.0f);
        distance = boundsDistance;
        return true;
        
    } else if (glm::abs(entry.z - 0.0f) < DISTANCE_THRESHOLD) {
        normal = getRotation() * glm::vec3(0.0f, 0.0f, -1.0f);
        distance = boundsDistance;
        return true;
        
    } else if (glm::abs(entry.z - innerHeight) < DISTANCE_THRESHOLD) {
        normal = getRotation() * glm::vec3(0.0f, 0.0f, 1.0f);
        distance = boundsDistance;
        return true;
    }
    
    entry.x += HeightfieldHeight::HEIGHT_BORDER;
    entry.z += HeightfieldHeight::HEIGHT_BORDER;
    glm::vec3 floors = glm::floor(entry);
    glm::vec3 ceils = glm::ceil(entry);
    if (floors.x == ceils.x) {
        if (dir.x > 0.0f) {
            ceils.x += 1.0f;
        } else {
            floors.x -= 1.0f;
        } 
    }
    if (floors.z == ceils.z) {
        if (dir.z > 0.0f) {
            ceils.z += 1.0f;
        } else {
            floors.z -= 1.0f;
        }
    }
    
    glm::vec3 normalScale(1.0f / (inverseScale.y * inverseScale.z), 1.0f / (inverseScale.x * inverseScale.z),
        1.0f / (inverseScale.x * inverseScale.y));
    
    bool withinBounds = true;
    float accumulatedDistance = boundsDistance;
    while (withinBounds && accumulatedDistance <= 1.0f) {
        // find the heights at the corners of the current cell
        int floorX = qMin(qMax((int)floors.x, HeightfieldHeight::HEIGHT_BORDER), highestX);
        int floorZ = qMin(qMax((int)floors.z, HeightfieldHeight::HEIGHT_BORDER), highestZ);
        int ceilX = qMin(qMax((int)ceils.x, HeightfieldHeight::HEIGHT_BORDER), highestX);
        int ceilZ = qMin(qMax((int)ceils.z, HeightfieldHeight::HEIGHT_BORDER), highestZ);
        float upperLeft = src[floorZ * width + floorX];
        float upperRight = src[floorZ * width + ceilX];
        float lowerLeft = src[ceilZ * width + floorX];
        float lowerRight = src[ceilZ * width + ceilX];
        
        // find the distance to the next x coordinate
        float xDistance = FLT_MAX;
        if (dir.x > 0.0f) {
            xDistance = (ceils.x - entry.x) / dir.x;
        } else if (dir.x < 0.0f) {
            xDistance = (floors.x - entry.x) / dir.x;
        }
        
        // and the distance to the next z coordinate
        float zDistance = FLT_MAX;
        if (dir.z > 0.0f) {
            zDistance = (ceils.z - entry.z) / dir.z;
        } else if (dir.z < 0.0f) {
            zDistance = (floors.z - entry.z) / dir.z;
        }
        
        // the exit distance is the lower of those two
        float exitDistance = qMin(xDistance, zDistance);
        glm::vec3 exit, nextFloors = floors, nextCeils = ceils;
        if (exitDistance == FLT_MAX) {
            withinBounds = false; // line points upwards/downwards; check this cell only
            
        } else {
            // find the exit point and the next cell, and determine whether it's still within the bounds
            exit = entry + exitDistance * dir;
            withinBounds = (exit.y >= 0.0f && exit.y <= numeric_limits<quint16>::max());
            if (exitDistance == xDistance) {
                if (dir.x > 0.0f) {
                    nextFloors.x += 1.0f;
                    withinBounds &= (nextCeils.x += 1.0f) <= highestX;
                } else {
                    withinBounds &= (nextFloors.x -= 1.0f) >= HeightfieldHeight::HEIGHT_BORDER;
                    nextCeils.x -= 1.0f;
                }
            }
            if (exitDistance == zDistance) {
                if (dir.z > 0.0f) {
                    nextFloors.z += 1.0f;
                    withinBounds &= (nextCeils.z += 1.0f) <= highestZ;
                } else {
                    withinBounds &= (nextFloors.z -= 1.0f) >= HeightfieldHeight::HEIGHT_BORDER;
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
        float lowerProduct = glm::dot(lowerNormal, dir);
        if (lowerProduct != 0.0f) {
            float planeDistance = -glm::dot(lowerNormal, relativeEntry) / lowerProduct;
            glm::vec3 intersection = relativeEntry + planeDistance * dir;
            if (intersection.x >= 0.0f && intersection.x <= 1.0f && intersection.z >= 0.0f && intersection.z <= 1.0f &&
                    intersection.z >= intersection.x) {
                distance = accumulatedDistance + planeDistance;
                normal = glm::normalize(getRotation() * (lowerNormal * normalScale));
                return true;
            }
        }
        
        // then the one with the X+ segment
        glm::vec3 upperNormal(upperLeft - upperRight, 1.0f, upperRight - lowerRight);
        float upperProduct = glm::dot(upperNormal, dir);
        if (upperProduct != 0.0f) {
            float planeDistance = -glm::dot(upperNormal, relativeEntry) / upperProduct;
            glm::vec3 intersection = relativeEntry + planeDistance * dir;
            if (intersection.x >= 0.0f && intersection.x <= 1.0f && intersection.z >= 0.0f && intersection.z <= 1.0f &&
                    intersection.x >= intersection.z) {
                distance = accumulatedDistance + planeDistance;
                normal = glm::normalize(getRotation() * (upperNormal * normalScale));
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

void Heightfield::writeExtra(Bitstream& out) const {
    if (getWillBeVoxelized()) {
        out << _height << _color << _material << _stack;
        return;
    }
    MetavoxelLOD lod;
    if (out.getContext()) {
        lod = transformLOD(static_cast<MetavoxelStreamBase*>(out.getContext())->lod);
    }
    HeightfieldStreamBase base = { out, lod, lod };
    HeightfieldStreamState state = { base, glm::vec2(), 1.0f };
    _root->write(state);
}

void Heightfield::readExtra(Bitstream& in, bool reread) {
    if (getWillBeVoxelized()) {
        if (reread) {
            HeightfieldHeightPointer height;
            HeightfieldColorPointer color;
            HeightfieldMaterialPointer material;
            HeightfieldStackPointer stack;
            in >> height >> color >> material >> stack;
            
        } else {
            in >> _height >> _color >> _material >> _stack;
        }
        return;
    }
    MetavoxelLOD lod;
    if (in.getContext()) {
        lod = transformLOD(static_cast<MetavoxelStreamBase*>(in.getContext())->lod);
    }
    HeightfieldStreamBase base = { in, lod, lod };
    HeightfieldStreamState state = { base, glm::vec2(), 1.0f };
    
    HeightfieldNodePointer root(new HeightfieldNode());
    root->read(state);
    if (!reread) {
        setRoot(root);
    }
}

void Heightfield::writeExtraDelta(Bitstream& out, const SharedObject* reference) const {
    MetavoxelLOD lod, referenceLOD;
    if (out.getContext()) {
        MetavoxelStreamBase* base = static_cast<MetavoxelStreamBase*>(out.getContext());
        lod = transformLOD(base->lod);
        referenceLOD = transformLOD(base->referenceLOD);
    }
    HeightfieldStreamBase base = { out, lod, referenceLOD };
    HeightfieldStreamState state = { base, glm::vec2(), 1.0f };
    const HeightfieldNodePointer& referenceRoot = static_cast<const Heightfield*>(reference)->getRoot();
    if (_root == referenceRoot) {
        out << false;
        if (state.becameSubdivided()) {
            _root->writeSubdivision(state);
        }
    } else {
        out << true;
        _root->writeDelta(referenceRoot, state);
    }    
}

void Heightfield::readExtraDelta(Bitstream& in, const SharedObject* reference, bool reread) {
    MetavoxelLOD lod, referenceLOD;
    if (in.getContext()) {
        MetavoxelStreamBase* base = static_cast<MetavoxelStreamBase*>(in.getContext());
        lod = transformLOD(base->lod);
        referenceLOD = transformLOD(base->referenceLOD);
    }
    HeightfieldStreamBase base = { in, lod, referenceLOD };
    HeightfieldStreamState state = { base, glm::vec2(), 1.0f };
    
    bool changed;
    in >> changed;
    if (changed) {
        HeightfieldNodePointer root(new HeightfieldNode());
        root->readDelta(static_cast<const Heightfield*>(reference)->getRoot(), state);
        if (!reread) {
            setRoot(root);
        }
    } else if (state.becameSubdividedOrCollapsed()) {
        HeightfieldNodePointer root(_root->readSubdivision(state));
        if (!reread) {
            setRoot(root);
        }
    } else if (!reread) {
        setRoot(static_cast<const Heightfield*>(reference)->getRoot());
    }
}

void Heightfield::maybeWriteSubdivision(Bitstream& out) {
    MetavoxelLOD lod, referenceLOD;
    if (out.getContext()) {
        MetavoxelStreamBase* base = static_cast<MetavoxelStreamBase*>(out.getContext());
        lod = transformLOD(base->lod);
        referenceLOD = transformLOD(base->referenceLOD);
    }
    HeightfieldStreamBase base = { out, lod, referenceLOD };
    HeightfieldStreamState state = { base, glm::vec2(), 1.0f };
    
    if (state.becameSubdividedOrCollapsed()) {
        out << SharedObjectPointer(this);
        _root->writeSubdivision(state);
    }
}

SharedObject* Heightfield::readSubdivision(Bitstream& in) {
    MetavoxelLOD lod, referenceLOD;
    if (in.getContext()) {
        MetavoxelStreamBase* base = static_cast<MetavoxelStreamBase*>(in.getContext());
        lod = transformLOD(base->lod);
        referenceLOD = transformLOD(base->referenceLOD);
    }
    HeightfieldStreamBase base = { in, lod, referenceLOD };
    HeightfieldStreamState state = { base, glm::vec2(), 1.0f };
    
    if (state.becameSubdividedOrCollapsed()) {
        HeightfieldNodePointer root(_root->readSubdivision(state));
        if (_root != root) {
            Heightfield* newHeightfield = static_cast<Heightfield*>(clone(true));
            newHeightfield->setRemoteID(getRemoteID());
            newHeightfield->setRemoteOriginID(getRemoteOriginID());
            newHeightfield->setRoot(root);
            return newHeightfield;
        }
    }
    return this;
}

QByteArray Heightfield::getRendererClassName() const {
    return "HeightfieldRenderer";
}

void Heightfield::updateBounds() {
    glm::vec3 extent(getScale(), getScale() * _aspectY, getScale() * _aspectZ);
    glm::mat4 rotationMatrix = glm::mat4_cast(getRotation());
    setBounds(glm::translate(getTranslation()) * rotationMatrix * Box(glm::vec3(), extent));
}

void Heightfield::updateRoot() {
    HeightfieldNodePointer root(new HeightfieldNode());
    if (_height) {
        root->setContents(_height, _color, _material, _stack);
    }
    setRoot(root);
}

Heightfield* Heightfield::prepareEdit(float minimumValue, float maximumValue, float& normalizeScale, float& normalizeOffset) {
    // renormalize if necessary
    Heightfield* newHeightfield = static_cast<Heightfield*>(clone(true));
    if (minimumValue < 1.0f || maximumValue > numeric_limits<quint16>::max()) {
        normalizeScale = (numeric_limits<quint16>::max() - 1.0f) / (maximumValue - minimumValue);
        normalizeOffset = 1.0f - minimumValue;
        newHeightfield->setAspectY(_aspectY / normalizeScale);
        newHeightfield->setTranslation(getTranslation() - getRotation() *
            glm::vec3(0.0f, normalizeOffset * _aspectY * getScale() / (numeric_limits<quint16>::max() - 1), 0.0f));
    } else {
        normalizeScale = 1.0f;
        normalizeOffset = 0.0f;
    }
    return newHeightfield;
}

