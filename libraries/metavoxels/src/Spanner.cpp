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

#include <QFileDialog>
#include <QHBoxLayout>
#include <QItemEditorFactory>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QThread>

#include <glm/gtx/transform.hpp>

#include <GeometryUtil.h>

#include "Spanner.h"

using namespace std;

REGISTER_META_OBJECT(Spanner)
REGISTER_META_OBJECT(Sphere)
REGISTER_META_OBJECT(Cuboid)
REGISTER_META_OBJECT(StaticModel)
REGISTER_META_OBJECT(Heightfield)

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
    _merged(false) {
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

bool Spanner::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
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

void SpannerRenderer::render(const glm::vec4& color, Mode mode) {
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

const float EIGHT_BIT_MAXIMUM = 255.0f;

TempHeightfield::TempHeightfield(const Box& bounds, float increment, const QByteArray& height, const QByteArray& color,
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

bool TempHeightfield::hasOwnColors() const {
    return true;
}

bool TempHeightfield::hasOwnMaterials() const {
    return true;
}

QRgb TempHeightfield::getColorAt(const glm::vec3& point) {
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

int TempHeightfield::getMaterialAt(const glm::vec3& point) {
    glm::vec3 relative = (point - getBounds().minimum) / _increment;
    const uchar* src = (const uchar*)_material.constData();
    return src[(int)glm::round(relative.z) * _width + (int)glm::round(relative.x)];
}

QVector<SharedObjectPointer>& TempHeightfield::getMaterials() {
    return _materials;
}

bool TempHeightfield::contains(const glm::vec3& point) {
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

bool TempHeightfield::intersects(const glm::vec3& start, const glm::vec3& end, float& distance, glm::vec3& normal) {
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

HeightfieldData::HeightfieldData(int width) :
    _width(width) {
}

HeightfieldHeight::HeightfieldHeight(int width, const QVector<quint16>& contents) :
    HeightfieldData(width),
    _contents(contents) {
}

HeightfieldHeight::HeightfieldHeight(Bitstream& in, int bytes) {
}

HeightfieldHeight::HeightfieldHeight(Bitstream& in, int bytes, const HeightfieldHeightPointer& reference) {
}

void HeightfieldHeight::write(Bitstream& out) {
    QMutexLocker locker(&_encodedMutex);
    if (_encoded.isEmpty()) {
        
    }
    out << _encoded.size();
    out.writeAligned(_encoded);
}

void HeightfieldHeight::writeDelta(Bitstream& out, const HeightfieldHeightPointer& reference) {
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
        value = new HeightfieldHeight(0, QVector<quint16>());
    }
    return in;
}

template<> void Bitstream::writeRawDelta(const HeightfieldHeightPointer& value, const HeightfieldHeightPointer& reference) {
}

template<> void Bitstream::readRawDelta(HeightfieldHeightPointer& value, const HeightfieldHeightPointer& reference) {
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
        "Images (*.png *.jpg *.bmp *.raw)");
    if (result.isNull()) {
        return;
    }
    settings.setValue("heightDir", QFileInfo(result).path());
    QImage image;
    if (!image.load(result)) {
        QMessageBox::warning(this, "Invalid Image", "The selected image could not be read.");
        return;
    }
    const quint16 CONVERSION_OFFSET = 1;
    if (result.toLower().endsWith(".raw")) {
        QFile input(result);
        input.open(QIODevice::ReadOnly);
        QDataStream in(&input);
        in.setByteOrder(QDataStream::LittleEndian);
        QVector<quint16> rawContents;
        while (!in.atEnd()) {
            quint16 height;
            in >> height;
            rawContents.append(height);
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
    image = image.convertToFormat(QImage::Format_RGB888);
    int width = getHeightfieldSize(image.width()) + 2 * HeightfieldHeight::HEIGHT_BORDER;
    int height = getHeightfieldSize(image.height()) + 2 * HeightfieldHeight::HEIGHT_BORDER;
    QVector<quint16> contents(width * height);
    quint16* dest = contents.data() + (width + 1) * HeightfieldHeight::HEIGHT_BORDER;
    const float CONVERSION_SCALE = 65534.0f / EIGHT_BIT_MAXIMUM;
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

HeightfieldColor::HeightfieldColor(int width, const QByteArray& contents) :
    HeightfieldData(width),
    _contents(contents) {
}

HeightfieldColor::HeightfieldColor(Bitstream& in, int bytes) {
}

HeightfieldColor::HeightfieldColor(Bitstream& in, int bytes, const HeightfieldColorPointer& reference) {
}

void HeightfieldColor::write(Bitstream& out) {
    QMutexLocker locker(&_encodedMutex);
    if (_encoded.isEmpty()) {
        
    }
    out << _encoded.size();
    out.writeAligned(_encoded);
}

void HeightfieldColor::writeDelta(Bitstream& out, const HeightfieldColorPointer& reference) {
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
        value = new HeightfieldColor(0, QByteArray());
    }
    return in;
}

template<> void Bitstream::writeRawDelta(const HeightfieldColorPointer& value, const HeightfieldColorPointer& reference) {
    
}

template<> void Bitstream::readRawDelta(HeightfieldColorPointer& value, const HeightfieldColorPointer& reference) {
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

HeightfieldMaterial::HeightfieldMaterial(int width, const QByteArray& contents,
        const QVector<SharedObjectPointer>& materials) :
    HeightfieldData(width),
    _contents(contents),
    _materials(materials) {
}

HeightfieldMaterial::HeightfieldMaterial(Bitstream& in, int bytes) {
}

HeightfieldMaterial::HeightfieldMaterial(Bitstream& in, int bytes, const HeightfieldMaterialPointer& reference) {
}

void HeightfieldMaterial::write(Bitstream& out) {
    QMutexLocker locker(&_encodedMutex);
    if (_encoded.isEmpty()) {
        
    }
    out << _encoded.size();
    out.writeAligned(_encoded);
    out << _materials;
}

void HeightfieldMaterial::writeDelta(Bitstream& out, const HeightfieldMaterialPointer& reference) {
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
        value = new HeightfieldMaterial(0, QByteArray(), QVector<SharedObjectPointer>());
    }
    return in;
}

template<> void Bitstream::writeRawDelta(const HeightfieldMaterialPointer& value,
        const HeightfieldMaterialPointer& reference) {
}

template<> void Bitstream::readRawDelta(HeightfieldMaterialPointer& value, const HeightfieldMaterialPointer& reference) {
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

QByteArray Heightfield::getRendererClassName() const {
    return "HeightfieldRenderer";
}

void Heightfield::updateBounds() {
    glm::vec3 extent(getScale(), getScale() * _aspectY, getScale() * _aspectZ);
    glm::mat4 rotationMatrix = glm::mat4_cast(getRotation());
    setBounds(glm::translate(getTranslation()) * rotationMatrix * Box(-extent, extent));
}
