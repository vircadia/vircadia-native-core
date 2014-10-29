//
//  MetavoxelSystem.cpp
//  interface/src
//
//  Created by Andrzej Kapolka on 12/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QOpenGLFramebufferObject, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QMutexLocker>
#include <QOpenGLFramebufferObject>
#include <QReadLocker>
#include <QWriteLocker>
#include <QThreadPool>
#include <QtDebug>

#include <glm/gtx/transform.hpp>

#include <SharedUtil.h>

#include <MetavoxelMessages.h>
#include <MetavoxelUtil.h>
#include <ScriptCache.h>

#include "Application.h"
#include "MetavoxelSystem.h"
#include "renderer/Model.h"
#include "renderer/RenderUtil.h"

REGISTER_META_OBJECT(DefaultMetavoxelRendererImplementation)
REGISTER_META_OBJECT(SphereRenderer)
REGISTER_META_OBJECT(CuboidRenderer)
REGISTER_META_OBJECT(StaticModelRenderer)

static int bufferPointVectorMetaTypeId = qRegisterMetaType<BufferPointVector>();

MetavoxelSystem::NetworkSimulation::NetworkSimulation(float dropRate, float repeatRate,
        int minimumDelay, int maximumDelay, int bandwidthLimit) :
    dropRate(dropRate),
    repeatRate(repeatRate),
    minimumDelay(minimumDelay),
    maximumDelay(maximumDelay),
    bandwidthLimit(bandwidthLimit) {
}    

MetavoxelSystem::~MetavoxelSystem() {
    // kill the updater before we delete our network simulation objects
    _updater->thread()->quit();
    _updater->thread()->wait();
    _updater = NULL;
}

void MetavoxelSystem::init() {
    MetavoxelClientManager::init();
    DefaultMetavoxelRendererImplementation::init();
    
    _pointBufferAttribute = AttributeRegistry::getInstance()->registerAttribute(new BufferDataAttribute("pointBuffer"));
    
    _heightfieldBufferAttribute = AttributeRegistry::getInstance()->registerAttribute(
        new BufferDataAttribute("heightfieldBuffer"));  
    _heightfieldBufferAttribute->setLODThresholdMultiplier(
        AttributeRegistry::getInstance()->getHeightfieldAttribute()->getLODThresholdMultiplier());
    
    _voxelBufferAttribute = AttributeRegistry::getInstance()->registerAttribute(
        new BufferDataAttribute("voxelBuffer"));
    _voxelBufferAttribute->setLODThresholdMultiplier(
        AttributeRegistry::getInstance()->getVoxelColorAttribute()->getLODThresholdMultiplier());
}

MetavoxelLOD MetavoxelSystem::getLOD() {
    QReadLocker locker(&_lodLock);
    return _lod;
}

void MetavoxelSystem::setNetworkSimulation(const NetworkSimulation& simulation) {
    QWriteLocker locker(&_networkSimulationLock);
    _networkSimulation = simulation;
}

MetavoxelSystem::NetworkSimulation MetavoxelSystem::getNetworkSimulation() {
    QReadLocker locker(&_networkSimulationLock);
    return _networkSimulation;
}

class SimulateVisitor : public MetavoxelVisitor {
public:
    
    SimulateVisitor(float deltaTime, const MetavoxelLOD& lod);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    float _deltaTime;
};

SimulateVisitor::SimulateVisitor(float deltaTime, const MetavoxelLOD& lod) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getRendererAttribute(),
        QVector<AttributePointer>(), lod),
    _deltaTime(deltaTime) {
}

int SimulateVisitor::visit(MetavoxelInfo& info) {
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    static_cast<MetavoxelRenderer*>(info.inputValues.at(0).getInlineValue<
        SharedObjectPointer>().data())->getImplementation()->simulate(*_data, _deltaTime, info, _lod);
    return STOP_RECURSION;
}

void MetavoxelSystem::simulate(float deltaTime) {
    // update the lod
    {
        QWriteLocker locker(&_lodLock);
        const float DEFAULT_LOD_THRESHOLD = 0.01f;
        _lod = MetavoxelLOD(Application::getInstance()->getCamera()->getPosition(), DEFAULT_LOD_THRESHOLD);
    }

    SimulateVisitor simulateVisitor(deltaTime, getLOD());
    guideToAugmented(simulateVisitor);
}

class RenderVisitor : public MetavoxelVisitor {
public:
    
    RenderVisitor(const MetavoxelLOD& lod);
    
    virtual int visit(MetavoxelInfo& info);
};

RenderVisitor::RenderVisitor(const MetavoxelLOD& lod) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getRendererAttribute(),
        QVector<AttributePointer>(), lod) {
}

int RenderVisitor::visit(MetavoxelInfo& info) {
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    static_cast<MetavoxelRenderer*>(info.inputValues.at(0).getInlineValue<
        SharedObjectPointer>().data())->getImplementation()->render(*_data, info, _lod);
    return STOP_RECURSION;
}

void MetavoxelSystem::render() {
    // update the frustum
    ViewFrustum* viewFrustum = Application::getInstance()->getDisplayViewFrustum();
    _frustum.set(viewFrustum->getFarTopLeft(), viewFrustum->getFarTopRight(), viewFrustum->getFarBottomLeft(),
        viewFrustum->getFarBottomRight(), viewFrustum->getNearTopLeft(), viewFrustum->getNearTopRight(),
        viewFrustum->getNearBottomLeft(), viewFrustum->getNearBottomRight());
   
    RenderVisitor renderVisitor(getLOD());
    guideToAugmented(renderVisitor, true);
    
    // give external parties a chance to join in
    emit rendering();
}

void MetavoxelSystem::refreshVoxelData() {
    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        if (node->getType() == NodeType::MetavoxelServer) {
            QMutexLocker locker(&node->getMutex());
            MetavoxelSystemClient* client = static_cast<MetavoxelSystemClient*>(node->getLinkedData());
            if (client) {
                QMetaObject::invokeMethod(client, "refreshVoxelData");
            }
        }
    }
}

class RayHeightfieldIntersectionVisitor : public RayIntersectionVisitor {
public:
    
    float intersectionDistance;
    
    RayHeightfieldIntersectionVisitor(const glm::vec3& origin, const glm::vec3& direction, const MetavoxelLOD& lod);
    
    virtual int visit(MetavoxelInfo& info, float distance);
};

RayHeightfieldIntersectionVisitor::RayHeightfieldIntersectionVisitor(const glm::vec3& origin,
        const glm::vec3& direction, const MetavoxelLOD& lod) :
    RayIntersectionVisitor(origin, direction, QVector<AttributePointer>() <<
        Application::getInstance()->getMetavoxels()->getHeightfieldBufferAttribute(), QVector<AttributePointer>(), lod),
    intersectionDistance(FLT_MAX) {
}

static const int EIGHT_BIT_MAXIMUM = 255;
static const float EIGHT_BIT_MAXIMUM_RECIPROCAL = 1.0f / EIGHT_BIT_MAXIMUM;

int RayHeightfieldIntersectionVisitor::visit(MetavoxelInfo& info, float distance) {
    if (!info.isLeaf) {
        return _order;
    }
    const HeightfieldBuffer* buffer = static_cast<HeightfieldBuffer*>(
        info.inputValues.at(0).getInlineValue<BufferDataPointer>().data());
    if (!buffer) {
        return STOP_RECURSION;
    }
    const QByteArray& contents = buffer->getHeight();
    const uchar* src = (const uchar*)contents.constData();
    int size = glm::sqrt((float)contents.size());
    int unextendedSize = size - HeightfieldBuffer::HEIGHT_EXTENSION;
    int highest = HeightfieldBuffer::HEIGHT_BORDER + unextendedSize;
    float heightScale = unextendedSize * EIGHT_BIT_MAXIMUM_RECIPROCAL;
    
    // find the initial location in heightfield coordinates
    glm::vec3 entry = (_origin + distance * _direction - info.minimum) * (float)unextendedSize / info.size;
    entry.x += HeightfieldBuffer::HEIGHT_BORDER;
    entry.z += HeightfieldBuffer::HEIGHT_BORDER;
    glm::vec3 floors = glm::floor(entry);
    glm::vec3 ceils = glm::ceil(entry);
    if (floors.x == ceils.x) {
        if (_direction.x > 0.0f) {
            ceils.x += 1.0f;
        } else {
            floors.x -= 1.0f;
        } 
    }
    if (floors.z == ceils.z) {
        if (_direction.z > 0.0f) {
            ceils.z += 1.0f;
        } else {
            floors.z -= 1.0f;
        }
    }
    
    bool withinBounds = true;
    float accumulatedDistance = 0.0f;
    while (withinBounds) {
        // find the heights at the corners of the current cell
        int floorX = qMin(qMax((int)floors.x, HeightfieldBuffer::HEIGHT_BORDER), highest);
        int floorZ = qMin(qMax((int)floors.z, HeightfieldBuffer::HEIGHT_BORDER), highest);
        int ceilX = qMin(qMax((int)ceils.x, HeightfieldBuffer::HEIGHT_BORDER), highest);
        int ceilZ = qMin(qMax((int)ceils.z, HeightfieldBuffer::HEIGHT_BORDER), highest);
        float upperLeft = src[floorZ * size + floorX] * heightScale;
        float upperRight = src[floorZ * size + ceilX] * heightScale;
        float lowerLeft = src[ceilZ * size + floorX] * heightScale;
        float lowerRight = src[ceilZ * size + ceilX] * heightScale;
        
        // find the distance to the next x coordinate
        float xDistance = FLT_MAX;
        if (_direction.x > 0.0f) {
            xDistance = (ceils.x - entry.x) / _direction.x;
        } else if (_direction.x < 0.0f) {
            xDistance = (floors.x - entry.x) / _direction.x;
        }
        
        // and the distance to the next z coordinate
        float zDistance = FLT_MAX;
        if (_direction.z > 0.0f) {
            zDistance = (ceils.z - entry.z) / _direction.z;
        } else if (_direction.z < 0.0f) {
            zDistance = (floors.z - entry.z) / _direction.z;
        }
        
        // the exit distance is the lower of those two
        float exitDistance = qMin(xDistance, zDistance);
        glm::vec3 exit, nextFloors = floors, nextCeils = ceils;
        if (exitDistance == FLT_MAX) {
            if (_direction.y > 0.0f) {
                return SHORT_CIRCUIT; // line points upwards; no collisions possible
            }    
            withinBounds = false; // line points downwards; check this cell only
            
        } else {
            // find the exit point and the next cell, and determine whether it's still within the bounds
            exit = entry + exitDistance * _direction;
            withinBounds = (exit.y >= HeightfieldBuffer::HEIGHT_BORDER && exit.y <= highest);
            if (exitDistance == xDistance) {
                if (_direction.x > 0.0f) {
                    nextFloors.x += 1.0f;
                    withinBounds &= (nextCeils.x += 1.0f) <= highest;
                } else {
                    withinBounds &= (nextFloors.x -= 1.0f) >= HeightfieldBuffer::HEIGHT_BORDER;
                    nextCeils.x -= 1.0f;
                }
            }
            if (exitDistance == zDistance) {
                if (_direction.z > 0.0f) {
                    nextFloors.z += 1.0f;
                    withinBounds &= (nextCeils.z += 1.0f) <= highest;
                } else {
                    withinBounds &= (nextFloors.z -= 1.0f) >= HeightfieldBuffer::HEIGHT_BORDER;
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
        float lowerProduct = glm::dot(lowerNormal, _direction);
        if (lowerProduct < 0.0f) {
            float planeDistance = -glm::dot(lowerNormal, relativeEntry) / lowerProduct;
            glm::vec3 intersection = relativeEntry + planeDistance * _direction;
            if (intersection.x >= 0.0f && intersection.x <= 1.0f && intersection.z >= 0.0f && intersection.z <= 1.0f &&
                    intersection.z >= intersection.x) {
                intersectionDistance = qMin(intersectionDistance, distance +
                    (accumulatedDistance + planeDistance) * (info.size / unextendedSize));
                return SHORT_CIRCUIT;
            }
        }
        
        // then the one with the X+ segment
        glm::vec3 upperNormal(upperLeft - upperRight, 1.0f, upperRight - lowerRight);
        float upperProduct = glm::dot(upperNormal, _direction);
        if (upperProduct < 0.0f) {
            float planeDistance = -glm::dot(upperNormal, relativeEntry) / upperProduct;
            glm::vec3 intersection = relativeEntry + planeDistance * _direction;
            if (intersection.x >= 0.0f && intersection.x <= 1.0f && intersection.z >= 0.0f && intersection.z <= 1.0f &&
                    intersection.x >= intersection.z) {
                intersectionDistance = qMin(intersectionDistance, distance +
                    (accumulatedDistance + planeDistance) * (info.size / unextendedSize));
                return SHORT_CIRCUIT;
            }
        }
        
        // no joy; continue on our way
        entry = exit;
        floors = nextFloors;
        ceils = nextCeils;
        accumulatedDistance += exitDistance;
    }
    
    return STOP_RECURSION;
}

bool MetavoxelSystem::findFirstRayHeightfieldIntersection(const glm::vec3& origin,
        const glm::vec3& direction, float& distance) {
    RayHeightfieldIntersectionVisitor visitor(origin, direction, getLOD());
    guideToAugmented(visitor);
    if (visitor.intersectionDistance == FLT_MAX) {
        return false;
    }
    distance = visitor.intersectionDistance;
    return true;
}

class RayVoxelIntersectionVisitor : public RayIntersectionVisitor {
public:
    
    float intersectionDistance;
    
    RayVoxelIntersectionVisitor(const glm::vec3& origin, const glm::vec3& direction, const MetavoxelLOD& lod);
    
    virtual int visit(MetavoxelInfo& info, float distance);
};

RayVoxelIntersectionVisitor::RayVoxelIntersectionVisitor(const glm::vec3& origin,
        const glm::vec3& direction, const MetavoxelLOD& lod) :
    RayIntersectionVisitor(origin, direction, QVector<AttributePointer>() <<
        Application::getInstance()->getMetavoxels()->getVoxelBufferAttribute(), QVector<AttributePointer>(), lod),
    intersectionDistance(FLT_MAX) {
}

int RayVoxelIntersectionVisitor::visit(MetavoxelInfo& info, float distance) {
    if (!info.isLeaf) {
        return _order;
    }
    const VoxelBuffer* buffer = static_cast<VoxelBuffer*>(
        info.inputValues.at(0).getInlineValue<BufferDataPointer>().data());
    if (!buffer) {
        return STOP_RECURSION;
    }
    glm::vec3 entry = ((_origin + distance * _direction) - info.minimum) / info.size;
    if (buffer->findFirstRayIntersection(entry, _origin, _direction, intersectionDistance)) {
        return SHORT_CIRCUIT;
    }
    return STOP_RECURSION;
}

bool MetavoxelSystem::findFirstRayVoxelIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) {
    RayVoxelIntersectionVisitor visitor(origin, direction, getLOD());
    guideToAugmented(visitor);
    if (visitor.intersectionDistance == FLT_MAX) {
        return false;
    }
    distance = visitor.intersectionDistance;
    return true;
}

class HeightfieldHeightVisitor : public MetavoxelVisitor {
public:
    
    float height;
    
    HeightfieldHeightVisitor(const MetavoxelLOD& lod, const glm::vec3& location);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    glm::vec3 _location;
};

HeightfieldHeightVisitor::HeightfieldHeightVisitor(const MetavoxelLOD& lod, const glm::vec3& location) :
    MetavoxelVisitor(QVector<AttributePointer>() <<
        Application::getInstance()->getMetavoxels()->getHeightfieldBufferAttribute(), QVector<AttributePointer>(), lod),
    height(-FLT_MAX),
    _location(location) {
}

static const int REVERSE_ORDER = MetavoxelVisitor::encodeOrder(7, 6, 5, 4, 3, 2, 1, 0);

int HeightfieldHeightVisitor::visit(MetavoxelInfo& info) {
    glm::vec3 relative = _location - info.minimum;
    if (relative.x < 0.0f || relative.z < 0.0f || relative.x > info.size || relative.z > info.size ||
            height >= info.minimum.y + info.size) {
        return STOP_RECURSION;
    }
    if (!info.isLeaf) {
        return REVERSE_ORDER;
    }
    const HeightfieldBuffer* buffer = static_cast<HeightfieldBuffer*>(
        info.inputValues.at(0).getInlineValue<BufferDataPointer>().data());
    if (!buffer) {
        return STOP_RECURSION;
    }
    const QByteArray& contents = buffer->getHeight();
    const uchar* src = (const uchar*)contents.constData();
    int size = glm::sqrt((float)contents.size());
    int unextendedSize = size - HeightfieldBuffer::HEIGHT_EXTENSION;
    int highest = HeightfieldBuffer::HEIGHT_BORDER + unextendedSize;
    relative *= unextendedSize / info.size;
    relative.x += HeightfieldBuffer::HEIGHT_BORDER;
    relative.z += HeightfieldBuffer::HEIGHT_BORDER;
    
    // find the bounds of the cell containing the point and the shared vertex heights
    glm::vec3 floors = glm::floor(relative);
    glm::vec3 ceils = glm::ceil(relative);
    glm::vec3 fracts = glm::fract(relative);
    int floorX = qMin(qMax((int)floors.x, HeightfieldBuffer::HEIGHT_BORDER), highest);
    int floorZ = qMin(qMax((int)floors.z, HeightfieldBuffer::HEIGHT_BORDER), highest);
    int ceilX = qMin(qMax((int)ceils.x, HeightfieldBuffer::HEIGHT_BORDER), highest);
    int ceilZ = qMin(qMax((int)ceils.z, HeightfieldBuffer::HEIGHT_BORDER), highest);
    float upperLeft = src[floorZ * size + floorX];
    float lowerRight = src[ceilZ * size + ceilX];
    float interpolatedHeight = glm::mix(upperLeft, lowerRight, fracts.z);
    
    // the final vertex (and thus which triangle we check) depends on which half we're on
    if (fracts.x >= fracts.z) {
        float upperRight = src[floorZ * size + ceilX];
        interpolatedHeight = glm::mix(interpolatedHeight, glm::mix(upperRight, lowerRight, fracts.z),
            (fracts.x - fracts.z) / (1.0f - fracts.z));
        
    } else {
        float lowerLeft = src[ceilZ * size + floorX];
        interpolatedHeight = glm::mix(glm::mix(upperLeft, lowerLeft, fracts.z), interpolatedHeight, fracts.x / fracts.z);
    }
    if (interpolatedHeight == 0.0f) {
        return STOP_RECURSION; // ignore zero values
    }
    
    // convert the interpolated height into world space
    height = qMax(height, info.minimum.y + interpolatedHeight * info.size * EIGHT_BIT_MAXIMUM_RECIPROCAL);
    return SHORT_CIRCUIT;
}

float MetavoxelSystem::getHeightfieldHeight(const glm::vec3& location) {
    HeightfieldHeightVisitor visitor(getLOD(), location);
    guideToAugmented(visitor);
    return visitor.height;
}

void MetavoxelSystem::paintHeightfieldColor(const glm::vec3& position, float radius, const QColor& color) {
    MetavoxelEditMessage edit = { QVariant::fromValue(PaintHeightfieldMaterialEdit(position, radius, SharedObjectPointer(), color)) };
    applyEdit(edit, true);
}

void MetavoxelSystem::paintHeightfieldMaterial(const glm::vec3& position, float radius, const SharedObjectPointer& material) {
    MetavoxelEditMessage edit = { QVariant::fromValue(PaintHeightfieldMaterialEdit(position, radius, material)) };
    applyMaterialEdit(edit, true);
}

void MetavoxelSystem::paintVoxelColor(const glm::vec3& position, float radius, const QColor& color) {
    MetavoxelEditMessage edit = { QVariant::fromValue(PaintVoxelMaterialEdit(position, radius, SharedObjectPointer(), color)) };
    applyEdit(edit, true);
}

void MetavoxelSystem::paintVoxelMaterial(const glm::vec3& position, float radius, const SharedObjectPointer& material) {
    MetavoxelEditMessage edit = { QVariant::fromValue(PaintVoxelMaterialEdit(position, radius, material)) };
    applyMaterialEdit(edit, true);
}

void MetavoxelSystem::setVoxelColor(const SharedObjectPointer& spanner, const QColor& color) {
    MetavoxelEditMessage edit = { QVariant::fromValue(VoxelMaterialSpannerEdit(spanner, SharedObjectPointer(), color)) };
    applyEdit(edit, true);
}

void MetavoxelSystem::setVoxelMaterial(const SharedObjectPointer& spanner, const SharedObjectPointer& material) {
    MetavoxelEditMessage edit = { QVariant::fromValue(VoxelMaterialSpannerEdit(spanner, material)) };
    applyMaterialEdit(edit, true);
}

class CursorRenderVisitor : public MetavoxelVisitor {
public:
    
    CursorRenderVisitor(const AttributePointer& attribute, const Box& bounds);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    Box _bounds;
};

CursorRenderVisitor::CursorRenderVisitor(const AttributePointer& attribute, const Box& bounds) :
    MetavoxelVisitor(QVector<AttributePointer>() << attribute),
    _bounds(bounds) {
}

int CursorRenderVisitor::visit(MetavoxelInfo& info) {
    if (!info.getBounds().intersects(_bounds)) {
        return STOP_RECURSION;
    }
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    BufferDataPointer buffer = info.inputValues.at(0).getInlineValue<BufferDataPointer>();
    if (buffer) {
        buffer->render(true);
    }
    return STOP_RECURSION;
}

void MetavoxelSystem::renderHeightfieldCursor(const glm::vec3& position, float radius) {
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    DefaultMetavoxelRendererImplementation::getHeightfieldCursorProgram().bind();
    
    glActiveTexture(GL_TEXTURE4);
    float scale = 1.0f / radius;
    glm::vec4 sCoefficients(scale, 0.0f, 0.0f, -scale * position.x);
    glm::vec4 tCoefficients(0.0f, 0.0f, scale, -scale * position.z);
    glm::vec4 rCoefficients(0.0f, 0.0f, 0.0f, 0.0f);
    glTexGenfv(GL_S, GL_EYE_PLANE, (const GLfloat*)&sCoefficients);
    glTexGenfv(GL_T, GL_EYE_PLANE, (const GLfloat*)&tCoefficients);
    glTexGenfv(GL_R, GL_EYE_PLANE, (const GLfloat*)&rCoefficients);
    glActiveTexture(GL_TEXTURE0);
    
    glm::vec3 extents(radius, radius, radius);
    CursorRenderVisitor visitor(Application::getInstance()->getMetavoxels()->getHeightfieldBufferAttribute(),
        Box(position - extents, position + extents));
    guideToAugmented(visitor);
    
    DefaultMetavoxelRendererImplementation::getHeightfieldCursorProgram().release();
    
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
}

void MetavoxelSystem::renderVoxelCursor(const glm::vec3& position, float radius) {
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    
    DefaultMetavoxelRendererImplementation::getVoxelCursorProgram().bind();
    
    glActiveTexture(GL_TEXTURE4);
    float scale = 1.0f / radius;
    glm::vec4 sCoefficients(scale, 0.0f, 0.0f, -scale * position.x);
    glm::vec4 tCoefficients(0.0f, scale, 0.0f, -scale * position.y);
    glm::vec4 rCoefficients(0.0f, 0.0f, scale, -scale * position.z);
    glTexGenfv(GL_S, GL_EYE_PLANE, (const GLfloat*)&sCoefficients);
    glTexGenfv(GL_T, GL_EYE_PLANE, (const GLfloat*)&tCoefficients);
    glTexGenfv(GL_R, GL_EYE_PLANE, (const GLfloat*)&rCoefficients);
    glActiveTexture(GL_TEXTURE0);
    
    glm::vec3 extents(radius, radius, radius);
    Box bounds(position - extents, position + extents);
    CursorRenderVisitor voxelVisitor(Application::getInstance()->getMetavoxels()->getVoxelBufferAttribute(), bounds);
    guideToAugmented(voxelVisitor);
    
    DefaultMetavoxelRendererImplementation::getVoxelCursorProgram().release();
    
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    DefaultMetavoxelRendererImplementation::getHeightfieldCursorProgram().bind();
    
    CursorRenderVisitor heightfieldVisitor(Application::getInstance()->getMetavoxels()->getHeightfieldBufferAttribute(),
        bounds);
    guideToAugmented(heightfieldVisitor);
    
    DefaultMetavoxelRendererImplementation::getHeightfieldCursorProgram().release();
    
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
}

void MetavoxelSystem::deleteTextures(int heightID, int colorID, int textureID) {
    glDeleteTextures(1, (GLuint*)&heightID);
    glDeleteTextures(1, (GLuint*)&colorID);
    glDeleteTextures(1, (GLuint*)&textureID);
}

class MaterialEditApplier : public SignalHandler {
public:

    MaterialEditApplier(const MetavoxelEditMessage& message, const QSharedPointer<NetworkTexture> texture);
    
    virtual void handle();

protected:
    
    MetavoxelEditMessage _message;
    QSharedPointer<NetworkTexture> _texture;
};

MaterialEditApplier::MaterialEditApplier(const MetavoxelEditMessage& message, const QSharedPointer<NetworkTexture> texture) :
    _message(message),
    _texture(texture) {
}

void MaterialEditApplier::handle() {
    static_cast<MaterialEdit*>(_message.edit.data())->averageColor = _texture->getAverageColor();
    Application::getInstance()->getMetavoxels()->applyEdit(_message, true);
    deleteLater();
}

void MetavoxelSystem::applyMaterialEdit(const MetavoxelEditMessage& message, bool reliable) {
    const MaterialEdit* edit = static_cast<const MaterialEdit*>(message.edit.constData());
    MaterialObject* material = static_cast<MaterialObject*>(edit->material.data());
    if (material && material->getDiffuse().isValid()) {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, "applyMaterialEdit", Q_ARG(const MetavoxelEditMessage&, message),
                Q_ARG(bool, reliable));
            return;
        }
        QSharedPointer<NetworkTexture> texture = Application::getInstance()->getTextureCache()->getTexture(
            material->getDiffuse(), SPLAT_TEXTURE);
        if (texture->isLoaded()) {
            MetavoxelEditMessage newMessage = message;
            static_cast<MaterialEdit*>(newMessage.edit.data())->averageColor = texture->getAverageColor();
            applyEdit(newMessage, true);    
        
        } else {
            MaterialEditApplier* applier = new MaterialEditApplier(message, texture);
            connect(texture.data(), &Resource::loaded, applier, &SignalHandler::handle);
        }
    } else {
        applyEdit(message, true);
    }
}

MetavoxelClient* MetavoxelSystem::createClient(const SharedNodePointer& node) {
    return new MetavoxelSystemClient(node, _updater);
}

void MetavoxelSystem::guideToAugmented(MetavoxelVisitor& visitor, bool render) {
    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        if (node->getType() == NodeType::MetavoxelServer) {
            QMutexLocker locker(&node->getMutex());
            MetavoxelSystemClient* client = static_cast<MetavoxelSystemClient*>(node->getLinkedData());
            if (client) {
                MetavoxelData data = client->getAugmentedData();
                data.guide(visitor);
                if (render) {
                    // save the rendered augmented data so that its cached texture references, etc., don't
                    // get collected when we replace it with more recent versions
                    client->setRenderedAugmentedData(data);
                }
            }
        }
    }
}

Throttle::Throttle() :
    _limit(INT_MAX),
    _total(0) {
}

bool Throttle::shouldThrottle(int bytes) {
    // clear expired buckets
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    while (!_buckets.isEmpty() && now >= _buckets.first().first) {
        _total -= _buckets.takeFirst().second;
    }
    
    // if possible, add the new bucket
    if (_total + bytes > _limit) {
        return true;
    }
    const int BUCKET_DURATION = 1000;
    _buckets.append(Bucket(now + BUCKET_DURATION, bytes));
    _total += bytes;
    return false;
}

MetavoxelSystemClient::MetavoxelSystemClient(const SharedNodePointer& node, MetavoxelUpdater* updater) :
    MetavoxelClient(node, updater) {
}

void MetavoxelSystemClient::setAugmentedData(const MetavoxelData& data) {
    QWriteLocker locker(&_augmentedDataLock);
    _augmentedData = data;
}

MetavoxelData MetavoxelSystemClient::getAugmentedData() {
    QReadLocker locker(&_augmentedDataLock);
    return _augmentedData;
}

class ReceiveDelayer : public QObject {
public:
    
    ReceiveDelayer(const SharedNodePointer& node, const QByteArray& packet);

protected:

    virtual void timerEvent(QTimerEvent* event);

private:
    
    SharedNodePointer _node;
    QByteArray _packet;
};

ReceiveDelayer::ReceiveDelayer(const SharedNodePointer& node, const QByteArray& packet) :
    _node(node),
    _packet(packet) {
}

void ReceiveDelayer::timerEvent(QTimerEvent* event) {
    QMutexLocker locker(&_node->getMutex());
    MetavoxelClient* client = static_cast<MetavoxelClient*>(_node->getLinkedData());
    if (client) {
        QMetaObject::invokeMethod(&client->getSequencer(), "receivedDatagram", Q_ARG(const QByteArray&, _packet));
    }
    deleteLater();
}

int MetavoxelSystemClient::parseData(const QByteArray& packet) {
    // process through sequencer
    MetavoxelSystem::NetworkSimulation simulation = Application::getInstance()->getMetavoxels()->getNetworkSimulation();
    if (randFloat() < simulation.dropRate) {
        return packet.size();
    }
    int count = (randFloat() < simulation.repeatRate) ? 2 : 1;
    for (int i = 0; i < count; i++) {
        if (simulation.bandwidthLimit > 0) {
            _receiveThrottle.setLimit(simulation.bandwidthLimit);
            if (_receiveThrottle.shouldThrottle(packet.size())) {
                continue;
            }
        }
        int delay = randIntInRange(simulation.minimumDelay, simulation.maximumDelay);
        if (delay > 0) {
            ReceiveDelayer* delayer = new ReceiveDelayer(_node, packet);
            delayer->startTimer(delay);
            
        } else {
            QMetaObject::invokeMethod(&_sequencer, "receivedDatagram", Q_ARG(const QByteArray&, packet));
        }
        Application::getInstance()->getBandwidthMeter()->inputStream(BandwidthMeter::METAVOXELS).updateValue(packet.size());
    }
    return packet.size();
}

class AugmentVisitor : public MetavoxelVisitor {
public:
    
    AugmentVisitor(const MetavoxelLOD& lod, const MetavoxelData& previousData);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    const MetavoxelData& _previousData;
};

AugmentVisitor::AugmentVisitor(const MetavoxelLOD& lod, const MetavoxelData& previousData) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getRendererAttribute(),
        QVector<AttributePointer>(), lod),
    _previousData(previousData) {
}

int AugmentVisitor::visit(MetavoxelInfo& info) {
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    static_cast<MetavoxelRenderer*>(info.inputValues.at(0).getInlineValue<
        SharedObjectPointer>().data())->getImplementation()->augment(*_data, _previousData, info, _lod);
    return STOP_RECURSION;
}

class Augmenter : public QRunnable {
public:
    
    Augmenter(const SharedNodePointer& node, const MetavoxelData& data,
        const MetavoxelData& previousData, const MetavoxelLOD& lod);
    
    virtual void run();

private:
    
    QWeakPointer<Node> _node;
    MetavoxelData _data;
    MetavoxelData _previousData;
    MetavoxelLOD _lod;
};

Augmenter::Augmenter(const SharedNodePointer& node, const MetavoxelData& data,
        const MetavoxelData& previousData, const MetavoxelLOD& lod) :
    _node(node),
    _data(data),
    _previousData(previousData),
    _lod(lod) {
}

void Augmenter::run() {
    SharedNodePointer node = _node;
    if (!node) {
        return;
    }
    AugmentVisitor visitor(_lod, _previousData);
    _data.guide(visitor);
    QMutexLocker locker(&node->getMutex());
    QMetaObject::invokeMethod(node->getLinkedData(), "setAugmentedData", Q_ARG(const MetavoxelData&, _data));
}

void MetavoxelSystemClient::refreshVoxelData() {
    // make it look as if all the colors have changed
    MetavoxelData oldData = getAugmentedData();
    oldData.touch(AttributeRegistry::getInstance()->getVoxelColorAttribute());

    QThreadPool::globalInstance()->start(new Augmenter(_node, _data, oldData, _remoteDataLOD));
}

void MetavoxelSystemClient::dataChanged(const MetavoxelData& oldData) {
    MetavoxelClient::dataChanged(oldData);
    QThreadPool::globalInstance()->start(new Augmenter(_node, _data, getAugmentedData(), _remoteDataLOD));
}

class SendDelayer : public QObject {
public:
    
    SendDelayer(const SharedNodePointer& node, const QByteArray& data);

    virtual void timerEvent(QTimerEvent* event);

private:
    
    SharedNodePointer _node;
    QByteArray _data;
};

SendDelayer::SendDelayer(const SharedNodePointer& node, const QByteArray& data) :
    _node(node),
    _data(data.constData(), data.size()) {
}

void SendDelayer::timerEvent(QTimerEvent* event) {
    NodeList::getInstance()->writeDatagram(_data, _node);
    deleteLater();
}

void MetavoxelSystemClient::sendDatagram(const QByteArray& data) {
    MetavoxelSystem::NetworkSimulation simulation = Application::getInstance()->getMetavoxels()->getNetworkSimulation();
    if (randFloat() < simulation.dropRate) {
        return;
    }
    int count = (randFloat() < simulation.repeatRate) ? 2 : 1;
    for (int i = 0; i < count; i++) {
        if (simulation.bandwidthLimit > 0) {
            _sendThrottle.setLimit(simulation.bandwidthLimit);
            if (_sendThrottle.shouldThrottle(data.size())) {
                continue;
            }
        }
        int delay = randIntInRange(simulation.minimumDelay, simulation.maximumDelay);
        if (delay > 0) {
            SendDelayer* delayer = new SendDelayer(_node, data);
            delayer->startTimer(delay);
            
        } else {
            NodeList::getInstance()->writeDatagram(data, _node);
        }
        Application::getInstance()->getBandwidthMeter()->outputStream(BandwidthMeter::METAVOXELS).updateValue(data.size());
    }
}

BufferData::~BufferData() {
}

PointBuffer::PointBuffer(const BufferPointVector& points) :
    _points(points) {
}

void PointBuffer::render(bool cursor) {
    // initialize buffer, etc. on first render
    if (!_buffer.isCreated()) {
        _buffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        _buffer.create();
        _buffer.bind();
        _pointCount = _points.size();
        _buffer.allocate(_points.constData(), _pointCount * sizeof(BufferPoint));
        _points.clear();
        _buffer.release();
    }
    if (_pointCount == 0) {
        return;
    }
    _buffer.bind();
    
    BufferPoint* point = 0;
    glVertexPointer(4, GL_FLOAT, sizeof(BufferPoint), &point->vertex);
    glColorPointer(3, GL_UNSIGNED_BYTE, sizeof(BufferPoint), &point->color);
    glNormalPointer(GL_BYTE, sizeof(BufferPoint), &point->normal);
    
    glDrawArrays(GL_POINTS, 0, _pointCount);
    
    _buffer.release();
}

const int HeightfieldBuffer::HEIGHT_BORDER = 1;
const int HeightfieldBuffer::SHARED_EDGE = 1;
const int HeightfieldBuffer::HEIGHT_EXTENSION = 2 * HeightfieldBuffer::HEIGHT_BORDER + HeightfieldBuffer::SHARED_EDGE;

HeightfieldBuffer::HeightfieldBuffer(const glm::vec3& translation, float scale,
        const QByteArray& height, const QByteArray& color, const QByteArray& material,
        const QVector<SharedObjectPointer>& materials) :
    _translation(translation),
    _scale(scale),
    _heightBounds(translation, translation + glm::vec3(scale, scale, scale)),
    _colorBounds(_heightBounds),
    _materialBounds(_heightBounds),
    _height(height),
    _color(color),
    _material(material),
    _materials(materials),
    _heightTextureID(0),
    _colorTextureID(0),
    _materialTextureID(0),
    _heightSize(glm::sqrt((float)height.size())),
    _heightIncrement(scale / (_heightSize - HEIGHT_EXTENSION)),
    _colorSize(glm::sqrt((float)color.size() / DataBlock::COLOR_BYTES)),
    _colorIncrement(scale / (_colorSize - SHARED_EDGE)),
    _materialSize(glm::sqrt((float)material.size())),
    _materialIncrement(scale / (_materialSize - SHARED_EDGE)) {
    
    _heightBounds.minimum.x -= _heightIncrement * HEIGHT_BORDER;
    _heightBounds.minimum.z -= _heightIncrement * HEIGHT_BORDER;
    _heightBounds.maximum.x += _heightIncrement * (SHARED_EDGE + HEIGHT_BORDER);
    _heightBounds.maximum.z += _heightIncrement * (SHARED_EDGE + HEIGHT_BORDER);
    
    _colorBounds.maximum.x += _colorIncrement * SHARED_EDGE;
    _colorBounds.maximum.z += _colorIncrement * SHARED_EDGE;
    
    _materialBounds.maximum.x += _materialIncrement * SHARED_EDGE;
    _materialBounds.maximum.z += _materialIncrement * SHARED_EDGE;
}

HeightfieldBuffer::~HeightfieldBuffer() {
    // the textures have to be deleted on the main thread (for its opengl context)
    if (QThread::currentThread() != Application::getInstance()->thread()) {
        QMetaObject::invokeMethod(Application::getInstance()->getMetavoxels(), "deleteTextures",
            Q_ARG(int, _heightTextureID), Q_ARG(int, _colorTextureID), Q_ARG(int, _materialTextureID));
    } else {
        glDeleteTextures(1, &_heightTextureID);
        glDeleteTextures(1, &_colorTextureID);
        glDeleteTextures(1, &_materialTextureID);
    }
}

QByteArray HeightfieldBuffer::getUnextendedHeight() const {
    int srcSize = glm::sqrt(float(_height.size()));
    int destSize = srcSize - 3;
    QByteArray unextended(destSize * destSize, 0);
    const char* src = _height.constData() + srcSize + 1;
    char* dest = unextended.data();
    for (int z = 0; z < destSize; z++, src += srcSize, dest += destSize) {
        memcpy(dest, src, destSize);
    }
    return unextended;
}

QByteArray HeightfieldBuffer::getUnextendedColor(int x, int y) const {
    int unextendedSize = _heightSize - HEIGHT_EXTENSION;
    QByteArray unextended(unextendedSize * unextendedSize * DataBlock::COLOR_BYTES, 0);
    char* dest = unextended.data();
    const char* src = _color.constData() + (y * _colorSize + x) * unextendedSize * DataBlock::COLOR_BYTES;
    for (int z = 0; z < unextendedSize; z++, dest += unextendedSize * DataBlock::COLOR_BYTES,
            src += _colorSize * DataBlock::COLOR_BYTES) {
        memcpy(dest, src, unextendedSize * DataBlock::COLOR_BYTES);
    }
    return unextended;
}

class HeightfieldPoint {
public:
    glm::vec2 textureCoord;
    glm::vec3 vertex;
};

const int SPLAT_COUNT = 4;
const GLint SPLAT_TEXTURE_UNITS[] = { 3, 4, 5, 6 };

void HeightfieldBuffer::render(bool cursor) {
    // initialize textures, etc. on first render
    if (_heightTextureID == 0) {
        glGenTextures(1, &_heightTextureID);
        glBindTexture(GL_TEXTURE_2D, _heightTextureID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _heightSize, _heightSize, 0,
            GL_LUMINANCE, GL_UNSIGNED_BYTE, _height.constData());
        
        glGenTextures(1, &_colorTextureID);
        glBindTexture(GL_TEXTURE_2D, _colorTextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (_color.isEmpty()) {
            const quint8 WHITE_COLOR[] = { 255, 255, 255 };
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, WHITE_COLOR);
               
        } else {
            int colorSize = glm::sqrt(float(_color.size() / DataBlock::COLOR_BYTES));
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, colorSize, colorSize, 0, GL_RGB, GL_UNSIGNED_BYTE, _color.constData());
        }
        
        if (!_material.isEmpty()) {
            glGenTextures(1, &_materialTextureID);
            glBindTexture(GL_TEXTURE_2D, _materialTextureID);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            int materialSize = glm::sqrt(float(_material.size()));
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, materialSize, materialSize, 0,
                GL_LUMINANCE, GL_UNSIGNED_BYTE, _material.constData());
            
            _networkTextures.resize(_materials.size());
            for (int i = 0; i < _materials.size(); i++) {
                const SharedObjectPointer material = _materials.at(i);
                if (material) {
                    _networkTextures[i] = Application::getInstance()->getTextureCache()->getTexture(
                        static_cast<MaterialObject*>(material.data())->getDiffuse(), SPLAT_TEXTURE);
                }
            }
        }
    }
    // create the buffer objects lazily
    int innerSize = _heightSize - 2 * HeightfieldBuffer::HEIGHT_BORDER;
    int vertexCount = _heightSize * _heightSize;
    int rows = _heightSize - 1;
    int indexCount = rows * rows * 3 * 2;
    BufferPair& bufferPair = _bufferPairs[_heightSize];
    if (!bufferPair.first.isCreated()) {
        QVector<HeightfieldPoint> vertices(vertexCount);
        HeightfieldPoint* point = vertices.data();
        
        float vertexStep = 1.0f / (innerSize - 1);
        float z = -vertexStep;
        float textureStep = 1.0f / _heightSize;
        float t = textureStep / 2.0f;
        for (int i = 0; i < _heightSize; i++, z += vertexStep, t += textureStep) {
            float x = -vertexStep;
            float s = textureStep / 2.0f;
            const float SKIRT_LENGTH = 0.25f;
            float baseY = (i == 0 || i == _heightSize - 1) ? -SKIRT_LENGTH : 0.0f;
            for (int j = 0; j < _heightSize; j++, point++, x += vertexStep, s += textureStep) {
                point->vertex = glm::vec3(x, (j == 0 || j == _heightSize - 1) ? -SKIRT_LENGTH : baseY, z);
                point->textureCoord = glm::vec2(s, t);
            }
        }
        
        bufferPair.first.setUsagePattern(QOpenGLBuffer::StaticDraw);
        bufferPair.first.create();
        bufferPair.first.bind();
        bufferPair.first.allocate(vertices.constData(), vertexCount * sizeof(HeightfieldPoint));
        
        QVector<int> indices(indexCount);
        int* index = indices.data();
        for (int i = 0; i < rows; i++) {
            int lineIndex = i * _heightSize;
            int nextLineIndex = (i + 1) * _heightSize;
            for (int j = 0; j < rows; j++) {
                *index++ = lineIndex + j;
                *index++ = nextLineIndex + j;
                *index++ = nextLineIndex + j + 1;
                
                *index++ = nextLineIndex + j + 1;
                *index++ = lineIndex + j + 1;
                *index++ = lineIndex + j;
            }
        }
        
        bufferPair.second = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        bufferPair.second.create();
        bufferPair.second.bind();
        bufferPair.second.allocate(indices.constData(), indexCount * sizeof(int));
        
    } else {
        bufferPair.first.bind();
        bufferPair.second.bind();
    }
    
    HeightfieldPoint* point = 0;
    glVertexPointer(3, GL_FLOAT, sizeof(HeightfieldPoint), &point->vertex);
    glTexCoordPointer(2, GL_FLOAT, sizeof(HeightfieldPoint), &point->textureCoord);
    
    glPushMatrix();
    glTranslatef(_translation.x, _translation.y, _translation.z);
    glScalef(_scale, _scale, _scale);
    
    glBindTexture(GL_TEXTURE_2D, _heightTextureID);
    
    if (cursor) {
        glDrawRangeElements(GL_TRIANGLES, 0, vertexCount - 1, indexCount, GL_UNSIGNED_INT, 0);
    
    } else if (!_materials.isEmpty()) {
        DefaultMetavoxelRendererImplementation::getBaseHeightfieldProgram().setUniformValue(
            DefaultMetavoxelRendererImplementation::getBaseHeightScaleLocation(), 1.0f / _heightSize);
        DefaultMetavoxelRendererImplementation::getBaseHeightfieldProgram().setUniformValue(
            DefaultMetavoxelRendererImplementation::getBaseColorScaleLocation(), (float)_heightSize / innerSize);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _colorTextureID);
        
        glDrawRangeElements(GL_TRIANGLES, 0, vertexCount - 1, indexCount, GL_UNSIGNED_INT, 0);
        
        Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(true, false);
        
        glDepthFunc(GL_LEQUAL);
        glDepthMask(false);
        glEnable(GL_BLEND);
        glDisable(GL_ALPHA_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);
        
        DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().bind();
        const DefaultMetavoxelRendererImplementation::SplatLocations& locations =
            DefaultMetavoxelRendererImplementation::getSplatHeightfieldLocations();
        DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
            locations.heightScale, 1.0f / _heightSize);
        DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
            locations.textureScale, (float)_heightSize / innerSize);
        DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
            locations.splatTextureOffset, _translation.x / _scale, _translation.z / _scale);
            
        glBindTexture(GL_TEXTURE_2D, _materialTextureID);
    
        for (int i = 0; i < _materials.size(); i += SPLAT_COUNT) {
            QVector4D scalesS, scalesT;
            
            for (int j = 0; j < SPLAT_COUNT; j++) {
                glActiveTexture(GL_TEXTURE0 + SPLAT_TEXTURE_UNITS[j]);
                int index = i + j;
                if (index < _networkTextures.size()) {
                    const NetworkTexturePointer& texture = _networkTextures.at(index);
                    if (texture) {
                        MaterialObject* material = static_cast<MaterialObject*>(_materials.at(index).data());
                        scalesS[j] = _scale / material->getScaleS();
                        scalesT[j] = _scale / material->getScaleT();
                        glBindTexture(GL_TEXTURE_2D, texture->getID());    
                    } else {
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }
                } else {
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }
            const float QUARTER_STEP = 0.25f * EIGHT_BIT_MAXIMUM_RECIPROCAL;
            DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
                locations.splatTextureScalesS, scalesS);
            DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
                locations.splatTextureScalesT, scalesT);
            DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
                locations.textureValueMinima,
                (i + 1) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP, (i + 2) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP,
                (i + 3) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP, (i + 4) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP);
            DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
                locations.textureValueMaxima,
                (i + 1) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP, (i + 2) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP,
                (i + 3) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP, (i + 4) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP);
            glDrawRangeElements(GL_TRIANGLES, 0, vertexCount - 1, indexCount, GL_UNSIGNED_INT, 0);
        }
    
        for (int i = 0; i < SPLAT_COUNT; i++) {
            glActiveTexture(GL_TEXTURE0 + SPLAT_TEXTURE_UNITS[i]);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        glActiveTexture(GL_TEXTURE0);
        
        glDisable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_ALPHA_TEST);
        glDisable(GL_BLEND);
        glDepthMask(true);
        glDepthFunc(GL_LESS);
        
        Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(true, true);
        
        DefaultMetavoxelRendererImplementation::getBaseHeightfieldProgram().bind();
        
    } else {
        DefaultMetavoxelRendererImplementation::getBaseHeightfieldProgram().setUniformValue(
            DefaultMetavoxelRendererImplementation::getBaseHeightScaleLocation(), 1.0f / _heightSize);
        DefaultMetavoxelRendererImplementation::getBaseHeightfieldProgram().setUniformValue(
            DefaultMetavoxelRendererImplementation::getBaseColorScaleLocation(), (float)_heightSize / innerSize);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _colorTextureID);
        
        glDrawRangeElements(GL_TRIANGLES, 0, vertexCount - 1, indexCount, GL_UNSIGNED_INT, 0);
        
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0); 
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glPopMatrix();
    
    bufferPair.first.release();
    bufferPair.second.release();
}

QHash<int, HeightfieldBuffer::BufferPair> HeightfieldBuffer::_bufferPairs;

void HeightfieldPreview::render(const glm::vec3& translation, float scale) const {
    Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(true, true);
    
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_EQUAL, 0.0f);
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    DefaultMetavoxelRendererImplementation::getBaseHeightfieldProgram().bind();
    
    glPushMatrix();
    glTranslatef(translation.x, translation.y, translation.z);
    glScalef(scale, scale, scale);
    
    foreach (const BufferDataPointer& buffer, _buffers) {
        buffer->render();
    }
    
    glPopMatrix();
    
    DefaultMetavoxelRendererImplementation::getBaseHeightfieldProgram().release();
    
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    
    Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(true, false);
}

void VoxelPoint::setNormal(const glm::vec3& normal) {
    this->normal[0] = (char)(normal.x * 127.0f);
    this->normal[1] = (char)(normal.y * 127.0f);
    this->normal[2] = (char)(normal.z * 127.0f);
}

VoxelBuffer::VoxelBuffer(const QVector<VoxelPoint>& vertices, const QVector<int>& indices, const QVector<glm::vec3>& hermite,
        const QMultiHash<QRgb, int>& quadIndices, int size, const QVector<SharedObjectPointer>& materials) :
    _vertices(vertices),
    _indices(indices),
    _hermite(hermite),
    _quadIndices(quadIndices),
    _size(size),
    _vertexCount(vertices.size()),
    _indexCount(indices.size()),
    _hermiteCount(hermite.size()),
    _indexBuffer(QOpenGLBuffer::IndexBuffer),
    _materials(materials) {
}

static bool findRayTriangleIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& distance) {
    glm::vec3 firstSide = v0 - v1;
    glm::vec3 secondSide = v2 - v1;
    glm::vec3 normal = glm::cross(secondSide, firstSide);
    float dividend = glm::dot(normal, v1) - glm::dot(origin, normal);
    if (dividend > 0.0f) {
        return false; // origin below plane
    }
    float divisor = glm::dot(normal, direction);
    if (divisor > -EPSILON) {
        return false;
    }
    float t = dividend / divisor;
    glm::vec3 point = origin + direction * t;
    if (glm::dot(normal, glm::cross(point - v1, firstSide)) > 0.0f &&
            glm::dot(normal, glm::cross(secondSide, point - v1)) > 0.0f &&
            glm::dot(normal, glm::cross(point - v0, v2 - v0)) > 0.0f) {
        distance = t;
        return true;
    }
    return false;
}

bool VoxelBuffer::findFirstRayIntersection(const glm::vec3& entry, const glm::vec3& origin,
        const glm::vec3& direction, float& distance) const {
    float highest = _size - 1.0f;
    glm::vec3 position = entry * highest;
    glm::vec3 floors = glm::floor(position);
    int max = _size - 2;
    int x = qMin((int)floors.x, max), y = qMin((int)floors.y, max), z = qMin((int)floors.z, max);
    forever {
        for (QMultiHash<QRgb, int>::const_iterator it = _quadIndices.constFind(qRgb(x + 1, y + 1, z + 1));
                it != _quadIndices.constEnd(); it++) {
            const int* indices = _indices.constData() + *it;
            if (findRayTriangleIntersection(origin, direction, _vertices.at(indices[0]).vertex,
                    _vertices.at(indices[1]).vertex, _vertices.at(indices[2]).vertex, distance) ||
                findRayTriangleIntersection(origin, direction, _vertices.at(indices[0]).vertex,
                    _vertices.at(indices[2]).vertex, _vertices.at(indices[3]).vertex, distance)) {
                return true;
            }
        }
        float xDistance = FLT_MAX, yDistance = FLT_MAX, zDistance = FLT_MAX;
        if (direction.x > 0.0f) {
            xDistance = (x + 1.0f - position.x) / direction.x;
        } else if (direction.x < 0.0f) {
            xDistance = (x - position.x) / direction.x;
        }
        if (direction.y > 0.0f) {
            yDistance = (y + 1.0f - position.y) / direction.y;
        } else if (direction.y < 0.0f) {
            yDistance = (y - position.y) / direction.y;
        }
        if (direction.z > 0.0f) {
            zDistance = (z + 1.0f - position.z) / direction.z;
        } else if (direction.z < 0.0f) {
            zDistance = (z - position.z) / direction.z;
        }
        float minimumDistance = qMin(xDistance, qMin(yDistance, zDistance));
        if (minimumDistance == xDistance) {
            if (direction.x > 0.0f) {
                if (x++ == max) {
                    return false;
                }
            } else if (x-- == 0) {
                return false;
            }
        }
        if (minimumDistance == yDistance) {
            if (direction.y > 0.0f) {
                if (y++ == max) {
                    return false;
                }
            } else if (y-- == 0) {
                return false;
            }
        }
        if (minimumDistance == zDistance) {
            if (direction.z > 0.0f) {
                if (z++ == max) {
                    return false;
                }
            } else if (z-- == 0) {
                return false;
            }
        }
        position += direction * minimumDistance;
    }
    return false;
}

void VoxelBuffer::render(bool cursor) {
    if (!_vertexBuffer.isCreated()) {
        _vertexBuffer.create();
        _vertexBuffer.bind();
        _vertexBuffer.allocate(_vertices.constData(), _vertices.size() * sizeof(VoxelPoint));
        
        _indexBuffer.create();
        _indexBuffer.bind();
        _indexBuffer.allocate(_indices.constData(), _indices.size() * sizeof(int));
        
        if (!_materials.isEmpty()) {
            _networkTextures.resize(_materials.size());
            for (int i = 0; i < _materials.size(); i++) {
                const SharedObjectPointer material = _materials.at(i);
                if (material) {
                    _networkTextures[i] = Application::getInstance()->getTextureCache()->getTexture(
                        static_cast<MaterialObject*>(material.data())->getDiffuse(), SPLAT_TEXTURE);
                }
            }
        }
    } else {
        _vertexBuffer.bind();
        _indexBuffer.bind();
    }
    
    VoxelPoint* point = 0;
    glVertexPointer(3, GL_FLOAT, sizeof(VoxelPoint), &point->vertex);
    glColorPointer(3, GL_UNSIGNED_BYTE, sizeof(VoxelPoint), &point->color);
    glNormalPointer(GL_BYTE, sizeof(VoxelPoint), &point->normal);
    
    glDrawRangeElements(GL_QUADS, 0, _vertexCount - 1, _indexCount, GL_UNSIGNED_INT, 0);
    
    if (!(_materials.isEmpty() || cursor)) {
        Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(true, false);
        
        glDepthFunc(GL_LEQUAL);
        glDepthMask(false);
        glEnable(GL_BLEND);
        glDisable(GL_ALPHA_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);
        
        DefaultMetavoxelRendererImplementation::getSplatVoxelProgram().bind();
        const DefaultMetavoxelRendererImplementation::SplatLocations& locations =
            DefaultMetavoxelRendererImplementation::getSplatVoxelLocations();
        
        DefaultMetavoxelRendererImplementation::getSplatVoxelProgram().setAttributeBuffer(locations.materials,
            GL_UNSIGNED_BYTE, (qint64)&point->materials, SPLAT_COUNT, sizeof(VoxelPoint));
        DefaultMetavoxelRendererImplementation::getSplatVoxelProgram().enableAttributeArray(locations.materials);
        
        DefaultMetavoxelRendererImplementation::getSplatVoxelProgram().setAttributeBuffer(locations.materialWeights,
            GL_UNSIGNED_BYTE, (qint64)&point->materialWeights, SPLAT_COUNT, sizeof(VoxelPoint));
        DefaultMetavoxelRendererImplementation::getSplatVoxelProgram().enableAttributeArray(locations.materialWeights);
        
        for (int i = 0; i < _materials.size(); i += SPLAT_COUNT) {
            QVector4D scalesS, scalesT;
            
            for (int j = 0; j < SPLAT_COUNT; j++) {
                glActiveTexture(GL_TEXTURE0 + SPLAT_TEXTURE_UNITS[j]);
                int index = i + j;
                if (index < _networkTextures.size()) {
                    const NetworkTexturePointer& texture = _networkTextures.at(index);
                    if (texture) {
                        MaterialObject* material = static_cast<MaterialObject*>(_materials.at(index).data());
                        scalesS[j] = 1.0f / material->getScaleS();
                        scalesT[j] = 1.0f / material->getScaleT();
                        glBindTexture(GL_TEXTURE_2D, texture->getID());    
                    } else {
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }
                } else {
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }
            const float QUARTER_STEP = 0.25f * EIGHT_BIT_MAXIMUM_RECIPROCAL;
            DefaultMetavoxelRendererImplementation::getSplatVoxelProgram().setUniformValue(
                locations.splatTextureScalesS, scalesS);
            DefaultMetavoxelRendererImplementation::getSplatVoxelProgram().setUniformValue(
                locations.splatTextureScalesT, scalesT);
            DefaultMetavoxelRendererImplementation::getSplatVoxelProgram().setUniformValue(
                locations.textureValueMinima,
                (i + 1) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP, (i + 2) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP,
                (i + 3) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP, (i + 4) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP);
            DefaultMetavoxelRendererImplementation::getSplatVoxelProgram().setUniformValue(
                locations.textureValueMaxima,
                (i + 1) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP, (i + 2) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP,
                (i + 3) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP, (i + 4) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP);
            glDrawRangeElements(GL_QUADS, 0, _vertexCount - 1, _indexCount, GL_UNSIGNED_INT, 0);
        }
    
        for (int i = 0; i < SPLAT_COUNT; i++) {
            glActiveTexture(GL_TEXTURE0 + SPLAT_TEXTURE_UNITS[i]);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    
        glActiveTexture(GL_TEXTURE0);
        
        glDisable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_ALPHA_TEST);
        glDisable(GL_BLEND);
        glDepthMask(true);
        glDepthFunc(GL_LESS);
        
        Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(true, true);
        
        DefaultMetavoxelRendererImplementation::getSplatVoxelProgram().disableAttributeArray(locations.materials);
        DefaultMetavoxelRendererImplementation::getSplatVoxelProgram().disableAttributeArray(locations.materialWeights);
        
        DefaultMetavoxelRendererImplementation::getBaseVoxelProgram().bind();
    }
    
    _vertexBuffer.release();
    _indexBuffer.release();
    
    if (_hermiteCount > 0 && Menu::getInstance()->isOptionChecked(MenuOption::DisplayHermiteData) && !cursor) {
        if (!_hermiteBuffer.isCreated()) {
            _hermiteBuffer.create();
            _hermiteBuffer.bind();
            _hermiteBuffer.allocate(_hermite.constData(), _hermite.size() * sizeof(glm::vec3));
            _hermite.clear();
        
        } else {
            _hermiteBuffer.bind();
        }
        
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        
        glVertexPointer(3, GL_FLOAT, 0, 0);
        
        Application::getInstance()->getDeferredLightingEffect()->getSimpleProgram().bind();
        
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glNormal3f(0.0f, 1.0f, 0.0f);
        
        glLineWidth(1.0f);
        
        glDrawArrays(GL_LINES, 0, _hermiteCount);
        
        DefaultMetavoxelRendererImplementation::getBaseVoxelProgram().bind();
        
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        
        _hermiteBuffer.release();
    }
}

BufferDataAttribute::BufferDataAttribute(const QString& name) :
    InlineAttribute<BufferDataPointer>(name) {
}

bool BufferDataAttribute::merge(void*& parent, void* children[], bool postRead) const {
    *(BufferDataPointer*)&parent = _defaultValue;
    for (int i = 0; i < MERGE_COUNT; i++) {
        if (decodeInline<BufferDataPointer>(children[i])) {
            return false;
        }
    }
    return true;
}

AttributeValue BufferDataAttribute::inherit(const AttributeValue& parentValue) const {
    return AttributeValue(parentValue.getAttribute());
}

void DefaultMetavoxelRendererImplementation::init() {
    if (!_pointProgram.isLinked()) {
        _pointProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/metavoxel_point.vert");
        _pointProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/metavoxel_voxel_base.frag");
        _pointProgram.link();
       
        _pointProgram.bind();
        _pointScaleLocation = _pointProgram.uniformLocation("pointScale");
        _pointProgram.release();
        
        _baseHeightfieldProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/metavoxel_heightfield_base.vert");
        _baseHeightfieldProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/metavoxel_heightfield_base.frag");
        _baseHeightfieldProgram.link();
        
        _baseHeightfieldProgram.bind();
        _baseHeightfieldProgram.setUniformValue("heightMap", 0);
        _baseHeightfieldProgram.setUniformValue("diffuseMap", 1);
        _baseHeightScaleLocation = _baseHeightfieldProgram.uniformLocation("heightScale");
        _baseColorScaleLocation = _baseHeightfieldProgram.uniformLocation("colorScale");
        _baseHeightfieldProgram.release();
        
        loadSplatProgram("heightfield", _splatHeightfieldProgram, _splatHeightfieldLocations);
        
        _heightfieldCursorProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/metavoxel_heightfield_cursor.vert");
        _heightfieldCursorProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/metavoxel_cursor.frag");
        _heightfieldCursorProgram.link();
        
        _heightfieldCursorProgram.bind();
        _heightfieldCursorProgram.setUniformValue("heightMap", 0);
        _heightfieldCursorProgram.release();
        
        _baseVoxelProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/metavoxel_voxel_base.vert");
        _baseVoxelProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/metavoxel_voxel_base.frag");
        _baseVoxelProgram.link();
        
        loadSplatProgram("voxel", _splatVoxelProgram, _splatVoxelLocations);
        
        _voxelCursorProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/metavoxel_voxel_cursor.vert");
        _voxelCursorProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/metavoxel_cursor.frag");
        _voxelCursorProgram.link();
    }
}

DefaultMetavoxelRendererImplementation::DefaultMetavoxelRendererImplementation() {
}

class PointAugmentVisitor : public MetavoxelVisitor {
public:

    PointAugmentVisitor(const MetavoxelLOD& lod);
    
    virtual void prepare(MetavoxelData* data);
    virtual int visit(MetavoxelInfo& info);
    virtual bool postVisit(MetavoxelInfo& info);

private:
    
    BufferPointVector _points;
    float _pointLeafSize;
};

PointAugmentVisitor::PointAugmentVisitor(const MetavoxelLOD& lod) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getColorAttribute() <<
        AttributeRegistry::getInstance()->getNormalAttribute(), QVector<AttributePointer>() <<
            Application::getInstance()->getMetavoxels()->getPointBufferAttribute(), lod) {
}

const int ALPHA_RENDER_THRESHOLD = 0;

void PointAugmentVisitor::prepare(MetavoxelData* data) {
    MetavoxelVisitor::prepare(data);
    const float MAX_POINT_LEAF_SIZE = 64.0f;
    _pointLeafSize = qMin(data->getSize(), MAX_POINT_LEAF_SIZE);
}

int PointAugmentVisitor::visit(MetavoxelInfo& info) {
    if (!info.isLeaf) {
        return (info.size > _pointLeafSize) ? DEFAULT_ORDER : (DEFAULT_ORDER | ALL_NODES_REST);    
    }
    QRgb color = info.inputValues.at(0).getInlineValue<QRgb>();
    quint8 alpha = qAlpha(color);
    if (alpha > ALPHA_RENDER_THRESHOLD) {
        QRgb normal = info.inputValues.at(1).getInlineValue<QRgb>();
        BufferPoint point = { glm::vec4(info.minimum + glm::vec3(info.size, info.size, info.size) * 0.5f, info.size),
            { quint8(qRed(color)), quint8(qGreen(color)), quint8(qBlue(color)) }, 
            { quint8(qRed(normal)), quint8(qGreen(normal)), quint8(qBlue(normal)) } };
        _points.append(point);
    }
    if (info.size >= _pointLeafSize) {
        PointBuffer* buffer = NULL;
        if (!_points.isEmpty()) { 
            BufferPointVector swapPoints;
            _points.swap(swapPoints);
            buffer = new PointBuffer(swapPoints);
        }
        BufferDataPointer pointer(buffer);
        info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline(pointer));
    }
    return STOP_RECURSION;
}

bool PointAugmentVisitor::postVisit(MetavoxelInfo& info) {
    if (info.size != _pointLeafSize) {
        return false;
    }
    PointBuffer* buffer = NULL;
    if (!_points.isEmpty()) {
        BufferPointVector swapPoints;
        _points.swap(swapPoints);
        buffer = new PointBuffer(swapPoints);
    }
    BufferDataPointer pointer(buffer);
    info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline(pointer));
    return true;
}

class HeightfieldFetchVisitor : public MetavoxelVisitor {
public:
    
    HeightfieldFetchVisitor(const MetavoxelLOD& lod, const QVector<Box>& intersections);
    
    void init(HeightfieldBuffer* buffer);
    
    virtual int visit(MetavoxelInfo& info);
    virtual bool postVisit(MetavoxelInfo& info);

private:
    
    const QVector<Box>& _intersections;
    HeightfieldBuffer* _buffer;
    
    QVector<int> _depthFlags;
};

enum DepthFlags { HEIGHT_FLAG = 0x01, COLOR_FLAG = 0x02, MATERIAL_FLAG = 0x04 };

HeightfieldFetchVisitor::HeightfieldFetchVisitor(const MetavoxelLOD& lod, const QVector<Box>& intersections) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getHeightfieldAttribute() <<
        AttributeRegistry::getInstance()->getHeightfieldColorAttribute() <<
        AttributeRegistry::getInstance()->getHeightfieldMaterialAttribute(), QVector<AttributePointer>(), lod),
    _intersections(intersections) {
}

void HeightfieldFetchVisitor::init(HeightfieldBuffer* buffer) {
    _buffer = buffer;
}

int HeightfieldFetchVisitor::visit(MetavoxelInfo& info) {
    if (!info.getBounds().intersects(_buffer->getHeightBounds())) {
        return STOP_RECURSION;
    }
    if (_depthFlags.size() > _depth) {
        _depthFlags[_depth] = 0;
    } else {
        _depthFlags.append(0);
    }
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    postVisit(info);
    return STOP_RECURSION;
}

bool HeightfieldFetchVisitor::postVisit(MetavoxelInfo& info) {
    HeightfieldHeightDataPointer height = info.inputValues.at(0).getInlineValue<HeightfieldHeightDataPointer>();
    int flags = _depthFlags.at(_depth);
    if (height) {
        // to handle borders correctly, make sure we only sample nodes with resolution <= ours
        int heightSize = glm::sqrt((float)height->getContents().size());
        float heightIncrement = info.size / heightSize;
        if (heightIncrement < _buffer->getHeightIncrement() || (flags & HEIGHT_FLAG)) {
            height.reset();
        } else {
            flags |= HEIGHT_FLAG;
        }
    }
    HeightfieldColorDataPointer color = info.inputValues.at(1).getInlineValue<HeightfieldColorDataPointer>();
    if (color) {
        int colorSize = glm::sqrt((float)color->getContents().size() / DataBlock::COLOR_BYTES);
        float colorIncrement = info.size / colorSize;
        if (colorIncrement < _buffer->getColorIncrement() || (flags & COLOR_FLAG)) {
            color.reset();
        } else {
            flags |= COLOR_FLAG;
        }
    }
    HeightfieldMaterialDataPointer material = info.inputValues.at(2).getInlineValue<HeightfieldMaterialDataPointer>();
    if (material) {
        int materialSize = glm::sqrt((float)material->getContents().size());
        float materialIncrement = info.size / materialSize;
        if (materialIncrement < _buffer->getMaterialIncrement() || (flags & MATERIAL_FLAG)) {
            material.reset();
        } else {
            flags |= MATERIAL_FLAG;
        }
    }
    if (_depth > 0) {
        _depthFlags[_depth - 1] |= flags;
    }
    if (!(height || color || material)) {
        return false;
    }
    Box bounds = info.getBounds();
    foreach (const Box& intersection, _intersections) {
        Box overlap = intersection.getIntersection(bounds);
        if (overlap.isEmpty()) {
            continue;
        }
        if (height) {
            float heightIncrement = _buffer->getHeightIncrement();
            const Box& heightBounds = _buffer->getHeightBounds();
            int destX = (overlap.minimum.x - heightBounds.minimum.x) / heightIncrement;
            int destY = (overlap.minimum.z - heightBounds.minimum.z) / heightIncrement;
            int destWidth = glm::ceil((overlap.maximum.x - overlap.minimum.x) / heightIncrement);
            int destHeight = glm::ceil((overlap.maximum.z - overlap.minimum.z) / heightIncrement);
            int heightSize = _buffer->getHeightSize();
            char* dest = _buffer->getHeight().data() + destY * heightSize + destX;
            
            const QByteArray& srcHeight = height->getContents();
            int srcSize = glm::sqrt((float)srcHeight.size());
            float srcIncrement = info.size / srcSize;
            
            if (info.size == _buffer->getScale() && srcSize == (heightSize - HeightfieldBuffer::HEIGHT_EXTENSION)) {
                // easy case: same resolution
                int srcX = (overlap.minimum.x - info.minimum.x) / srcIncrement;
                int srcY = (overlap.minimum.z - info.minimum.z) / srcIncrement;
                
                const char* src = srcHeight.constData() + srcY * srcSize + srcX;
                for (int y = 0; y < destHeight; y++, src += srcSize, dest += heightSize) {
                    memcpy(dest, src, destWidth);
                }
            } else {
                // more difficult: different resolutions
                float srcX = (overlap.minimum.x - info.minimum.x) / srcIncrement;
                float srcY = (overlap.minimum.z - info.minimum.z) / srcIncrement;
                float srcAdvance = heightIncrement / srcIncrement;
                int shift = 0;
                float size = _buffer->getScale();
                while (size < info.size) {
                    shift++;
                    size *= 2.0f;
                }
                int subtract = (_buffer->getTranslation().y - info.minimum.y) * EIGHT_BIT_MAXIMUM / _buffer->getScale();
                for (int y = 0; y < destHeight; y++, dest += heightSize, srcY += srcAdvance) {
                    const uchar* src = (const uchar*)srcHeight.constData() + (int)srcY * srcSize;
                    float lineSrcX = srcX;
                    for (char* lineDest = dest, *end = dest + destWidth; lineDest != end; lineDest++, lineSrcX += srcAdvance) {
                        *lineDest = qMin(qMax(0, (src[(int)lineSrcX] << shift) - subtract), EIGHT_BIT_MAXIMUM);
                    }
                }
            }
        }
        if (color) {
            const Box& colorBounds = _buffer->getColorBounds();
            overlap = colorBounds.getIntersection(overlap);
            float colorIncrement = _buffer->getColorIncrement();
            int destX = (overlap.minimum.x - colorBounds.minimum.x) / colorIncrement;
            int destY = (overlap.minimum.z - colorBounds.minimum.z) / colorIncrement;
            int destWidth = glm::ceil((overlap.maximum.x - overlap.minimum.x) / colorIncrement);
            int destHeight = glm::ceil((overlap.maximum.z - overlap.minimum.z) / colorIncrement);
            int colorSize = _buffer->getColorSize();
            char* dest = _buffer->getColor().data() + (destY * colorSize + destX) * DataBlock::COLOR_BYTES;
            int destStride = colorSize * DataBlock::COLOR_BYTES;
            int destBytes = destWidth * DataBlock::COLOR_BYTES;
            
            const QByteArray& srcColor = color->getContents();
            int srcSize = glm::sqrt(float(srcColor.size() / DataBlock::COLOR_BYTES));
            int srcStride = srcSize * DataBlock::COLOR_BYTES;
            float srcIncrement = info.size / srcSize;
            
            if (srcIncrement == colorIncrement) {
                // easy case: same resolution
                int srcX = (overlap.minimum.x - info.minimum.x) / srcIncrement;
                int srcY = (overlap.minimum.z - info.minimum.z) / srcIncrement;
                
                const char* src = srcColor.constData() + (srcY * srcSize + srcX) * DataBlock::COLOR_BYTES;    
                for (int y = 0; y < destHeight; y++, src += srcStride, dest += destStride) {
                    memcpy(dest, src, destBytes);
                }
            } else {
                // more difficult: different resolutions
                float srcX = (overlap.minimum.x - info.minimum.x) / srcIncrement;
                float srcY = (overlap.minimum.z - info.minimum.z) / srcIncrement;
                float srcAdvance = colorIncrement / srcIncrement;
                for (int y = 0; y < destHeight; y++, dest += destStride, srcY += srcAdvance) {
                    const char* src = srcColor.constData() + (int)srcY * srcStride;
                    float lineSrcX = srcX;
                    for (char* lineDest = dest, *end = dest + destBytes; lineDest != end; lineDest += DataBlock::COLOR_BYTES,
                            lineSrcX += srcAdvance) {
                        const char* lineSrc = src + (int)lineSrcX * DataBlock::COLOR_BYTES;
                        lineDest[0] = lineSrc[0];
                        lineDest[1] = lineSrc[1];
                        lineDest[2] = lineSrc[2];
                    }
                }
            }
        }
        if (material) {
            const Box& materialBounds = _buffer->getMaterialBounds();
            overlap = materialBounds.getIntersection(overlap);
            float materialIncrement = _buffer->getMaterialIncrement();
            int destX = (overlap.minimum.x - materialBounds.minimum.x) / materialIncrement;
            int destY = (overlap.minimum.z - materialBounds.minimum.z) / materialIncrement;
            int destWidth = glm::ceil((overlap.maximum.x - overlap.minimum.x) / materialIncrement);
            int destHeight = glm::ceil((overlap.maximum.z - overlap.minimum.z) / materialIncrement);
            int materialSize = _buffer->getMaterialSize();
            char* dest = _buffer->getMaterial().data() + destY * materialSize + destX;
            
            const QByteArray& srcMaterial = material->getContents();
            const QVector<SharedObjectPointer> srcMaterials = material->getMaterials();
            int srcSize = glm::sqrt((float)srcMaterial.size());
            float srcIncrement = info.size / srcSize;
            QHash<int, int> materialMappings;
            
            if (srcIncrement == materialIncrement) {
                // easy case: same resolution
                int srcX = (overlap.minimum.x - info.minimum.x) / srcIncrement;
                int srcY = (overlap.minimum.z - info.minimum.z) / srcIncrement;
                
                const uchar* src = (const uchar*)srcMaterial.constData() + srcY * srcSize + srcX;
                for (int y = 0; y < destHeight; y++, src += srcSize, dest += materialSize) {
                    const uchar* lineSrc = src;
                    for (char* lineDest = dest, *end = dest + destWidth; lineDest != end; lineDest++, lineSrc++) {
                        int value = *lineSrc;
                        if (value != 0) {
                            int& mapping = materialMappings[value];
                            if (mapping == 0) {
                                mapping = getMaterialIndex(material->getMaterials().at(value - 1),
                                    _buffer->getMaterials(), _buffer->getMaterial());
                            }
                            value = mapping;
                        }
                        *lineDest = value;
                    }
                }
            } else {
                // more difficult: different resolutions
                float srcX = (overlap.minimum.x - info.minimum.x) / srcIncrement;
                float srcY = (overlap.minimum.z - info.minimum.z) / srcIncrement;
                float srcAdvance = materialIncrement / srcIncrement;
                for (int y = 0; y < destHeight; y++, dest += materialSize, srcY += srcAdvance) {
                    const uchar* src = (const uchar*)srcMaterial.constData() + (int)srcY * srcSize;
                    float lineSrcX = srcX;
                    for (char* lineDest = dest, *end = dest + destWidth; lineDest != end; lineDest++, lineSrcX += srcAdvance) {
                        int value = src[(int)lineSrcX];
                        if (value != 0) {
                            int& mapping = materialMappings[value];
                            if (mapping == 0) {
                                mapping = getMaterialIndex(material->getMaterials().at(value - 1),
                                    _buffer->getMaterials(), _buffer->getMaterial());
                            }
                            value = mapping;
                        }
                        *lineDest = value;
                    }
                }
            }
            clearUnusedMaterials(_buffer->getMaterials(), _buffer->getMaterial());
        }
    }
    return false;
}

class HeightfieldRegionVisitor : public MetavoxelVisitor {
public:
    
    QVector<Box> regions;
    Box regionBounds;

    HeightfieldRegionVisitor(const MetavoxelLOD& lod);
    
    virtual int visit(MetavoxelInfo& info);
    virtual bool postVisit(MetavoxelInfo& info);

private:
    
    void addRegion(const Box& unextended, const Box& extended);
    
    int _heightSize;
    int _heightDepth;
    
    int _inheritedColorSize;
    int _inheritedColorDepth;
    
    int _containedColorSize;
    int _containedColorDepth;
    
    int _inheritedMaterialSize;
    int _inheritedMaterialDepth;
    
    int _containedMaterialSize;
    int _containedMaterialDepth;
    
    QVector<Box> _intersections;
    HeightfieldFetchVisitor _fetchVisitor;
};

HeightfieldRegionVisitor::HeightfieldRegionVisitor(const MetavoxelLOD& lod) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getHeightfieldAttribute() <<
        AttributeRegistry::getInstance()->getHeightfieldColorAttribute() <<
        AttributeRegistry::getInstance()->getHeightfieldMaterialAttribute() <<
        Application::getInstance()->getMetavoxels()->getHeightfieldBufferAttribute(), QVector<AttributePointer>() <<
            Application::getInstance()->getMetavoxels()->getHeightfieldBufferAttribute(), lod),
    regionBounds(glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX), glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX)),
    _heightDepth(-1),
    _inheritedColorDepth(-1),
    _containedColorDepth(-1),
    _inheritedMaterialDepth(-1),
    _containedMaterialDepth(-1),
    _fetchVisitor(lod, _intersections) {
}

static int signedLeftShift(int value, int shift) {
    return (shift > 0) ? value << shift : value >> (-shift);
}

int HeightfieldRegionVisitor::visit(MetavoxelInfo& info) {
    HeightfieldHeightDataPointer height = info.inputValues.at(0).getInlineValue<HeightfieldHeightDataPointer>();
    int order = DEFAULT_ORDER;
    if (height) {
        _heightSize = glm::sqrt((float)height->getContents().size());
        _heightDepth = _depth;
        order |= ALL_NODES_REST;
    }
    HeightfieldColorDataPointer color = info.inputValues.at(1).getInlineValue<HeightfieldColorDataPointer>();
    if (color) {
        int colorSize = glm::sqrt((float)(color->getContents().size() / DataBlock::COLOR_BYTES));
        if (_heightDepth == -1) {
            _inheritedColorSize = colorSize;
            _inheritedColorDepth = _depth;
            
        } else if (_containedColorDepth == -1 ||
                colorSize > signedLeftShift(_containedColorSize, _containedColorDepth - _depth)) {
            _containedColorSize = colorSize;
            _containedColorDepth = _depth;
        }
    }
    HeightfieldMaterialDataPointer material = info.inputValues.at(2).getInlineValue<HeightfieldMaterialDataPointer>();
    if (material) {
        int materialSize = glm::sqrt((float)material->getContents().size());       
        if (_heightDepth == -1) {
            _inheritedMaterialSize = materialSize;
            _inheritedMaterialDepth = _depth;
        
        } else if (_containedMaterialDepth == -1 ||
                materialSize > signedLeftShift(_containedMaterialSize, _containedMaterialDepth - _depth)) {
            _containedMaterialSize = materialSize;
            _containedMaterialDepth = _depth;
        }
    }
    if (!info.isLeaf) {
        return order;
    }
    postVisit(info);
    return STOP_RECURSION;
}

bool HeightfieldRegionVisitor::postVisit(MetavoxelInfo& info) {
    HeightfieldBuffer* buffer = NULL;
    if (_depth == _heightDepth) {
        int extendedHeightSize = _heightSize + HeightfieldBuffer::HEIGHT_EXTENSION;
        int heightContentsSize = extendedHeightSize * extendedHeightSize;
        
        int extendedColorSize = qMax(_inheritedColorDepth == -1 ? 0 :
            signedLeftShift(_inheritedColorSize, _inheritedColorDepth - _depth) + HeightfieldBuffer::SHARED_EDGE,
            _containedColorDepth == -1 ? 0 :
                signedLeftShift(_containedColorSize, _containedColorDepth - _depth) + HeightfieldBuffer::SHARED_EDGE);
        int colorContentsSize = extendedColorSize * extendedColorSize * DataBlock::COLOR_BYTES;
        
        int extendedMaterialSize = qMax(_inheritedMaterialDepth == -1 ? 0 :
            signedLeftShift(_inheritedMaterialSize, _inheritedMaterialDepth - _depth) + HeightfieldBuffer::SHARED_EDGE,
            _containedMaterialDepth == -1 ? 0 :
                signedLeftShift(_containedMaterialSize, _containedMaterialDepth - _depth) + HeightfieldBuffer::SHARED_EDGE);
        int materialContentsSize = extendedMaterialSize * extendedMaterialSize;
        
        const HeightfieldBuffer* existingBuffer = static_cast<const HeightfieldBuffer*>(
            info.inputValues.at(3).getInlineValue<BufferDataPointer>().data());
        Box bounds = info.getBounds();
        if (existingBuffer && existingBuffer->getHeight().size() == heightContentsSize &&
                existingBuffer->getColor().size() == colorContentsSize &&
                existingBuffer->getMaterial().size() == materialContentsSize) {
            // we already have a buffer of the correct resolution
            addRegion(bounds, existingBuffer->getHeightBounds());
            buffer = new HeightfieldBuffer(info.minimum, info.size, existingBuffer->getHeight(),
                existingBuffer->getColor(), existingBuffer->getMaterial(), existingBuffer->getMaterials());
        
        } else {
            // we must create a new buffer and update its borders
            buffer = new HeightfieldBuffer(info.minimum, info.size, QByteArray(heightContentsSize, 0),
                QByteArray(colorContentsSize, 0), QByteArray(materialContentsSize, 0));
            const Box& heightBounds = buffer->getHeightBounds();
            addRegion(bounds, heightBounds);
            
            _intersections.clear();
            _intersections.append(Box(heightBounds.minimum,
                glm::vec3(bounds.maximum.x, heightBounds.maximum.y, bounds.minimum.z)));
            _intersections.append(Box(glm::vec3(bounds.maximum.x, heightBounds.minimum.y, heightBounds.minimum.z),
                glm::vec3(heightBounds.maximum.x, heightBounds.maximum.y, bounds.maximum.z)));
            _intersections.append(Box(glm::vec3(bounds.minimum.x, heightBounds.minimum.y, bounds.maximum.z),
                heightBounds.maximum));
            _intersections.append(Box(glm::vec3(heightBounds.minimum.x, heightBounds.minimum.y, bounds.minimum.z),
                glm::vec3(bounds.minimum.x, heightBounds.maximum.y, heightBounds.maximum.z)));
            
            _fetchVisitor.init(buffer);
            _data->guide(_fetchVisitor);
        }
        _heightDepth = _containedColorDepth = _containedMaterialDepth = -1;
    }
    if (_depth == _inheritedColorDepth) {
        _inheritedColorDepth = -1;
    }
    if (_depth == _inheritedMaterialDepth) {
        _inheritedMaterialDepth = -1;
    }
    BufferDataPointer pointer(buffer);
    info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline(pointer));
    return true;
}

void HeightfieldRegionVisitor::addRegion(const Box& unextended, const Box& extended) {
    regions.append(unextended);
    regionBounds.add(extended);
}

class HeightfieldUpdateVisitor : public MetavoxelVisitor {
public:
    
    HeightfieldUpdateVisitor(const MetavoxelLOD& lod, const QVector<Box>& regions, const Box& regionBounds);

    virtual int visit(MetavoxelInfo& info);
    
private:
    
    const QVector<Box>& _regions;    
    const Box& _regionBounds;
    QVector<Box> _intersections;
    HeightfieldFetchVisitor _fetchVisitor;
};

HeightfieldUpdateVisitor::HeightfieldUpdateVisitor(const MetavoxelLOD& lod, const QVector<Box>& regions,
        const Box& regionBounds) :
    MetavoxelVisitor(QVector<AttributePointer>() <<
        Application::getInstance()->getMetavoxels()->getHeightfieldBufferAttribute(), QVector<AttributePointer>() <<
            Application::getInstance()->getMetavoxels()->getHeightfieldBufferAttribute(), lod),
    _regions(regions),
    _regionBounds(regionBounds),
    _fetchVisitor(lod, _intersections) {
}

int HeightfieldUpdateVisitor::visit(MetavoxelInfo& info) {
    if (!info.getBounds().intersects(_regionBounds)) {
        return STOP_RECURSION;
    }
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    const HeightfieldBuffer* buffer = static_cast<const HeightfieldBuffer*>(
        info.inputValues.at(0).getInlineValue<BufferDataPointer>().data());
    if (!buffer) {
        return STOP_RECURSION;
    }
    _intersections.clear();
    foreach (const Box& region, _regions) {
        if (region.intersects(buffer->getHeightBounds())) {
            _intersections.append(region.getIntersection(buffer->getHeightBounds()));
        }
    }
    if (_intersections.isEmpty()) {
        return STOP_RECURSION;
    }
    HeightfieldBuffer* newBuffer = new HeightfieldBuffer(info.minimum, info.size,
        buffer->getHeight(), buffer->getColor(), buffer->getMaterial(), buffer->getMaterials());
    _fetchVisitor.init(newBuffer);
    _data->guide(_fetchVisitor);
    BufferDataPointer pointer(newBuffer);
    info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline(pointer));
    return STOP_RECURSION;
}

class VoxelAugmentVisitor : public MetavoxelVisitor {
public:

    VoxelAugmentVisitor(const MetavoxelLOD& lod);
    
    virtual int visit(MetavoxelInfo& info);
};

VoxelAugmentVisitor::VoxelAugmentVisitor(const MetavoxelLOD& lod) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getVoxelColorAttribute() <<
        AttributeRegistry::getInstance()->getVoxelMaterialAttribute() <<
            AttributeRegistry::getInstance()->getVoxelHermiteAttribute(), QVector<AttributePointer>() <<
                Application::getInstance()->getMetavoxels()->getVoxelBufferAttribute(), lod) {
}

class EdgeCrossing {
public:
    glm::vec3 point;
    glm::vec3 normal;
    QRgb color;
    char material;
    int axis;
};

class AxisIndex {
public:
    int x, y, z;
    
    AxisIndex(int x = -1, int y = -1, int z = -1) : x(x), y(y), z(z) { }
};

static glm::vec3 safeNormalize(const glm::vec3& vector) {
    float length = glm::length(vector);
    return (length > 0.0f) ? (vector / length) : vector;
}

int VoxelAugmentVisitor::visit(MetavoxelInfo& info) {
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    BufferData* buffer = NULL;
    VoxelColorDataPointer color = info.inputValues.at(0).getInlineValue<VoxelColorDataPointer>();
    VoxelMaterialDataPointer material = info.inputValues.at(1).getInlineValue<VoxelMaterialDataPointer>();
    VoxelHermiteDataPointer hermite = info.inputValues.at(2).getInlineValue<VoxelHermiteDataPointer>();
    
    if (color && hermite) {
        QVector<VoxelPoint> vertices;
        QVector<int> indices;
        QVector<glm::vec3> hermiteSegments;
        QMultiHash<QRgb, int> quadIndices;
        
        // see http://www.frankpetterson.com/publications/dualcontour/dualcontour.pdf for a description of the
        // dual contour algorithm for generating meshes from voxel data using Hermite-tagged edges
        const QVector<QRgb>& colorContents = color->getContents();
        const QVector<QRgb>& hermiteContents = hermite->getContents();
        int size = color->getSize();
        int area = size * size;
        
        // number variables such as offset3 and alpha0 in this function correspond to cube corners, where the x, y, and z
        // components are represented as bits in the 0, 1, and 2 position, respectively; hence, alpha0 is the value at
        // the minimum x, y, and z corner and alpha7 is the value at the maximum x, y, and z
        int offset3 = size + 1;
        int offset5 = area + 1;
        int offset6 = area + size;
        int offset7 = area + size + 1;
        
        const QRgb* colorZ = colorContents.constData();
        const QRgb* hermiteData = hermiteContents.constData();
        int hermiteStride = hermite->getSize() * VoxelHermiteData::EDGE_COUNT;
        int hermiteArea = hermiteStride * hermite->getSize();
        
        const char* materialData = material ? material->getContents().constData() : NULL;
        
        // as we scan down the cube generating vertices between grid points, we remember the indices of the last
        // (element, line, section--x, y, z) so that we can connect generated vertices as quads
        int expanded = size + 1;
        QVector<AxisIndex> lineIndices(expanded);
        QVector<AxisIndex> lastLineIndices(expanded);
        QVector<AxisIndex> planeIndices(expanded * expanded);
        QVector<AxisIndex> lastPlaneIndices(expanded * expanded);
        
        const int EDGES_PER_CUBE = 12;
        EdgeCrossing crossings[EDGES_PER_CUBE];
        
        float highest = size - 1.0f;
        float scale = info.size / highest;
        const int ALPHA_OFFSET = 24;
        bool displayHermite = Menu::getInstance()->isOptionChecked(MenuOption::DisplayHermiteData);
        for (int z = 0; z < expanded; z++) {
            const QRgb* colorY = colorZ;
            for (int y = 0; y < expanded; y++) {
                AxisIndex lastIndex;
                const QRgb* colorX = colorY;
                for (int x = 0; x < expanded; x++) {
                    int alpha0 = colorX[0] >> ALPHA_OFFSET;
                    int alpha1 = alpha0, alpha2 = alpha0, alpha4 = alpha0;
                    int alphaTotal = alpha0;
                    int possibleTotal = EIGHT_BIT_MAXIMUM;
                    
                    // cubes on the edge are two-dimensional: this ensures that their vertices will be shared between
                    // neighboring blocks, which share only one layer of points
                    bool middleX = (x != 0 && x != size);
                    bool middleY = (y != 0 && y != size);
                    bool middleZ = (z != 0 && z != size);
                    if (middleZ) {
                        alphaTotal += (alpha4 = colorX[area] >> ALPHA_OFFSET);
                        possibleTotal += EIGHT_BIT_MAXIMUM;
                    }
                    
                    int alpha5 = alpha4, alpha6 = alpha4;
                    if (middleY) {
                        alphaTotal += (alpha2 = colorX[size] >> ALPHA_OFFSET);
                        possibleTotal += EIGHT_BIT_MAXIMUM;
                        
                        if (middleZ) {
                            alphaTotal += (alpha6 = colorX[offset6] >> ALPHA_OFFSET);
                            possibleTotal += EIGHT_BIT_MAXIMUM;
                        }
                    }
                    
                    int alpha3 = alpha2, alpha7 = alpha6;
                    if (middleX) {
                        alphaTotal += (alpha1 = colorX[1] >> ALPHA_OFFSET);
                        possibleTotal += EIGHT_BIT_MAXIMUM;
                        
                        if (middleY) {
                            alphaTotal += (alpha3 = colorX[offset3] >> ALPHA_OFFSET);
                            possibleTotal += EIGHT_BIT_MAXIMUM;
                            
                            if (middleZ) {
                                alphaTotal += (alpha7 = colorX[offset7] >> ALPHA_OFFSET);
                                possibleTotal += EIGHT_BIT_MAXIMUM;
                            }
                        }
                        if (middleZ) {
                            alphaTotal += (alpha5 = colorX[offset5] >> ALPHA_OFFSET);
                            possibleTotal += EIGHT_BIT_MAXIMUM;
                        }
                    }
                    if (alphaTotal == 0 || alphaTotal == possibleTotal) {
                        if (x != 0) {
                            colorX++;
                        }
                        continue; // no corners set/all corners set
                    }
                    // the terrifying conditional code that follows checks each cube edge for a crossing, gathering
                    // its properties (color, material, normal) if one is present; as before, boundary edges are excluded
                    int clampedX = qMax(x - 1, 0), clampedY = qMax(y - 1, 0), clampedZ = qMax(z - 1, 0);
                    const QRgb* hermiteBase = hermiteData + clampedZ * hermiteArea + clampedY * hermiteStride +
                        clampedX * VoxelHermiteData::EDGE_COUNT;
                    const char* materialBase = materialData ?
                        (materialData + clampedZ * area + clampedY * size + clampedX) : NULL;
                    int crossingCount = 0;
                    if (middleX) {
                        if (alpha0 != alpha1) {
                            QRgb hermite = hermiteBase[0];
                            EdgeCrossing& crossing = crossings[crossingCount++];
                            crossing.normal = unpackNormal(hermite);
                            if (alpha0 == 0) {
                                crossing.color = colorX[1];
                                crossing.material = materialBase ? materialBase[1] : 0;
                            } else {
                                crossing.color = colorX[0];
                                crossing.material = materialBase ? materialBase[0] : 0;
                            }
                            crossing.point = glm::vec3(qAlpha(hermite) * EIGHT_BIT_MAXIMUM_RECIPROCAL, 0.0f, 0.0f);
                            crossing.axis = 0;
                        }
                        if (middleY) {
                            if (alpha1 != alpha3) {
                                QRgb hermite = hermiteBase[VoxelHermiteData::EDGE_COUNT + 1];
                                EdgeCrossing& crossing = crossings[crossingCount++];
                                crossing.normal = unpackNormal(hermite);
                                if (alpha1 == 0) {
                                    crossing.color = colorX[offset3];
                                    crossing.material = materialBase ? materialBase[offset3] : 0;
                                } else {
                                    crossing.color = colorX[1];
                                    crossing.material = materialBase ? materialBase[1] : 0;
                                }
                                crossing.point = glm::vec3(1.0f, qAlpha(hermite) * EIGHT_BIT_MAXIMUM_RECIPROCAL, 0.0f);
                                crossing.axis = 1;
                            }
                            if (alpha2 != alpha3) {
                                QRgb hermite = hermiteBase[hermiteStride];
                                EdgeCrossing& crossing = crossings[crossingCount++];
                                crossing.normal = unpackNormal(hermite);
                                if (alpha2 == 0) {
                                    crossing.color = colorX[offset3];
                                    crossing.material = materialBase ? materialBase[offset3] : 0;
                                } else {
                                    crossing.color = colorX[size];
                                    crossing.material = materialBase ? materialBase[size] : 0;
                                }
                                crossing.point = glm::vec3(qAlpha(hermite) * EIGHT_BIT_MAXIMUM_RECIPROCAL, 1.0f, 0.0f);
                                crossing.axis = 0;
                            }
                            if (middleZ) {
                                if (alpha3 != alpha7) {
                                    QRgb hermite = hermiteBase[hermiteStride + VoxelHermiteData::EDGE_COUNT + 2];
                                    EdgeCrossing& crossing = crossings[crossingCount++];
                                    crossing.normal = unpackNormal(hermite);
                                    if (alpha3 == 0) {
                                        crossing.color = colorX[offset7];
                                        crossing.material = materialBase ? materialBase[offset7] : 0;
                                    } else {
                                        crossing.color = colorX[offset3];
                                        crossing.material = materialBase ? materialBase[offset3] : 0;
                                    }
                                    crossing.point = glm::vec3(1.0f, 1.0f, qAlpha(hermite) * EIGHT_BIT_MAXIMUM_RECIPROCAL);
                                    crossing.axis = 2;
                                }
                                if (alpha5 != alpha7) {
                                    QRgb hermite = hermiteBase[hermiteArea + VoxelHermiteData::EDGE_COUNT + 1];
                                    EdgeCrossing& crossing = crossings[crossingCount++];
                                    crossing.normal = unpackNormal(hermite);
                                    if (alpha5 == 0) {
                                        crossing.color = colorX[offset7];
                                        crossing.material = materialBase ? materialBase[offset7] : 0;
                                    } else {
                                        crossing.color = colorX[offset5];
                                        crossing.material = materialBase ? materialBase[offset5] : 0;
                                    }
                                    crossing.point = glm::vec3(1.0f, qAlpha(hermite) * EIGHT_BIT_MAXIMUM_RECIPROCAL, 1.0f);
                                    crossing.axis = 1;
                                }
                                if (alpha6 != alpha7) {
                                    QRgb hermite = hermiteBase[hermiteArea + hermiteStride];
                                    EdgeCrossing& crossing = crossings[crossingCount++];
                                    crossing.normal = unpackNormal(hermite);
                                    if (alpha6 == 0) {
                                        crossing.color = colorX[offset7];
                                        crossing.material = materialBase ? materialBase[offset7] : 0;
                                    } else {
                                        crossing.color = colorX[offset6];
                                        crossing.material = materialBase ? materialBase[offset6] : 0;
                                    }
                                    crossing.point = glm::vec3(qAlpha(hermite) * EIGHT_BIT_MAXIMUM_RECIPROCAL, 1.0f, 1.0f);
                                    crossing.axis = 0;
                                }
                            }
                        }
                        if (middleZ) {
                            if (alpha1 != alpha5) {
                                QRgb hermite = hermiteBase[VoxelHermiteData::EDGE_COUNT + 2];
                                EdgeCrossing& crossing = crossings[crossingCount++];
                                crossing.normal = unpackNormal(hermite);
                                if (alpha1 == 0) {
                                    crossing.color = colorX[offset5];
                                    crossing.material = materialBase ? materialBase[offset5] : 0;
                                } else {
                                    crossing.color = colorX[1];
                                    crossing.material = materialBase ? materialBase[1] : 0;
                                }
                                crossing.point = glm::vec3(1.0f, 0.0f, qAlpha(hermite) * EIGHT_BIT_MAXIMUM_RECIPROCAL);
                                crossing.axis = 2;
                            }
                            if (alpha4 != alpha5) {
                                QRgb hermite = hermiteBase[hermiteArea];
                                EdgeCrossing& crossing = crossings[crossingCount++];
                                crossing.normal = unpackNormal(hermite);
                                if (alpha4 == 0) {
                                    crossing.color = colorX[offset5];
                                    crossing.material = materialBase ? materialBase[offset5] : 0;
                                } else {
                                    crossing.color = colorX[area];
                                    crossing.material = materialBase ? materialBase[area] : 0;
                                }
                                crossing.point = glm::vec3(qAlpha(hermite) * EIGHT_BIT_MAXIMUM_RECIPROCAL, 0.0f, 1.0f);
                                crossing.axis = 0;
                            }
                        }
                    }
                    if (middleY) {
                        if (alpha0 != alpha2) {
                            QRgb hermite = hermiteBase[1];
                            EdgeCrossing& crossing = crossings[crossingCount++];
                            crossing.normal = unpackNormal(hermite);
                            if (alpha0 == 0) {
                                crossing.color = colorX[size];
                                crossing.material = materialBase ? materialBase[size] : 0;
                            } else {
                                crossing.color = colorX[0];
                                crossing.material = materialBase ? materialBase[0] : 0;
                            }
                            crossing.point = glm::vec3(0.0f, qAlpha(hermite) * EIGHT_BIT_MAXIMUM_RECIPROCAL, 0.0f);
                            crossing.axis = 1;
                        }
                        if (middleZ) {
                            if (alpha2 != alpha6) {
                                QRgb hermite = hermiteBase[hermiteStride + 2];
                                EdgeCrossing& crossing = crossings[crossingCount++];
                                crossing.normal = unpackNormal(hermite);
                                if (alpha2 == 0) {
                                    crossing.color = colorX[offset6];
                                    crossing.material = materialBase ? materialBase[offset6] : 0;
                                } else {
                                    crossing.color = colorX[size];
                                    crossing.material = materialBase ? materialBase[size] : 0;
                                }
                                crossing.point = glm::vec3(0.0f, 1.0f, qAlpha(hermite) * EIGHT_BIT_MAXIMUM_RECIPROCAL);
                                crossing.axis = 2;
                            }
                            if (alpha4 != alpha6) {
                                QRgb hermite = hermiteBase[hermiteArea + 1];
                                EdgeCrossing& crossing = crossings[crossingCount++];
                                crossing.normal = unpackNormal(hermite);
                                if (alpha4 == 0) {
                                    crossing.color = colorX[offset6];
                                    crossing.material = materialBase ? materialBase[offset6] : 0;
                                } else {
                                    crossing.color = colorX[area];
                                    crossing.material = materialBase ? materialBase[area] : 0;
                                }
                                crossing.point = glm::vec3(0.0f, qAlpha(hermite) * EIGHT_BIT_MAXIMUM_RECIPROCAL, 1.0f);
                                crossing.axis = 1;
                            }
                        }
                    }
                    if (middleZ && alpha0 != alpha4) {
                        QRgb hermite = hermiteBase[2];
                        EdgeCrossing& crossing = crossings[crossingCount++];
                        crossing.normal = unpackNormal(hermite);
                        if (alpha0 == 0) {
                            crossing.color = colorX[area];
                            crossing.material = materialBase ? materialBase[area] : 0;
                        } else {
                            crossing.color = colorX[0];
                            crossing.material = materialBase ? materialBase[0] : 0;
                        }
                        crossing.point = glm::vec3(0.0f, 0.0f, qAlpha(hermite) * EIGHT_BIT_MAXIMUM_RECIPROCAL);
                        crossing.axis = 2;
                    }
                    // at present, we simply average the properties of each crossing as opposed to finding the vertex that
                    // minimizes the quadratic error function as described in the reference paper
                    glm::vec3 center;
                    glm::vec3 axisNormals[3];
                    const int MAX_MATERIALS_PER_VERTEX = 4;
                    quint8 materials[] = { 0, 0, 0, 0 };
                    glm::vec4 materialWeights;
                    float totalWeight = 0.0f;
                    int red = 0, green = 0, blue = 0;
                    for (int i = 0; i < crossingCount; i++) {
                        const EdgeCrossing& crossing = crossings[i];
                        center += crossing.point;
                        axisNormals[crossing.axis] += crossing.normal; 
                        red += qRed(crossing.color);
                        green += qGreen(crossing.color);
                        blue += qBlue(crossing.color);
                        
                        if (displayHermite) {
                            glm::vec3 start = info.minimum + (glm::vec3(clampedX, clampedY, clampedZ) +
                                crossing.point) * scale;
                            hermiteSegments.append(start);
                            hermiteSegments.append(start + crossing.normal * scale);
                        }
                        
                        // when assigning a material, search for its presence and, if not found,
                        // place it in the first empty slot
                        if (crossing.material != 0) {
                            for (int j = 0; j < MAX_MATERIALS_PER_VERTEX; j++) {
                                if (materials[j] == crossing.material) {
                                    materialWeights[j] += 1.0f;
                                    totalWeight += 1.0f;
                                    break;
                                    
                                } else if (materials[j] == 0) {
                                    materials[j] = crossing.material;
                                    materialWeights[j] = 1.0f;
                                    totalWeight += 1.0f;
                                    break;
                                }
                            }
                        }
                    }
                    glm::vec3 normal = safeNormalize(axisNormals[0] + axisNormals[1] + axisNormals[2]);
                    center /= crossingCount;
                    
                    // use a sequence of Givens rotations to perform a QR decomposition
                    // see http://www.cs.rice.edu/~jwarren/papers/techreport02408.pdf
                    glm::mat4 r(0.0f);
                    glm::vec4 bottom;
                    float smallestCosNormal = 1.0f;
                    for (int i = 0; i < crossingCount; i++) {
                        const EdgeCrossing& crossing = crossings[i];
                        bottom = glm::vec4(crossing.normal, glm::dot(crossing.normal, crossing.point - center));
                        smallestCosNormal = qMin(smallestCosNormal, glm::dot(crossing.normal, normal));
                        
                        for (int j = 0; j < 4; j++) {
                            float angle = glm::atan(-bottom[j], r[j][j]);
                            float sina = glm::sin(angle);
                            float cosa = glm::cos(angle);
                            
                            for (int k = 0; k < 4; k++) {
                                float tmp = bottom[k];
                                bottom[k] = sina * r[k][j] + cosa * tmp;
                                r[k][j] = cosa * r[k][j] - sina * tmp;
                            }
                        }
                    }
                    
                    // extract the submatrices, form ata
                    glm::mat3 a(r);
                    glm::vec3 b(r[3]);
                    glm::mat3 atrans = glm::transpose(a);
                    glm::mat3 ata = atrans * a;
                    
                    // find the eigenvalues and eigenvectors of ata
                    // (see http://en.wikipedia.org/wiki/Jacobi_eigenvalue_algorithm)
                    glm::mat3 d = ata;
                    glm::quat combinedRotation;
                    const int MAX_ITERATIONS = 20;
                    for (int i = 0; i < MAX_ITERATIONS; i++) {
                        glm::vec3 offDiagonals = glm::abs(glm::vec3(d[1][0], d[2][0], d[2][1]));
                        int largestIndex = (offDiagonals[0] > offDiagonals[1]) ? (offDiagonals[0] > offDiagonals[2] ? 0 : 2) :
                            (offDiagonals[1] > offDiagonals[2] ? 1 : 2);
                        const float DESIRED_PRECISION = 0.00001f;
                        if (offDiagonals[largestIndex] < DESIRED_PRECISION) {
                            break;
                        }
                        int largestJ = (largestIndex == 2) ? 1 : 0;
                        int largestI = (largestIndex == 0) ? 1 : 2; 
                        float sjj = d[largestJ][largestJ];
                        float sii = d[largestI][largestI];
                        float angle = glm::atan(2.0f * d[largestJ][largestI], sjj - sii) / 2.0f;
                        glm::quat rotation = glm::angleAxis(angle, largestIndex == 0 ? glm::vec3(0.0f, 0.0f, -1.0f) :
                            (largestIndex == 1 ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(-1.0f, 0.0f, 0.0f)));
                        combinedRotation = glm::normalize(rotation * combinedRotation);
                        glm::mat3 matrix = glm::mat3_cast(combinedRotation);
                        d = matrix * ata * glm::transpose(matrix);
                    }
                    
                    // form the singular matrix from the eigenvalues
                    const float MIN_SINGULAR_THRESHOLD = 0.1f;
                    d[0][0] = (d[0][0] < MIN_SINGULAR_THRESHOLD) ? 0.0f : 1.0f / d[0][0];
                    d[1][1] = (d[1][1] < MIN_SINGULAR_THRESHOLD) ? 0.0f : 1.0f / d[1][1];
                    d[2][2] = (d[2][2] < MIN_SINGULAR_THRESHOLD) ? 0.0f : 1.0f / d[2][2];
                    
                    // compute the pseudo-inverse, ataplus, and use to find the minimizing solution
                    glm::mat3 u = glm::mat3_cast(combinedRotation);
                    glm::mat3 ataplus = glm::transpose(u) * d * u; 
                    glm::vec3 solution = (ataplus * atrans * b) + center;
                    
                    // make sure it doesn't fall beyond the cell boundaries
                    center = glm::clamp(solution, 0.0f, 1.0f);
                    
                    if (totalWeight > 0.0f) {
                        materialWeights *= (EIGHT_BIT_MAXIMUM / totalWeight);
                    }
                    VoxelPoint point = { info.minimum + (glm::vec3(clampedX, clampedY, clampedZ) + center) * scale,
                        { (quint8)(red / crossingCount), (quint8)(green / crossingCount), (quint8)(blue / crossingCount) },
                        { (char)(normal.x * 127.0f), (char)(normal.y * 127.0f), (char)(normal.z * 127.0f) },
                        { materials[0], materials[1], materials[2], materials[3] },
                        { (quint8)materialWeights[0], (quint8)materialWeights[1], (quint8)materialWeights[2],
                            (quint8)materialWeights[3] } };
                    
                    // determine whether we must "crease" by generating directional normals
                    const float CREASE_COS_NORMAL = glm::cos(glm::radians(40.0f));
                    AxisIndex index(vertices.size(), vertices.size(), vertices.size());
                    if (smallestCosNormal > CREASE_COS_NORMAL) {    
                        vertices.append(point);
                        
                    } else {
                        axisNormals[0] = safeNormalize(axisNormals[0]);
                        axisNormals[1] = safeNormalize(axisNormals[1]);
                        axisNormals[2] = safeNormalize(axisNormals[2]);
                        glm::vec3 normalXY(safeNormalize(axisNormals[0] + axisNormals[1]));
                        glm::vec3 normalXZ(safeNormalize(axisNormals[0] + axisNormals[2]));
                        glm::vec3 normalYZ(safeNormalize(axisNormals[1] + axisNormals[2]));
                        if (glm::dot(axisNormals[0], normalXY) > CREASE_COS_NORMAL &&
                                glm::dot(axisNormals[1], normalXY) > CREASE_COS_NORMAL) {
                            point.setNormal(normalXY);
                            vertices.append(point);
                            
                            point.setNormal(axisNormals[2]);
                            index.z = vertices.size();
                            vertices.append(point);
                            
                        } else if (glm::dot(axisNormals[0], normalXZ) > CREASE_COS_NORMAL &&
                                glm::dot(axisNormals[2], normalXZ) > CREASE_COS_NORMAL) {
                            point.setNormal(normalXZ);
                            vertices.append(point);
                        
                            point.setNormal(axisNormals[1]);
                            index.y = vertices.size();
                            vertices.append(point);
                        
                        } else if (glm::dot(axisNormals[1], normalYZ) > CREASE_COS_NORMAL &&
                                glm::dot(axisNormals[2], normalYZ) > CREASE_COS_NORMAL) {
                            point.setNormal(normalYZ);
                            vertices.append(point);
                        
                            point.setNormal(axisNormals[0]);
                            index.x = vertices.size();
                            vertices.append(point);
                        
                        } else {
                            point.setNormal(axisNormals[0]);
                            vertices.append(point);
                            
                            point.setNormal(axisNormals[1]);
                            index.y = vertices.size();
                            vertices.append(point);
                            
                            point.setNormal(axisNormals[2]);
                            index.z = vertices.size();
                            vertices.append(point);
                        }
                    }
                    
                    // the first x, y, and z are repeated for the boundary edge; past that, we consider generating
                    // quads for each edge that includes a transition, using indices of previously generated vertices
                    if (x != 0 && y != 0 && z != 0) {
                        if (alpha0 != alpha1) {
                            quadIndices.insert(qRgb(x, y, z), indices.size());
                            quadIndices.insert(qRgb(x, y - 1, z), indices.size());
                            quadIndices.insert(qRgb(x, y - 1, z - 1), indices.size());
                            quadIndices.insert(qRgb(x, y, z - 1), indices.size());
                            indices.append(index.x);
                            int index1 = lastLineIndices.at(x).x;
                            int index2 = lastPlaneIndices.at((y - 1) * expanded + x).x;
                            int index3 = lastPlaneIndices.at(y * expanded + x).x;
                            if (alpha0 == 0) { // quad faces negative x
                                indices.append(index3);
                                indices.append(index2);
                                indices.append(index1);
                            } else { // quad faces positive x
                                indices.append(index1);
                                indices.append(index2);
                                indices.append(index3);
                            }
                        }
                        
                        if (alpha0 != alpha2) {
                            quadIndices.insert(qRgb(x, y, z), indices.size());
                            quadIndices.insert(qRgb(x - 1, y, z), indices.size());
                            quadIndices.insert(qRgb(x - 1, y, z - 1), indices.size());
                            quadIndices.insert(qRgb(x, y, z - 1), indices.size());
                            indices.append(index.y);
                            int index1 = lastIndex.y;
                            int index2 = lastPlaneIndices.at(y * expanded + x - 1).y;
                            int index3 = lastPlaneIndices.at(y * expanded + x).y;
                            if (alpha0 == 0) { // quad faces negative y
                                indices.append(index1);
                                indices.append(index2);
                                indices.append(index3);
                            } else { // quad faces positive y
                                indices.append(index3);
                                indices.append(index2);
                                indices.append(index1);
                            }
                        }
                        
                        if (alpha0 != alpha4) {
                            quadIndices.insert(qRgb(x, y, z), indices.size());
                            quadIndices.insert(qRgb(x - 1, y, z), indices.size());
                            quadIndices.insert(qRgb(x - 1, y - 1, z), indices.size());
                            quadIndices.insert(qRgb(x, y - 1, z), indices.size());
                            indices.append(index.z);
                            int index1 = lastIndex.z;
                            int index2 = lastLineIndices.at(x - 1).z;
                            int index3 = lastLineIndices.at(x).z;
                            if (alpha0 == 0) { // quad faces negative z
                                indices.append(index3);
                                indices.append(index2);
                                indices.append(index1);
                            } else { // quad faces positive z
                                indices.append(index1);
                                indices.append(index2);
                                indices.append(index3);
                            }
                        }
                    }
                    lastIndex = index;
                    lineIndices[x] = index;
                    planeIndices[y * expanded + x] = index;
                    
                    if (x != 0) {
                        colorX++;
                    }
                }
                lineIndices.swap(lastLineIndices);
                
                if (y != 0) {
                    colorY += size;
                }
            }
            planeIndices.swap(lastPlaneIndices);
            
            if (z != 0) {
                colorZ += area;
            }
        }
        buffer = new VoxelBuffer(vertices, indices, hermiteSegments, quadIndices, size,
            material ? material->getMaterials() : QVector<SharedObjectPointer>());
    }
    BufferDataPointer pointer(buffer);
    info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline(pointer));
    return STOP_RECURSION;
}

void DefaultMetavoxelRendererImplementation::augment(MetavoxelData& data, const MetavoxelData& previous,
        MetavoxelInfo& info, const MetavoxelLOD& lod) {
    // copy the previous buffers
    MetavoxelData expandedPrevious = previous;
    while (expandedPrevious.getSize() < data.getSize()) {
        expandedPrevious.expand();
    }
    const AttributePointer& pointBufferAttribute = Application::getInstance()->getMetavoxels()->getPointBufferAttribute();
    MetavoxelNode* root = expandedPrevious.getRoot(pointBufferAttribute);
    if (root) {
        data.setRoot(pointBufferAttribute, root);
        root->incrementReferenceCount();
    }
    const AttributePointer& heightfieldBufferAttribute =
        Application::getInstance()->getMetavoxels()->getHeightfieldBufferAttribute();
    root = expandedPrevious.getRoot(heightfieldBufferAttribute);
    if (root) {
        data.setRoot(heightfieldBufferAttribute, root);
        root->incrementReferenceCount();
    }
    const AttributePointer& voxelBufferAttribute =
        Application::getInstance()->getMetavoxels()->getVoxelBufferAttribute();
    root = expandedPrevious.getRoot(voxelBufferAttribute);
    if (root) {
        data.setRoot(voxelBufferAttribute, root);
        root->incrementReferenceCount();
    }
    
    PointAugmentVisitor pointAugmentVisitor(lod);
    data.guideToDifferent(expandedPrevious, pointAugmentVisitor);
    
    HeightfieldRegionVisitor heightfieldRegionVisitor(lod);
    data.guideToDifferent(expandedPrevious, heightfieldRegionVisitor);
    
    HeightfieldUpdateVisitor heightfieldUpdateVisitor(lod, heightfieldRegionVisitor.regions,
        heightfieldRegionVisitor.regionBounds);
    data.guide(heightfieldUpdateVisitor);
    
    VoxelAugmentVisitor voxelAugmentVisitor(lod);
    data.guideToDifferent(expandedPrevious, voxelAugmentVisitor);
}

class SpannerSimulateVisitor : public SpannerVisitor {
public:
    
    SpannerSimulateVisitor(float deltaTime, const MetavoxelLOD& lod);
    
    virtual bool visit(Spanner* spanner, const glm::vec3& clipMinimum, float clipSize);

private:
    
    float _deltaTime;
};

SpannerSimulateVisitor::SpannerSimulateVisitor(float deltaTime, const MetavoxelLOD& lod) :
    SpannerVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getSpannersAttribute(),
        QVector<AttributePointer>(), QVector<AttributePointer>(), QVector<AttributePointer>(), lod),
    _deltaTime(deltaTime) {
}

bool SpannerSimulateVisitor::visit(Spanner* spanner, const glm::vec3& clipMinimum, float clipSize) {
    spanner->getRenderer()->simulate(_deltaTime);
    return true;
}

void DefaultMetavoxelRendererImplementation::simulate(MetavoxelData& data, float deltaTime,
        MetavoxelInfo& info, const MetavoxelLOD& lod) {
    SpannerSimulateVisitor spannerSimulateVisitor(deltaTime, lod);
    data.guide(spannerSimulateVisitor);
}

class SpannerRenderVisitor : public SpannerVisitor {
public:
    
    SpannerRenderVisitor(const MetavoxelLOD& lod);
    
    virtual int visit(MetavoxelInfo& info);
    virtual bool visit(Spanner* spanner, const glm::vec3& clipMinimum, float clipSize);

private:
    
    int _containmentDepth;
};

SpannerRenderVisitor::SpannerRenderVisitor(const MetavoxelLOD& lod) :
    SpannerVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getSpannersAttribute(),
        QVector<AttributePointer>(), QVector<AttributePointer>(), QVector<AttributePointer>(),
        lod, encodeOrder(Application::getInstance()->getDisplayViewFrustum()->getDirection())),
    _containmentDepth(INT_MAX) {
}

int SpannerRenderVisitor::visit(MetavoxelInfo& info) {
    if (_containmentDepth >= _depth) {
        Frustum::IntersectionType intersection = Application::getInstance()->getMetavoxels()->getFrustum().getIntersectionType(
            info.getBounds());
        if (intersection == Frustum::NO_INTERSECTION) {
            return STOP_RECURSION;
        }
        _containmentDepth = (intersection == Frustum::CONTAINS_INTERSECTION) ? _depth : INT_MAX;
    }
    return SpannerVisitor::visit(info);
}

bool SpannerRenderVisitor::visit(Spanner* spanner, const glm::vec3& clipMinimum, float clipSize) {
    const glm::vec4 OPAQUE_WHITE(1.0f, 1.0f, 1.0f, 1.0f);
    spanner->getRenderer()->render(OPAQUE_WHITE, SpannerRenderer::DEFAULT_MODE, clipMinimum, clipSize);
    return true;
}

class BufferRenderVisitor : public MetavoxelVisitor {
public:
    
    BufferRenderVisitor(const AttributePointer& attribute);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    int _order;
    int _containmentDepth;
};

BufferRenderVisitor::BufferRenderVisitor(const AttributePointer& attribute) :
    MetavoxelVisitor(QVector<AttributePointer>() << attribute),
    _order(encodeOrder(Application::getInstance()->getDisplayViewFrustum()->getDirection())),
    _containmentDepth(INT_MAX) {
}

int BufferRenderVisitor::visit(MetavoxelInfo& info) {
    if (_containmentDepth >= _depth) {
        Frustum::IntersectionType intersection = Application::getInstance()->getMetavoxels()->getFrustum().getIntersectionType(
            info.getBounds());
        if (intersection == Frustum::NO_INTERSECTION) {
            return STOP_RECURSION;
        }
        _containmentDepth = (intersection == Frustum::CONTAINS_INTERSECTION) ? _depth : INT_MAX;
    }
    if (!info.isLeaf) {
        return _order;
    }
    BufferDataPointer buffer = info.inputValues.at(0).getInlineValue<BufferDataPointer>();
    if (buffer) {
        buffer->render();
    }
    return STOP_RECURSION;
}

void DefaultMetavoxelRendererImplementation::render(MetavoxelData& data, MetavoxelInfo& info, const MetavoxelLOD& lod) {
    SpannerRenderVisitor spannerRenderVisitor(lod);
    data.guide(spannerRenderVisitor);
    
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int VIEWPORT_WIDTH_INDEX = 2;
    const int VIEWPORT_HEIGHT_INDEX = 3;
    float viewportWidth = viewport[VIEWPORT_WIDTH_INDEX];
    float viewportHeight = viewport[VIEWPORT_HEIGHT_INDEX];
    float viewportDiagonal = sqrtf(viewportWidth * viewportWidth + viewportHeight * viewportHeight);
    float worldDiagonal = glm::distance(Application::getInstance()->getDisplayViewFrustum()->getNearBottomLeft(),
        Application::getInstance()->getDisplayViewFrustum()->getNearTopRight());

    _pointProgram.bind();
    _pointProgram.setUniformValue(_pointScaleLocation, viewportDiagonal *
        Application::getInstance()->getDisplayViewFrustum()->getNearClip() / worldDiagonal);
        
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);

    glDisable(GL_BLEND);
    
    BufferRenderVisitor pointRenderVisitor(Application::getInstance()->getMetavoxels()->getPointBufferAttribute());
    data.guide(pointRenderVisitor);
    
    glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
    
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    _pointProgram.release();
    
    Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(true, true);
    
    glEnable(GL_CULL_FACE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_EQUAL, 0.0f);
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::RenderHeightfields)) {
        _baseHeightfieldProgram.bind();
        
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        
        BufferRenderVisitor heightfieldRenderVisitor(Application::getInstance()->getMetavoxels()->getHeightfieldBufferAttribute());
        data.guide(heightfieldRenderVisitor);
        
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);    
        
        _baseHeightfieldProgram.release();
    }
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::RenderDualContourSurfaces)) {
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        
        _baseVoxelProgram.bind();
        
        BufferRenderVisitor voxelRenderVisitor(Application::getInstance()->getMetavoxels()->getVoxelBufferAttribute());
        data.guide(voxelRenderVisitor);
        
        _baseVoxelProgram.release();
        
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
    }
    
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
        
    glDisableClientState(GL_VERTEX_ARRAY);
    
    Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(true, false);
}

void DefaultMetavoxelRendererImplementation::loadSplatProgram(const char* type,
        ProgramObject& program, SplatLocations& locations) {
    program.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
        "shaders/metavoxel_" + type + "_splat.vert");
    program.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
        "shaders/metavoxel_" + type + "_splat.frag");
    program.link();
    
    program.bind();
    program.setUniformValue("heightMap", 0);
    program.setUniformValue("textureMap", 1);
    program.setUniformValueArray("diffuseMaps", SPLAT_TEXTURE_UNITS, SPLAT_COUNT);
    locations.heightScale = program.uniformLocation("heightScale");
    locations.textureScale = program.uniformLocation("textureScale");
    locations.splatTextureOffset = program.uniformLocation("splatTextureOffset");
    locations.splatTextureScalesS = program.uniformLocation("splatTextureScalesS");
    locations.splatTextureScalesT = program.uniformLocation("splatTextureScalesT");
    locations.textureValueMinima = program.uniformLocation("textureValueMinima");
    locations.textureValueMaxima = program.uniformLocation("textureValueMaxima");
    locations.materials = program.attributeLocation("materials");
    locations.materialWeights = program.attributeLocation("materialWeights");
    program.release();
}

ProgramObject DefaultMetavoxelRendererImplementation::_pointProgram;
int DefaultMetavoxelRendererImplementation::_pointScaleLocation;
ProgramObject DefaultMetavoxelRendererImplementation::_baseHeightfieldProgram;
int DefaultMetavoxelRendererImplementation::_baseHeightScaleLocation;
int DefaultMetavoxelRendererImplementation::_baseColorScaleLocation;
ProgramObject DefaultMetavoxelRendererImplementation::_splatHeightfieldProgram;
DefaultMetavoxelRendererImplementation::SplatLocations DefaultMetavoxelRendererImplementation::_splatHeightfieldLocations;
ProgramObject DefaultMetavoxelRendererImplementation::_heightfieldCursorProgram;
ProgramObject DefaultMetavoxelRendererImplementation::_baseVoxelProgram;
ProgramObject DefaultMetavoxelRendererImplementation::_splatVoxelProgram;
DefaultMetavoxelRendererImplementation::SplatLocations DefaultMetavoxelRendererImplementation::_splatVoxelLocations;
ProgramObject DefaultMetavoxelRendererImplementation::_voxelCursorProgram;

static void enableClipPlane(GLenum plane, float x, float y, float z, float w) {
    GLdouble coefficients[] = { x, y, z, w };
    glClipPlane(plane, coefficients);
    glEnable(plane);
}

void ClippedRenderer::render(const glm::vec4& color, Mode mode, const glm::vec3& clipMinimum, float clipSize) {
    if (clipSize == 0.0f) {
        renderUnclipped(color, mode);
        return;
    }
    enableClipPlane(GL_CLIP_PLANE0, -1.0f, 0.0f, 0.0f, clipMinimum.x + clipSize);
    enableClipPlane(GL_CLIP_PLANE1, 1.0f, 0.0f, 0.0f, -clipMinimum.x);
    enableClipPlane(GL_CLIP_PLANE2, 0.0f, -1.0f, 0.0f, clipMinimum.y + clipSize);
    enableClipPlane(GL_CLIP_PLANE3, 0.0f, 1.0f, 0.0f, -clipMinimum.y);
    enableClipPlane(GL_CLIP_PLANE4, 0.0f, 0.0f, -1.0f, clipMinimum.z + clipSize);
    enableClipPlane(GL_CLIP_PLANE5, 0.0f, 0.0f, 1.0f, -clipMinimum.z);
    
    renderUnclipped(color, mode);
    
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDisable(GL_CLIP_PLANE2);
    glDisable(GL_CLIP_PLANE3);
    glDisable(GL_CLIP_PLANE4);
    glDisable(GL_CLIP_PLANE5);
}

SphereRenderer::SphereRenderer() {
}

void SphereRenderer::render(const glm::vec4& color, Mode mode, const glm::vec3& clipMinimum, float clipSize) {
    if (clipSize == 0.0f) {
        renderUnclipped(color, mode);
        return;
    }
    // slight performance optimization: don't render if clip bounds are entirely within sphere
    Sphere* sphere = static_cast<Sphere*>(_spanner);
    Box clipBox(clipMinimum, clipMinimum + glm::vec3(clipSize, clipSize, clipSize));
    for (int i = 0; i < Box::VERTEX_COUNT; i++) {
        const float CLIP_PROPORTION = 0.95f;
        if (glm::distance(sphere->getTranslation(), clipBox.getVertex(i)) >= sphere->getScale() * CLIP_PROPORTION) {
            ClippedRenderer::render(color, mode, clipMinimum, clipSize);
            return;
        }
    }
}

void SphereRenderer::renderUnclipped(const glm::vec4& color, Mode mode) {
    Sphere* sphere = static_cast<Sphere*>(_spanner);
    const QColor& ownColor = sphere->getColor();
    glColor4f(ownColor.redF() * color.r, ownColor.greenF() * color.g, ownColor.blueF() * color.b, ownColor.alphaF() * color.a);
    
    glPushMatrix();
    const glm::vec3& translation = sphere->getTranslation();
    glTranslatef(translation.x, translation.y, translation.z);
    glm::quat rotation = sphere->getRotation();
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
    
    Application::getInstance()->getGeometryCache()->renderSphere(sphere->getScale(), 10, 10);
    
    glPopMatrix();
}

CuboidRenderer::CuboidRenderer() {
}

void CuboidRenderer::renderUnclipped(const glm::vec4& color, Mode mode) {
    Cuboid* cuboid = static_cast<Cuboid*>(_spanner);
    const QColor& ownColor = cuboid->getColor();
    glColor4f(ownColor.redF() * color.r, ownColor.greenF() * color.g, ownColor.blueF() * color.b, ownColor.alphaF() * color.a);
    
    glPushMatrix();
    const glm::vec3& translation = cuboid->getTranslation();
    glTranslatef(translation.x, translation.y, translation.z);
    glm::quat rotation = cuboid->getRotation();
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
    glScalef(1.0f, cuboid->getAspectY(), cuboid->getAspectZ());
    
    glutSolidCube(cuboid->getScale() * 2.0f);
    
    glPopMatrix();
}

StaticModelRenderer::StaticModelRenderer() :
    _model(new Model(this)) {
}

void StaticModelRenderer::init(Spanner* spanner) {
    SpannerRenderer::init(spanner);

    _model->init();
    
    StaticModel* staticModel = static_cast<StaticModel*>(spanner);
    applyTranslation(staticModel->getTranslation());
    applyRotation(staticModel->getRotation());
    applyScale(staticModel->getScale());
    applyURL(staticModel->getURL());
    
    connect(spanner, SIGNAL(translationChanged(const glm::vec3&)), SLOT(applyTranslation(const glm::vec3&)));
    connect(spanner, SIGNAL(rotationChanged(const glm::quat&)), SLOT(applyRotation(const glm::quat&)));
    connect(spanner, SIGNAL(scaleChanged(float)), SLOT(applyScale(float)));
    connect(spanner, SIGNAL(urlChanged(const QUrl&)), SLOT(applyURL(const QUrl&)));
}

void StaticModelRenderer::simulate(float deltaTime) {
    // update the bounds
    Box bounds;
    if (_model->isActive()) {
        const Extents& extents = _model->getGeometry()->getFBXGeometry().meshExtents;
        bounds = Box(extents.minimum, extents.maximum);
    }
    static_cast<StaticModel*>(_spanner)->setBounds(glm::translate(_model->getTranslation()) *
        glm::mat4_cast(_model->getRotation()) * glm::scale(_model->getScale()) * bounds);
    _model->simulate(deltaTime);
}

void StaticModelRenderer::renderUnclipped(const glm::vec4& color, Mode mode) {
    switch (mode) {
        case DIFFUSE_MODE:
            _model->render(color.a, Model::DIFFUSE_RENDER_MODE);
            break;
            
        case NORMAL_MODE:
            _model->render(color.a, Model::NORMAL_RENDER_MODE);
            break;
            
        default:
            _model->render(color.a);
            break;
    }
    _model->render(color.a);
}

bool StaticModelRenderer::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& clipMinimum, float clipSize, float& distance) const {
    RayIntersectionInfo info;
    info._rayStart = origin;
    info._rayDirection = direction;
    if (!_model->findRayIntersection(info)) {
        return false;
    }
    distance = info._hitDistance;
    return true;
}

void StaticModelRenderer::applyTranslation(const glm::vec3& translation) {
    _model->setTranslation(translation);
}

void StaticModelRenderer::applyRotation(const glm::quat& rotation) {
    _model->setRotation(rotation);
}

void StaticModelRenderer::applyScale(float scale) {
    const float SCALE_MULTIPLIER = 0.0006f;
    _model->setScale(glm::vec3(scale, scale, scale) * SCALE_MULTIPLIER);
}

void StaticModelRenderer::applyURL(const QUrl& url) {
    _model->setURL(url);
}
