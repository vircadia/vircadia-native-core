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
#include <QSettings>
#include <QThread>

#include <glm/gtx/transform.hpp>

#include <GeometryUtil.h>

#include "MetavoxelData.h"
#include "Spanner.h"

using namespace std;

REGISTER_META_OBJECT(Spanner)
REGISTER_META_OBJECT(Heightfield)
REGISTER_META_OBJECT(Sphere)
REGISTER_META_OBJECT(Cuboid)
REGISTER_META_OBJECT(StaticModel)

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

Spanner* Spanner::paintMaterial(const glm::vec3& position, float radius, const SharedObjectPointer& material,
        const QColor& color) {
    return this;
}

Spanner* Spanner::paintHeight(const glm::vec3& position, float radius, float height) {
    return this;
}

Spanner* Spanner::clearAndFetchHeight(const Box& bounds, SharedObjectPointer& heightfield) {
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
    QSettings settings;
    QString result = QFileDialog::getOpenFileName(this, "Select Height Image", settings.value("heightDir").toString(),
        "Images (*.png *.jpg *.bmp *.raw *.mdr)");
    if (result.isNull()) {
        return;
    }
    settings.setValue("heightDir", QFileInfo(result).path());
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
    image = image.convertToFormat(QImage::Format_RGB888);
    int width = getHeightfieldSize(image.width()) + 2 * HeightfieldHeight::HEIGHT_BORDER;
    int height = getHeightfieldSize(image.height()) + 2 * HeightfieldHeight::HEIGHT_BORDER;
    QVector<quint16> contents(width * height);
    quint16* dest = contents.data() + (width + 1) * HeightfieldHeight::HEIGHT_BORDER;
    const float CONVERSION_SCALE = 65534.0f / numeric_limits<quint8>::max();
    for (int i = 0; i < image.height(); i++, dest += width) {
        const uchar* src = image.constScanLine(i);
        for (quint16* lineDest = dest, *end = dest + image.width(); lineDest != end; lineDest++,
                src += DataBlock::COLOR_BYTES) {
            *lineDest = (quint16)(*src * CONVERSION_SCALE) + CONVERSION_OFFSET;
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
    QByteArray inflated(HEIGHTFIELD_DATA_HEADER_SIZE + contents.size(), 0);
    qint32* header = (qint32*)inflated.data();
    *header++ = offsetX;
    *header++ = offsetY;
    *header++ = width;
    *header++ = height;
    if (!contents.isEmpty()) {
        // encode with Paeth filter (see http://en.wikipedia.org/wiki/Portable_Network_Graphics#Filtering)
        const uchar* src = (const uchar*)contents.constData();
        uchar* dest = (uchar*)inflated.data() + HEIGHTFIELD_DATA_HEADER_SIZE;
        *dest++ = *src++;
        *dest++ = *src++;
        *dest++ = *src++;
        int stride = width * DataBlock::COLOR_BYTES;
        for (uchar* end = dest + stride - DataBlock::COLOR_BYTES; dest != end; dest++, src++) {
            *dest = *src - src[-DataBlock::COLOR_BYTES];
        }
        for (int y = 1; y < height; y++) {
            *dest++ = *src - src[-stride];
            src++;
            *dest++ = *src - src[-stride];
            src++;
            *dest++ = *src - src[-stride];
            src++;
            for (uchar* end = dest + stride - DataBlock::COLOR_BYTES; dest != end; dest++, src++) {
                int a = src[-DataBlock::COLOR_BYTES];
                int b = src[-stride];
                int c = src[-stride - DataBlock::COLOR_BYTES];
                int p = a + b - c;
                int ad = abs(a - p);
                int bd = abs(b - p);
                int cd = abs(c - p);
                *dest = *src - (ad < bd ? (ad < cd ? a : c) : (bd < cd ? b : c));
            }
        }
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
    QByteArray contents(inflated.size() - HEIGHTFIELD_DATA_HEADER_SIZE, 0);
    if (!contents.isEmpty()) {
        const uchar* src = (const uchar*)inflated.constData() + HEIGHTFIELD_DATA_HEADER_SIZE;
        uchar* dest = (uchar*)contents.data();
        *dest++ = *src++;
        *dest++ = *src++;
        *dest++ = *src++;
        int stride = width * DataBlock::COLOR_BYTES;
        for (uchar* end = dest + stride - DataBlock::COLOR_BYTES; dest != end; dest++, src++) {
            *dest = *src + dest[-DataBlock::COLOR_BYTES];
        }
        for (int y = 1; y < height; y++) {
            *dest = (*src++) + dest[-stride];
            dest++;
            *dest = (*src++) + dest[-stride];
            dest++;
            *dest = (*src++) + dest[-stride];
            dest++;
            for (uchar* end = dest + stride - DataBlock::COLOR_BYTES; dest != end; dest++, src++) {
                int a = dest[-DataBlock::COLOR_BYTES];
                int b = dest[-stride];
                int c = dest[-stride - DataBlock::COLOR_BYTES];
                int p = a + b - c;
                int ad = abs(a - p);
                int bd = abs(b - p);
                int cd = abs(c - p);
                *dest = *src + (ad < bd ? (ad < cd ? a : c) : (bd < cd ? b : c));
            }
        }
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
    QSettings settings;
    QString result = QFileDialog::getOpenFileName(this, "Select Color Image", settings.value("heightDir").toString(),
        "Images (*.png *.jpg *.bmp)");
    if (result.isNull()) {
        return;
    }
    settings.setValue("heightDir", QFileInfo(result).path());
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

HeightfieldStack::HeightfieldStack(int width, const QVector<SharedObjectPointer>& materials) :
    HeightfieldData(width),
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
}

void HeightfieldStack::write(Bitstream& out) {
    out << _materials;
}

void HeightfieldStack::writeDelta(Bitstream& out, const HeightfieldStackPointer& reference) {
}

void HeightfieldStack::read(Bitstream& in, int bytes) {
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

float HeightfieldNode::getHeight(const glm::vec3& location) const {
    if (location.x < 0.0f || location.z < 0.0f || location.x > 1.0f || location.z > 1.0f) {
        return -FLT_MAX;
    }
    if (!isLeaf()) {
        if (location.x < 0.5f) {
            if (location.z < 0.5f) {
                return _children[0]->getHeight(location * 2.0f);
            } else {
                return _children[Y_MAXIMUM_FLAG]->getHeight(location * 2.0f - glm::vec3(0.0f, 0.0f, 1.0f));
            }
        } else {
            if (location.z < 0.5f) {
                return _children[X_MAXIMUM_FLAG]->getHeight(location * 2.0f - glm::vec3(1.0f, 0.0f, 0.0f));
            } else {
                return _children[X_MAXIMUM_FLAG | Y_MAXIMUM_FLAG]->getHeight(location * 2.0f - glm::vec3(1.0f, 0.0f, 1.0f));
            }
        }
    }
    if (!_height) {
        return -FLT_MAX;
    }
    int width = _height->getWidth();
    const QVector<quint16>& contents = _height->getContents();
    const quint16* src = contents.constData();
    int height = contents.size() / width;
    int innerWidth = width - HeightfieldHeight::HEIGHT_EXTENSION;
    int innerHeight = height - HeightfieldHeight::HEIGHT_EXTENSION;
    
    glm::vec3 relative = location;
    relative.x = relative.x * innerWidth + HeightfieldHeight::HEIGHT_BORDER;
    relative.z = relative.z * innerHeight + HeightfieldHeight::HEIGHT_BORDER;
    
    // find the bounds of the cell containing the point and the shared vertex heights
    glm::vec3 floors = glm::floor(relative);
    glm::vec3 ceils = glm::ceil(relative);
    glm::vec3 fracts = glm::fract(relative);
    int floorX = (int)floors.x;
    int floorZ = (int)floors.z;
    int ceilX = (int)ceils.x;
    int ceilZ = (int)ceils.z;
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
        return -FLT_MAX; // ignore zero values
    }
    return interpolatedHeight / numeric_limits<quint16>::max();
}

bool HeightfieldNode::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    float boundsDistance;
    if (!Box(glm::vec3(), glm::vec3(1.0f, 1.0f, 1.0f)).findRayIntersection(origin, direction, boundsDistance)) {
        return false;
    }
    if (!isLeaf()) {
        float closestDistance = FLT_MAX;
        for (int i = 0; i < CHILD_COUNT; i++) {
            float childDistance;
            if (_children[i]->findRayIntersection(origin * glm::vec3(2.0f, 1.0f, 2.0f) -
                    glm::vec3(i & X_MAXIMUM_FLAG ? 1.0f : 0.0f, 0.0f, i & Y_MAXIMUM_FLAG ? 1.0f : 0.0f),
                    direction * glm::vec3(2.0f, 1.0f, 2.0f), childDistance)) {
                closestDistance = qMin(closestDistance, childDistance);
            }
        }
        if (closestDistance == FLT_MAX) {
            return false;
        }
        distance = closestDistance;
        return true;
    }
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
    
    glm::vec3 scale((float)innerWidth, (float)numeric_limits<quint16>::max(), (float)innerHeight);
    glm::vec3 dir = direction * scale;
    glm::vec3 entry = origin * scale + dir * boundsDistance;
    
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

HeightfieldNode* HeightfieldNode::paintMaterial(const glm::vec3& position, const glm::vec3& radius,
        const SharedObjectPointer& material, const QColor& color) {
    if (position.x + radius.x < 0.0f || position.z + radius.z < 0.0f ||
            position.x - radius.x > 1.0f || position.z - radius.z > 1.0f) {
        return this;
    }
    if (!isLeaf()) {
        HeightfieldNode* newNode = this;
        for (int i = 0; i < CHILD_COUNT; i++) {
            HeightfieldNode* newChild = _children[i]->paintMaterial(position * glm::vec3(2.0f, 1.0f, 2.0f) -
                glm::vec3(i & X_MAXIMUM_FLAG ? 1.0f : 0.0f, 0.0f, i & Y_MAXIMUM_FLAG ? 1.0f : 0.0f),
                radius * glm::vec3(2.0f, 1.0f, 2.0f), material, color);
            if (_children[i] != newChild) {
                if (newNode == this) {
                    newNode = new HeightfieldNode(*this);
                }
                newNode->setChild(i, HeightfieldNodePointer(newChild));
            }
        }
        if (newNode != this) {
            newNode->mergeChildren(false, true);
        }
        return newNode;
    }
    if (!_height) {
        return this;
    }
    int heightWidth = _height->getWidth();
    int heightHeight = _height->getContents().size() / heightWidth;
    int baseWidth = heightWidth - HeightfieldHeight::HEIGHT_EXTENSION + HeightfieldData::SHARED_EDGE;
    int baseHeight = heightHeight - HeightfieldHeight::HEIGHT_EXTENSION + HeightfieldData::SHARED_EDGE;
    
    int colorWidth = baseWidth, colorHeight = baseHeight;
    QByteArray colorContents;
    if (_color) {
        colorWidth = _color->getWidth();
        colorHeight = _color->getContents().size() / (colorWidth * DataBlock::COLOR_BYTES);
        colorContents = _color->getContents();
        
    } else {
        colorContents = QByteArray(baseWidth * baseHeight * DataBlock::COLOR_BYTES, 0xFF); 
    }
    
    int materialWidth = baseWidth, materialHeight = baseHeight;
    QByteArray materialContents;
    QVector<SharedObjectPointer> materials;
    if (_material) {
        materialWidth = _material->getWidth();
        materialHeight = _material->getContents().size() / materialWidth;
        materialContents = _material->getContents();
        materials = _material->getMaterials();
        
    } else {
        materialContents = QByteArray(baseWidth * baseHeight, 0);
    }
    
    int highestX = colorWidth - 1;
    int highestZ = colorHeight - 1;
    glm::vec3 scale((float)highestX, 1.0f, (float)highestZ);
    glm::vec3 center = position * scale;
    
    glm::vec3 extents = radius * scale;
    glm::vec3 start = glm::floor(center - extents);
    glm::vec3 end = glm::ceil(center + extents);
    
    // paint all points within the radius
    float z = qMax(start.z, 0.0f);
    float startX = qMax(start.x, 0.0f), endX = qMin(end.x, (float)highestX);
    int stride = colorWidth * DataBlock::COLOR_BYTES;
    uchar* lineDest = (uchar*)colorContents.data() + (int)z * stride + (int)startX * DataBlock::COLOR_BYTES;
    float squaredRadius = extents.x * extents.x;
    float multiplierZ = extents.x / extents.z;
    char red = color.red(), green = color.green(), blue = color.blue();
    bool changed = false;
    for (float endZ = qMin(end.z, (float)highestZ); z <= endZ; z += 1.0f) {
        uchar* dest = lineDest;
        for (float x = startX; x <= endX; x += 1.0f, dest += DataBlock::COLOR_BYTES) {
            float dx = x - center.x, dz = (z - center.z) * multiplierZ;
            if (dx * dx + dz * dz <= squaredRadius) {
                dest[0] = red;
                dest[1] = green;
                dest[2] = blue;
                changed = true;
            }
        }
        lineDest += stride;
    }
    HeightfieldNode* newNode = this;    
    if (changed) {
        newNode = new HeightfieldNode(*this);
        newNode->setColor(HeightfieldColorPointer(new HeightfieldColor(colorWidth, colorContents)));
    }
    
    highestX = materialWidth - 1;
    highestZ = materialHeight - 1;
    scale = glm::vec3((float)highestX, 1.0f, (float)highestZ);
    center = position * scale;
    
    extents = radius * scale;
    start = glm::floor(center - extents);
    end = glm::ceil(center + extents);
     
    // paint all points within the radius
    z = qMax(start.z, 0.0f);
    startX = qMax(start.x, 0.0f), endX = qMin(end.x, (float)highestX);
    lineDest = (uchar*)materialContents.data() + (int)z * materialWidth + (int)startX;
    squaredRadius = extents.x * extents.x;
    multiplierZ = extents.x / extents.z;
    uchar materialIndex = getMaterialIndex(material, materials, materialContents);
    changed = false;
    for (float endZ = qMin(end.z, (float)highestZ); z <= endZ; z += 1.0f) {
        uchar* dest = lineDest;
        for (float x = startX; x <= endX; x += 1.0f, dest++) {
            float dx = x - center.x, dz = (z - center.z) * multiplierZ;
            if (dx * dx + dz * dz <= squaredRadius) {
                *dest = materialIndex;
                changed = true;
            }
        }
        lineDest += materialWidth;
    }
    if (changed) {
        if (newNode == this) {
            newNode = new HeightfieldNode(*this);
        }
        clearUnusedMaterials(materials, materialContents);
        newNode->setMaterial(HeightfieldMaterialPointer(new HeightfieldMaterial(materialWidth,
            materialContents, materials)));
    }
    
    return newNode;
}

void HeightfieldNode::getRangeAfterHeightPaint(const glm::vec3& position, const glm::vec3& radius,
        float height, int& minimum, int& maximum) const {
    if (position.x + radius.x < 0.0f || position.z + radius.z < 0.0f ||
            position.x - radius.x > 1.0f || position.z - radius.z > 1.0f) {
        return;
    }
    if (!isLeaf()) {
        for (int i = 0; i < CHILD_COUNT; i++) {
            _children[i]->getRangeAfterHeightPaint(position * glm::vec3(2.0f, 1.0f, 2.0f) -
                glm::vec3(i & X_MAXIMUM_FLAG ? 1.0f : 0.0f, 0.0f, i & Y_MAXIMUM_FLAG ? 1.0f : 0.0f),
                radius * glm::vec3(2.0f, 1.0f, 2.0f), height, minimum, maximum);
        }
        return;
    }
    if (!_height) {
        return;
    }
    int heightWidth = _height->getWidth();
    int heightHeight = _height->getContents().size() / heightWidth;
    QVector<quint16> contents = _height->getContents();
    int innerWidth = heightWidth - HeightfieldHeight::HEIGHT_EXTENSION;
    int innerHeight = heightHeight - HeightfieldHeight::HEIGHT_EXTENSION;
    int highestX = heightWidth - 1;
    int highestZ = heightHeight - 1;
    
    glm::vec3 scale((float)innerWidth, 1.0f, (float)innerHeight);
    glm::vec3 center = position * scale;
    center.x += 1.0f;
    center.z += 1.0f;
    
    glm::vec3 extents = radius * scale;
    glm::vec3 start = glm::floor(center - extents);
    glm::vec3 end = glm::ceil(center + extents);
    
    // first see if we're going to exceed the range limits
    float z = qMax(start.z, 0.0f);
    float startX = qMax(start.x, 0.0f), endX = qMin(end.x, (float)highestX);
    quint16* lineDest = contents.data() + (int)z * heightWidth + (int)startX;
    float squaredRadius = extents.x * extents.x;
    float squaredRadiusReciprocal = 1.0f / squaredRadius;
    float multiplierZ = extents.x / extents.z;
    for (float endZ = qMin(end.z, (float)highestZ); z <= endZ; z += 1.0f) {
        quint16* dest = lineDest;
        for (float x = startX; x <= endX; x += 1.0f, dest++) {
            float dx = x - center.x, dz = (z - center.z) * multiplierZ;
            float distanceSquared = dx * dx + dz * dz;
            if (distanceSquared <= squaredRadius) {
                // height falls off towards edges
                int value = *dest;
                if (value != 0) {
                    value += height * (squaredRadius - distanceSquared) * squaredRadiusReciprocal;
                    minimum = qMin(minimum, value);
                    maximum = qMax(maximum, value);
                }
            }
        }
        lineDest += heightWidth;
    }
}

HeightfieldNode* HeightfieldNode::paintHeight(const glm::vec3& position, const glm::vec3& radius,
        float height, float normalizeScale, float normalizeOffset) {
    if ((position.x + radius.x < 0.0f || position.z + radius.z < 0.0f || position.x - radius.x > 1.0f ||
            position.z - radius.z > 1.0f) && normalizeScale == 1.0f && normalizeOffset == 0.0f) {
        return this;
    }
    if (!isLeaf()) {
        HeightfieldNode* newNode = this;
        for (int i = 0; i < CHILD_COUNT; i++) {
            HeightfieldNode* newChild = _children[i]->paintHeight(position * glm::vec3(2.0f, 1.0f, 2.0f) -
                glm::vec3(i & X_MAXIMUM_FLAG ? 1.0f : 0.0f, 0.0f, i & Y_MAXIMUM_FLAG ? 1.0f : 0.0f),
                radius * glm::vec3(2.0f, 1.0f, 2.0f), height, normalizeScale, normalizeOffset);
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
    if (!_height) {
        return this;
    }
    int heightWidth = _height->getWidth();
    int heightHeight = _height->getContents().size() / heightWidth;
    QVector<quint16> contents = _height->getContents();
    int innerWidth = heightWidth - HeightfieldHeight::HEIGHT_EXTENSION;
    int innerHeight = heightHeight - HeightfieldHeight::HEIGHT_EXTENSION;
    int highestX = heightWidth - 1;
    int highestZ = heightHeight - 1;
    
    glm::vec3 scale((float)innerWidth, 1.0f, (float)innerHeight);
    glm::vec3 center = position * scale;
    center.x += 1.0f;
    center.z += 1.0f;
    
    glm::vec3 extents = radius * scale;
    glm::vec3 start = glm::floor(center - extents);
    glm::vec3 end = glm::ceil(center + extents);
    
    // renormalize if necessary
    bool changed = false;
    if (normalizeScale != 1.0f || normalizeOffset != 0.0f) {
        changed = true;
        for (quint16* dest = contents.data(), *end = contents.data() + contents.size(); dest != end; dest++) {
            int value = *dest;
            if (value != 0) {
                *dest = (value + normalizeOffset) * normalizeScale;
            }
        }
    }
    
    // now apply the actual change
    float z = qMax(start.z, 0.0f);
    float startX = qMax(start.x, 0.0f), endX = qMin(end.x, (float)highestX);
    quint16* lineDest = contents.data() + (int)z * heightWidth + (int)startX;
    float squaredRadius = extents.x * extents.x;
    float squaredRadiusReciprocal = 1.0f / squaredRadius;
    float multiplierZ = extents.x / extents.z;
    for (float endZ = qMin(end.z, (float)highestZ); z <= endZ; z += 1.0f) {
        quint16* dest = lineDest;
        for (float x = startX; x <= endX; x += 1.0f, dest++) {
            float dx = x - center.x, dz = (z - center.z) * multiplierZ;
            float distanceSquared = dx * dx + dz * dz;
            if (distanceSquared <= squaredRadius) {
                // height falls off towards edges
                int value = *dest;
                if (value != 0) {
                    *dest = value + height * (squaredRadius - distanceSquared) * squaredRadiusReciprocal;
                    changed = true;
                }
            }
        }
        lineDest += heightWidth;
    }
    if (!changed) {
        return this;
    }
    HeightfieldNode* newNode = new HeightfieldNode(*this);
    newNode->setHeight(HeightfieldHeightPointer(new HeightfieldHeight(heightWidth, contents)));
    return newNode;
}

HeightfieldNode* HeightfieldNode::clearAndFetchHeight(const glm::vec3& translation, const glm::quat& rotation,
        const glm::vec3& scale, const Box& bounds, SharedObjectPointer& heightfield) {
    Box nodeBounds = glm::translate(translation) * glm::mat4_cast(rotation) * Box(glm::vec3(), scale);
    if (!nodeBounds.intersects(bounds)) {
        return this;
    }
    if (!isLeaf()) {
        HeightfieldNode* newNode = this;
        for (int i = 0; i < CHILD_COUNT; i++) {
            glm::vec3 nextScale = scale * glm::vec3(0.5f, 1.0f, 0.5f);
            HeightfieldNode* newChild = _children[i]->clearAndFetchHeight(translation +
                rotation * glm::vec3(i & X_MAXIMUM_FLAG ? nextScale.x : 0.0f, 0.0f,
                    i & Y_MAXIMUM_FLAG ? nextScale.z : 0.0f), rotation,
                nextScale, bounds, heightfield);
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
    if (!_height) {
        return this;
    }
    int heightWidth = _height->getWidth();
    int heightHeight = _height->getContents().size() / heightWidth;
    int innerHeightWidth = heightWidth - HeightfieldHeight::HEIGHT_EXTENSION;
    int innerHeightHeight = heightHeight - HeightfieldHeight::HEIGHT_EXTENSION;    
    float heightIncrementX = scale.x / innerHeightWidth;
    float heightIncrementZ = scale.z / innerHeightHeight;
    
    int colorWidth = heightWidth;
    int colorHeight = heightHeight;
    if (_color) {
        colorWidth = _color->getWidth();
        colorHeight = _color->getContents().size() / (colorWidth * DataBlock::COLOR_BYTES);
    }
    int innerColorWidth = colorWidth - HeightfieldData::SHARED_EDGE;
    int innerColorHeight = colorHeight - HeightfieldData::SHARED_EDGE;
    float colorIncrementX = scale.x / innerColorWidth;
    float colorIncrementZ = scale.z / innerColorHeight;
    
    int materialWidth = colorWidth;
    int materialHeight = colorHeight;
    if (_material) {
        materialWidth = _material->getWidth();
        materialHeight = _material->getContents().size() / materialWidth;
    }
    int innerMaterialWidth = materialWidth - HeightfieldData::SHARED_EDGE;
    int innerMaterialHeight = materialHeight - HeightfieldData::SHARED_EDGE;
    float materialIncrementX = scale.x / innerMaterialWidth;
    float materialIncrementZ = scale.z / innerMaterialHeight;
    
    float largestIncrementX = qMax(heightIncrementX, qMax(colorIncrementX, materialIncrementX));
    float largestIncrementZ = qMax(heightIncrementZ, qMax(colorIncrementZ, materialIncrementZ));

    glm::vec3 minimum(glm::floor(bounds.minimum.x / largestIncrementX) * largestIncrementX, nodeBounds.minimum.y,
        glm::floor(bounds.minimum.z / largestIncrementZ) * largestIncrementZ);
    glm::vec3 maximum(glm::ceil(bounds.maximum.x / largestIncrementX) * largestIncrementX, nodeBounds.maximum.y,
        glm::ceil(bounds.maximum.z / largestIncrementZ) * largestIncrementZ);
    Box largestBounds(minimum, maximum);
    
    // enlarge the area to fetch
    minimum.x -= largestIncrementX;
    maximum.x += largestIncrementX;
    minimum.z -= largestIncrementZ;
    maximum.z += largestIncrementX;
    
    glm::mat4 baseTransform = glm::mat4_cast(glm::inverse(rotation)) * glm::translate(-translation);
    glm::vec3 inverseScale(innerHeightWidth / scale.x, 1.0f, innerHeightHeight / scale.z);
    glm::mat4 transform = glm::scale(inverseScale) * baseTransform;
    Box transformedBounds = transform * largestBounds;
    
    // make sure there are values to clear
    int startX = glm::clamp((int)glm::ceil(transformedBounds.minimum.x) + HeightfieldHeight::HEIGHT_BORDER,
        0, heightWidth - 1);
    int startZ = glm::clamp((int)glm::ceil(transformedBounds.minimum.z) + HeightfieldHeight::HEIGHT_BORDER,
        0, heightHeight - 1);
    int endX = glm::clamp((int)glm::floor(transformedBounds.maximum.x) + HeightfieldHeight::HEIGHT_BORDER, 0, heightWidth - 1);
    int endZ = glm::clamp((int)glm::floor(transformedBounds.maximum.z) + HeightfieldHeight::HEIGHT_BORDER,
        0, heightHeight - 1);
    const quint16* src = _height->getContents().constData() + startZ * heightWidth + startX;
    for (int z = startZ; z <= endZ; z++, src += heightWidth) {
        const quint16* lineSrc = src;
        for (int x = startX; x <= endX; x++) {
            if (*lineSrc++ != 0) {
                goto clearableBreak;
            }
        }
    }
    return this;
    clearableBreak:
    
    int spannerHeightWidth = (int)((maximum.x - minimum.x) / heightIncrementX) + HeightfieldHeight::HEIGHT_EXTENSION;
    int spannerHeightHeight = (int)((maximum.z - minimum.z) / heightIncrementZ) + HeightfieldHeight::HEIGHT_EXTENSION;   
    int spannerColorWidth = (int)((maximum.x - minimum.x) / colorIncrementX) + HeightfieldData::SHARED_EDGE;
    int spannerColorHeight = (int)((maximum.z - minimum.z) / colorIncrementZ) + HeightfieldData::SHARED_EDGE;
    int spannerMaterialWidth = (int)((maximum.x - minimum.x) / materialIncrementX) + HeightfieldData::SHARED_EDGE;
    int spannerMaterialHeight = (int)((maximum.z - minimum.z) / materialIncrementZ) + HeightfieldData::SHARED_EDGE;
        
    // create heightfield if necessary
    Heightfield* spanner = static_cast<Heightfield*>(heightfield.data());
    if (!spanner) {
        heightfield = spanner = new Heightfield();
        spanner->setTranslation(minimum);
        spanner->setScale(maximum.x - minimum.x);
        spanner->setAspectY((maximum.y - minimum.y) / spanner->getScale());
        spanner->setAspectZ((maximum.z - minimum.z) / spanner->getScale());
        spanner->setHeight(HeightfieldHeightPointer(new HeightfieldHeight(spannerHeightWidth,
            QVector<quint16>(spannerHeightWidth * spannerHeightHeight))));
        spanner->setColor(HeightfieldColorPointer(new HeightfieldColor(spannerColorWidth,
            QByteArray(spannerColorWidth * spannerColorHeight * DataBlock::COLOR_BYTES, 0xFF))));
        spanner->setMaterial(HeightfieldMaterialPointer(new HeightfieldMaterial(spannerMaterialWidth,
            QByteArray(spannerMaterialWidth * spannerMaterialHeight, 0), QVector<SharedObjectPointer>())));
    }
    
    // fetch the height
    glm::vec3 spannerInverseScale((spannerHeightWidth - HeightfieldHeight::HEIGHT_EXTENSION) / spanner->getScale(), 1.0f,
        (spannerHeightHeight - HeightfieldHeight::HEIGHT_EXTENSION) / (spanner->getScale() * spanner->getAspectZ()));
    glm::mat4 spannerBaseTransform = glm::translate(-spanner->getTranslation());
    glm::mat4 spannerTransform = glm::scale(spannerInverseScale) * spannerBaseTransform;
    Box spannerTransformedBounds = spannerTransform * nodeBounds;
    int spannerStartX = glm::clamp((int)glm::floor(spannerTransformedBounds.minimum.x) + HeightfieldHeight::HEIGHT_BORDER,
        0, spannerHeightWidth - 1);
    int spannerStartZ = glm::clamp((int)glm::floor(spannerTransformedBounds.minimum.z) + HeightfieldHeight::HEIGHT_BORDER,
        0, spannerHeightHeight - 1);
    int spannerEndX = glm::clamp((int)glm::ceil(spannerTransformedBounds.maximum.x) + HeightfieldHeight::HEIGHT_BORDER,
        0, spannerHeightWidth - 1);
    int spannerEndZ = glm::clamp((int)glm::ceil(spannerTransformedBounds.maximum.z) + HeightfieldHeight::HEIGHT_BORDER,
        0, spannerHeightHeight - 1);
    quint16* dest = spanner->getHeight()->getContents().data() + spannerStartZ * spannerHeightWidth + spannerStartX;
    glm::vec3 step = 1.0f / spannerInverseScale;
    glm::vec3 initialPosition = glm::inverse(rotation) * (glm::vec3(spannerStartX - HeightfieldHeight::HEIGHT_BORDER, 0,
        spannerStartZ - HeightfieldHeight::HEIGHT_BORDER) * step + spanner->getTranslation() - translation) / scale;
    glm::vec3 position = initialPosition;
    step = glm::inverse(rotation) * step / scale;
    float heightScale = numeric_limits<quint16>::max();
    for (int z = spannerStartZ; z <= spannerEndZ; z++, dest += spannerHeightWidth, position.z += step.z) {
        quint16* lineDest = dest;
        position.x = initialPosition.x;
        for (int x = spannerStartX; x <= spannerEndX; x++, lineDest++, position.x += step.x) {
            float height = getHeight(position) * heightScale;
            if (height > *lineDest) {
                *lineDest = height;
            }
        }
    }
    
    // and the color
    if (_color) {
        spannerInverseScale = glm::vec3((spannerColorWidth - HeightfieldData::SHARED_EDGE) / spanner->getScale(), 1.0f,
            (spannerColorHeight - HeightfieldData::SHARED_EDGE) / (spanner->getScale() * spanner->getAspectZ()));
        spannerTransform = glm::scale(spannerInverseScale) * spannerBaseTransform;
        spannerTransformedBounds = spannerTransform * nodeBounds;
        spannerStartX = glm::clamp((int)glm::floor(spannerTransformedBounds.minimum.x), 0, spannerColorWidth - 1);
        spannerStartZ = glm::clamp((int)glm::floor(spannerTransformedBounds.minimum.z), 0, spannerColorHeight - 1);
        spannerEndX = glm::clamp((int)glm::ceil(spannerTransformedBounds.maximum.x), 0, spannerColorWidth - 1);
        spannerEndZ = glm::clamp((int)glm::ceil(spannerTransformedBounds.maximum.z), 0, spannerColorHeight - 1);
        
        char* dest = spanner->getColor()->getContents().data() +
            (spannerStartZ * spannerColorWidth + spannerStartX) * DataBlock::COLOR_BYTES;
        step = 1.0f / spannerInverseScale;
        initialPosition = glm::inverse(rotation) * (glm::vec3(spannerStartX, 0, spannerStartZ) * step +
            spanner->getTranslation() - translation) / scale;
        position = initialPosition;
        step = glm::inverse(rotation) * step / scale;
        for (int z = spannerStartZ; z <= spannerEndZ; z++, dest += spannerColorWidth * DataBlock::COLOR_BYTES,
                position.z += step.z) {
            char* lineDest = dest;
            position.x = initialPosition.x;
            for (int x = spannerStartX; x <= spannerEndX; x++, lineDest += DataBlock::COLOR_BYTES, position.x += step.x) {
                QRgb color = getColorAt(position);
                if (color != 0) {
                    lineDest[0] = qRed(color);
                    lineDest[1] = qGreen(color);
                    lineDest[2] = qBlue(color);
                }
            }
        }
    }
    
    // and the material
    if (_material) {
        spannerInverseScale = glm::vec3((spannerMaterialWidth - HeightfieldData::SHARED_EDGE) / spanner->getScale(), 1.0f,
            (spannerMaterialHeight - HeightfieldData::SHARED_EDGE) / (spanner->getScale() * spanner->getAspectZ()));
        spannerTransform = glm::scale(spannerInverseScale) * spannerBaseTransform;
        spannerTransformedBounds = spannerTransform * nodeBounds;
        spannerStartX = glm::clamp((int)glm::floor(spannerTransformedBounds.minimum.x), 0, spannerMaterialWidth - 1);
        spannerStartZ = glm::clamp((int)glm::floor(spannerTransformedBounds.minimum.z), 0, spannerMaterialHeight - 1);
        spannerEndX = glm::clamp((int)glm::ceil(spannerTransformedBounds.maximum.x), 0, spannerMaterialWidth - 1);
        spannerEndZ = glm::clamp((int)glm::ceil(spannerTransformedBounds.maximum.z), 0, spannerMaterialHeight - 1);
        
        char* dest = spanner->getMaterial()->getContents().data() + spannerStartZ * spannerMaterialWidth + spannerStartX;
        step = 1.0f / spannerInverseScale;
        initialPosition = glm::inverse(rotation) * (glm::vec3(spannerStartX, 0, spannerStartZ) * step +
            spanner->getTranslation() - translation) / scale;
        position = initialPosition;
        step = glm::inverse(rotation) * step / scale;
        QHash<int, int> materialMap;
        for (int z = spannerStartZ; z <= spannerEndZ; z++, dest += spannerMaterialWidth, position.z += step.z) {
            char* lineDest = dest;
            position.x = initialPosition.x;
            for (int x = spannerStartX; x <= spannerEndX; x++, lineDest++, position.x += step.x) {
                int material = getMaterialAt(position);
                if (material != -1) {
                    if (material != 0) {
                        int& mapping = materialMap[material];
                        if (mapping == 0) {
                            material = mapping = getMaterialIndex(_material->getMaterials().at(material - 1),
                                spanner->getMaterial()->getMaterials(), spanner->getMaterial()->getContents());
                        }
                    }
                    *lineDest = material;
                }
            }
        }
    }
    
    // clear the height
    QVector<quint16> newHeightContents = _height->getContents();
    dest = newHeightContents.data() + startZ * heightWidth + startX;
    for (int z = startZ; z <= endZ; z++, dest += heightWidth) {
        memset(dest, 0, (endX - startX + 1) * sizeof(quint16));
    }
    
    HeightfieldNode* newNode = new HeightfieldNode();
    newNode->setHeight(HeightfieldHeightPointer(new HeightfieldHeight(heightWidth, newHeightContents)));
    
    // and the color
    if (_color) {
        inverseScale = glm::vec3(innerColorWidth / scale.x, 1.0f, innerColorHeight / scale.z);
        transform = glm::scale(inverseScale) * baseTransform;
        transformedBounds = transform * largestBounds;
        startX = glm::clamp((int)glm::ceil(transformedBounds.minimum.x), 0, colorWidth - 1);
        startZ = glm::clamp((int)glm::ceil(transformedBounds.minimum.z), 0, colorHeight - 1);
        endX = glm::clamp((int)glm::floor(transformedBounds.maximum.x), 0, colorWidth - 1);
        endZ = glm::clamp((int)glm::floor(transformedBounds.maximum.z), 0, colorHeight - 1);
        QByteArray newColorContents = _color->getContents();
        char* dest = newColorContents.data() + (startZ * colorWidth + startX) * DataBlock::COLOR_BYTES;
        for (int z = startZ; z <= endZ; z++, dest += colorWidth * DataBlock::COLOR_BYTES) {
            memset(dest, 0, (endX - startX + 1) * DataBlock::COLOR_BYTES);
        }
        newNode->setColor(HeightfieldColorPointer(new HeightfieldColor(colorWidth, newColorContents)));
    }
    
    // and the material
    if (_material) {
        inverseScale = glm::vec3(innerMaterialWidth / scale.x, 1.0f, innerMaterialHeight / scale.z);
        transform = glm::scale(inverseScale) * baseTransform;
        transformedBounds = transform * largestBounds;
        startX = glm::clamp((int)glm::ceil(transformedBounds.minimum.x), 0, materialWidth - 1);
        startZ = glm::clamp((int)glm::ceil(transformedBounds.minimum.z), 0, materialHeight - 1);
        endX = glm::clamp((int)glm::floor(transformedBounds.maximum.x), 0, materialWidth - 1);
        endZ = glm::clamp((int)glm::floor(transformedBounds.maximum.z), 0, materialHeight - 1);
        QByteArray newMaterialContents = _material->getContents();
        QVector<SharedObjectPointer> newMaterials = _material->getMaterials();
        char* dest = newMaterialContents.data() + startZ * materialWidth + startX;
        for (int z = startZ; z <= endZ; z++, dest += materialWidth) {
            memset(dest, 0, endX - startX + 1);
        }
        clearUnusedMaterials(newMaterials, newMaterialContents);
        newNode->setMaterial(HeightfieldMaterialPointer(new HeightfieldMaterial(
            materialWidth, newMaterialContents, newMaterials)));
    }
    
    return newNode;
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
        state.base.stream.writeDelta(_material, reference->getStack());
        
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
    }
    if (heightWidth > 0 && height) {
        QVector<quint16> heightContents(heightWidth * heightHeight);
        for (int i = 0; i < CHILD_COUNT; i++) {
            HeightfieldHeightPointer childHeight = _children[i]->getHeight();
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
    if (!colorMaterial) {
        return;
    }
    if (colorWidth > 0) {
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
    if (materialWidth > 0) {
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

AbstractHeightfieldNodeRenderer::~AbstractHeightfieldNodeRenderer() {
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
    float result = _root->getHeight(glm::inverse(getRotation()) * (location - getTranslation()) * glm::vec3(1.0f / getScale(),
        0.0f, 1.0f / (getScale() * _aspectZ)));
    return (result == -FLT_MAX) ? -FLT_MAX : (getTranslation().y + result * getScale() * _aspectY);
}

bool Heightfield::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    glm::quat inverseRotation = glm::inverse(getRotation());
    glm::vec3 inverseScale(1.0f / getScale(),  1.0f / (getScale() * _aspectY), 1.0f / (getScale() * _aspectZ));
    return _root->findRayIntersection(inverseRotation * (origin - getTranslation()) * inverseScale,
        inverseRotation * direction * inverseScale, distance);
}

Spanner* Heightfield::paintMaterial(const glm::vec3& position, float radius,
        const SharedObjectPointer& material, const QColor& color) {
    glm::vec3 inverseScale(1.0f / getScale(), 1.0f, 1.0f / (getScale() * _aspectZ));
    HeightfieldNode* newRoot = _root->paintMaterial(glm::inverse(getRotation()) * (position - getTranslation()) *
        inverseScale, radius * inverseScale, material, color);
    if (_root == newRoot) {
        return this;
    }
    Heightfield* newHeightfield = static_cast<Heightfield*>(clone(true));
    newHeightfield->setRoot(HeightfieldNodePointer(newRoot));
    return newHeightfield;
}

Spanner* Heightfield::paintHeight(const glm::vec3& position, float radius, float height) {
    // first see if we're going to exceed the range limits
    glm::vec3 inverseScale(1.0f / getScale(), 1.0f, 1.0f / (getScale() * _aspectZ));
    glm::vec3 relativePosition = glm::inverse(getRotation()) * (position - getTranslation()) * inverseScale;
    glm::vec3 relativeRadius = radius * inverseScale;
    int minimumValue = 1, maximumValue = numeric_limits<quint16>::max();
    _root->getRangeAfterHeightPaint(relativePosition, relativeRadius,
        height * numeric_limits<quint16>::max() / (getScale() * _aspectY), minimumValue, maximumValue);

    // renormalize if necessary
    Heightfield* newHeightfield = static_cast<Heightfield*>(clone(true));
    float normalizeScale = 1.0f, normalizeOffset = 0.0f;
    if (minimumValue < 1 || maximumValue > numeric_limits<quint16>::max()) {
        normalizeScale = (numeric_limits<quint16>::max() - 1.0f) / (maximumValue - minimumValue);
        normalizeOffset = 1.0f - minimumValue;
        newHeightfield->setAspectY(_aspectY / normalizeScale);
        newHeightfield->setTranslation(getTranslation() - getRotation() *
            glm::vec3(0.0f, normalizeOffset * _aspectY * getScale() / (numeric_limits<quint16>::max() - 1), 0.0f));
    }
    
    // now apply the actual change
    newHeightfield->setRoot(HeightfieldNodePointer(_root->paintHeight(relativePosition, relativeRadius,
        height * numeric_limits<quint16>::max() / (getScale() * newHeightfield->getAspectY()),
        normalizeScale, normalizeOffset)));
    return newHeightfield;
}

Spanner* Heightfield::clearAndFetchHeight(const Box& bounds, SharedObjectPointer& heightfield) {
    HeightfieldNode* newRoot = _root->clearAndFetchHeight(getTranslation(), getRotation(),
        glm::vec3(getScale(), getScale() * _aspectY, getScale() * _aspectZ), bounds, heightfield);
    if (_root == newRoot) {
        return this;
    }
    Heightfield* newHeightfield = static_cast<Heightfield*>(clone(true));
    newHeightfield->setRoot(HeightfieldNodePointer(newRoot));
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

void Heightfield::readExtra(Bitstream& in) {
    if (getWillBeVoxelized()) {
        in >> _height >> _color >> _material >> _stack;
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
    setRoot(root);
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

void Heightfield::readExtraDelta(Bitstream& in, const SharedObject* reference) {
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
        setRoot(root);
    
    } else if (state.becameSubdividedOrCollapsed()) {
        setRoot(HeightfieldNodePointer(_root->readSubdivision(state)));
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
