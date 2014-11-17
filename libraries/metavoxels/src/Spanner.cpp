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

void SpannerRenderer::render(bool cursor) {
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
        "Images (*.png *.jpg *.bmp *.raw)");
    if (result.isNull()) {
        return;
    }
    settings.setValue("heightDir", QFileInfo(result).path());
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

bool Heightfield::isHeightfield() const {
    return true;
}

float Heightfield::getHeight(const glm::vec3& location) const {
    if (!_height) {
        return -FLT_MAX;
    }
    int width = _height->getWidth();
    const QVector<quint16>& contents = _height->getContents();
    const quint16* src = contents.constData();
    int height = contents.size() / width;
    int innerWidth = width - HeightfieldHeight::HEIGHT_EXTENSION;
    int innerHeight = height - HeightfieldHeight::HEIGHT_EXTENSION;
    int highestX = innerWidth + HeightfieldHeight::HEIGHT_BORDER;
    int highestZ = innerHeight + HeightfieldHeight::HEIGHT_BORDER;
    
    glm::vec3 relative = glm::inverse(getRotation()) * (location - getTranslation()) * glm::vec3(1.0f / getScale(),
        1.0f, 1.0f / (getScale() * _aspectZ));
    if (relative.x < 0.0f || relative.z < 0.0f || relative.x > 1.0f || relative.z > 1.0f) {
        return -FLT_MAX;
    }
    relative.x = relative.x * innerWidth + HeightfieldHeight::HEIGHT_BORDER;
    relative.z = relative.z * innerHeight + HeightfieldHeight::HEIGHT_BORDER;
    
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
        return -FLT_MAX; // ignore zero values
    }
    
    // convert the interpolated height into world space
    return getTranslation().y + interpolatedHeight * getScale() * _aspectY / numeric_limits<quint16>::max();
}

bool Heightfield::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
    if (!_height) {
        return false;
    }
    float boundsDistance;
    if (!getBounds().findRayIntersection(origin, direction, boundsDistance)) {
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
    
    glm::quat inverseRotation = glm::inverse(getRotation());
    glm::vec3 inverseScale(innerWidth / getScale(),  numeric_limits<quint16>::max() / (getScale() * _aspectY),
        innerHeight / (getScale() * _aspectZ));
    glm::vec3 dir = inverseRotation * direction * inverseScale;
    glm::vec3 entry = inverseRotation * (origin + direction * boundsDistance - getTranslation()) * inverseScale;
    
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

Spanner* Heightfield::paintMaterial(const glm::vec3& position, float radius,
        const SharedObjectPointer& material, const QColor& color) {
    if (!_height) {
        return this;
    }
    int heightWidth = _height->getWidth();
    int heightHeight = _height->getContents().size() / heightWidth;
    int baseWidth = heightWidth - HeightfieldHeight::HEIGHT_EXTENSION + HeightfieldData::SHARED_EDGE;
    int baseHeight = heightHeight - HeightfieldHeight::HEIGHT_EXTENSION + HeightfieldData::SHARED_EDGE;
    Heightfield* newHeightfield = static_cast<Heightfield*>(clone(true));
    
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
    glm::vec3 inverseScale(highestX / getScale(), 1.0f, highestZ / (getScale() * _aspectZ));
    glm::vec3 center = glm::inverse(getRotation()) * (position - getTranslation()) * inverseScale;
    
    glm::vec3 extents = glm::vec3(radius, radius, radius) * inverseScale;
    glm::vec3 start = glm::floor(center - extents);
    glm::vec3 end = glm::ceil(center + extents);
    
    // paint all points within the radius
    float z = qMax(start.z, 0.0f);
    float startX = qMax(start.x, 0.0f), endX = qMin(end.x, (float)highestX);
    int stride = colorWidth * DataBlock::COLOR_BYTES;
    uchar* lineDest = (uchar*)colorContents.data() + (int)z * stride + (int)startX * DataBlock::COLOR_BYTES;
    float squaredRadius = extents.x * extents.x;
    float multiplierZ = inverseScale.x / inverseScale.z;
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
    if (changed) {
        newHeightfield->setColor(HeightfieldColorPointer(new HeightfieldColor(colorWidth, colorContents)));
    }
    
    highestX = materialWidth - 1;
    highestZ = materialHeight - 1;
    inverseScale = glm::vec3(highestX / getScale(), 1.0f, highestZ / (getScale() * _aspectZ));
    center = glm::inverse(getRotation()) * (position - getTranslation()) * inverseScale;
    
    extents = glm::vec3(radius, radius, radius) * inverseScale;
    start = glm::floor(center - extents);
    end = glm::ceil(center + extents);
     
    // paint all points within the radius
    z = qMax(start.z, 0.0f);
    startX = qMax(start.x, 0.0f), endX = qMin(end.x, (float)highestX);
    lineDest = (uchar*)materialContents.data() + (int)z * materialWidth + (int)startX;
    squaredRadius = extents.x * extents.x; 
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
        clearUnusedMaterials(materials, materialContents);
        newHeightfield->setMaterial(HeightfieldMaterialPointer(new HeightfieldMaterial(materialWidth,
            materialContents, materials)));
    }
    
    return newHeightfield;
}

Spanner* Heightfield::paintHeight(const glm::vec3& position, float radius, float height) {
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
    Heightfield* newHeightfield = static_cast<Heightfield*>(clone(true));
    
    glm::vec3 inverseScale(innerWidth / getScale(), 1.0f, innerHeight / (getScale() * _aspectZ));
    glm::vec3 center = glm::inverse(getRotation()) * (position - getTranslation()) * inverseScale;
    center.x += 1.0f;
    center.z += 1.0f;
    
    glm::vec3 extents = glm::vec3(radius, radius, radius) * inverseScale;
    glm::vec3 start = glm::floor(center - extents);
    glm::vec3 end = glm::ceil(center + extents);
    
    // first see if we're going to exceed the range limits
    float z = qMax(start.z, 0.0f);
    float startX = qMax(start.x, 0.0f), endX = qMin(end.x, (float)highestX);
    quint16* lineDest = contents.data() + (int)z * heightWidth + (int)startX;
    float squaredRadius = extents.x * extents.x;
    float squaredRadiusReciprocal = 1.0f / squaredRadius;
    float scaledHeight = height * numeric_limits<quint16>::max() / (getScale() * _aspectY);
    float multiplierZ = inverseScale.x / inverseScale.z;
    int minimumValue = 1, maximumValue = numeric_limits<quint16>::max();
    for (float endZ = qMin(end.z, (float)highestZ); z <= endZ; z += 1.0f) {
        quint16* dest = lineDest;
        for (float x = startX; x <= endX; x += 1.0f, dest++) {
            float dx = x - center.x, dz = (z - center.z) * multiplierZ;
            float distanceSquared = dx * dx + dz * dz;
            if (distanceSquared <= squaredRadius) {
                // height falls off towards edges
                int value = *dest;
                if (value != 0) {
                    value += scaledHeight * (squaredRadius - distanceSquared) * squaredRadiusReciprocal;
                    minimumValue = qMin(minimumValue, value);
                    maximumValue = qMax(maximumValue, value);
                }
            }
        }
        lineDest += heightWidth;
    }
    
    // renormalize if necessary
    if (minimumValue < 1 || maximumValue > numeric_limits<quint16>::max()) {
        float scale = (numeric_limits<quint16>::max() - 1.0f) / (maximumValue - minimumValue);
        float offset = 1.0f - minimumValue;
        newHeightfield->setAspectY(_aspectY / scale);
        newHeightfield->setTranslation(getTranslation() - getRotation() *
            glm::vec3(0.0f, offset * _aspectY * getScale() / (numeric_limits<quint16>::max() - 1), 0.0f));
        for (quint16* dest = contents.data(), *end = contents.data() + contents.size(); dest != end; dest++) {
            int value = *dest;
            if (value != 0) {
                *dest = (value + offset) * scale;
            }
        }
    }
    
    // now apply the actual change
    z = qMax(start.z, 0.0f);
    lineDest = contents.data() + (int)z * heightWidth + (int)startX;
    scaledHeight = height * numeric_limits<quint16>::max() / (getScale() * newHeightfield->getAspectY());
    bool changed = false;
    for (float endZ = qMin(end.z, (float)highestZ); z <= endZ; z += 1.0f) {
        quint16* dest = lineDest;
        for (float x = startX; x <= endX; x += 1.0f, dest++) {
            float dx = x - center.x, dz = (z - center.z) * multiplierZ;
            float distanceSquared = dx * dx + dz * dz;
            if (distanceSquared <= squaredRadius) {
                // height falls off towards edges
                int value = *dest;
                if (value != 0) {
                    *dest = value + scaledHeight * (squaredRadius - distanceSquared) * squaredRadiusReciprocal;
                    changed = true;
                }
            }
        }
        lineDest += heightWidth;
    }
    if (changed) {
        newHeightfield->setHeight(HeightfieldHeightPointer(new HeightfieldHeight(heightWidth, contents)));
    }
    
    return newHeightfield;
}

QByteArray Heightfield::getRendererClassName() const {
    return "HeightfieldRenderer";
}

void Heightfield::updateBounds() {
    glm::vec3 extent(getScale(), getScale() * _aspectY, getScale() * _aspectZ);
    glm::mat4 rotationMatrix = glm::mat4_cast(getRotation());
    setBounds(glm::translate(getTranslation()) * rotationMatrix * Box(glm::vec3(), extent));
}
