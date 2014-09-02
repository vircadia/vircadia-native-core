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

#include <QMutexLocker>
#include <QReadLocker>
#include <QWriteLocker>
#include <QtDebug>

#include <glm/gtx/transform.hpp>

#include <SharedUtil.h>

#include <MetavoxelUtil.h>
#include <ScriptCache.h>

#include "Application.h"
#include "MetavoxelSystem.h"
#include "renderer/Model.h"

REGISTER_META_OBJECT(DefaultMetavoxelRendererImplementation)
REGISTER_META_OBJECT(SphereRenderer)
REGISTER_META_OBJECT(StaticModelRenderer)

static int bufferPointVectorMetaTypeId = qRegisterMetaType<BufferPointVector>();

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
        // the LOD threshold is temporarily tied to the avatar LOD parameter
        QWriteLocker locker(&_lodLock);
        const float BASE_LOD_THRESHOLD = 0.01f;
        _lod = MetavoxelLOD(Application::getInstance()->getCamera()->getPosition(),
            BASE_LOD_THRESHOLD * Menu::getInstance()->getAvatarLODDistanceMultiplier());
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
    ViewFrustum* viewFrustum = Application::getInstance()->getViewFrustum();
    _frustum.set(viewFrustum->getFarTopLeft(), viewFrustum->getFarTopRight(), viewFrustum->getFarBottomLeft(),
        viewFrustum->getFarBottomRight(), viewFrustum->getNearTopLeft(), viewFrustum->getNearTopRight(),
        viewFrustum->getNearBottomLeft(), viewFrustum->getNearBottomRight());
    
    RenderVisitor renderVisitor(getLOD());
    guideToAugmented(renderVisitor, true);
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

class HeightfieldCursorRenderVisitor : public MetavoxelVisitor {
public:
    
    HeightfieldCursorRenderVisitor(const Box& bounds);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    Box _bounds;
};

HeightfieldCursorRenderVisitor::HeightfieldCursorRenderVisitor(const Box& bounds) :
    MetavoxelVisitor(QVector<AttributePointer>() <<
        Application::getInstance()->getMetavoxels()->getHeightfieldBufferAttribute()),
    _bounds(bounds) {
}

int HeightfieldCursorRenderVisitor::visit(MetavoxelInfo& info) {
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
    glTexGenfv(GL_S, GL_EYE_PLANE, (const GLfloat*)&sCoefficients);
    glTexGenfv(GL_T, GL_EYE_PLANE, (const GLfloat*)&tCoefficients);
    glActiveTexture(GL_TEXTURE0);
    
    glm::vec3 extents(radius, radius, radius);
    HeightfieldCursorRenderVisitor visitor(Box(position - extents, position + extents));
    guideToAugmented(visitor);
    
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

int MetavoxelSystemClient::parseData(const QByteArray& packet) {
    // process through sequencer
    QMetaObject::invokeMethod(&_sequencer, "receivedDatagram", Q_ARG(const QByteArray&, packet));
    Application::getInstance()->getBandwidthMeter()->inputStream(BandwidthMeter::METAVOXELS).updateValue(packet.size());
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

void MetavoxelSystemClient::dataChanged(const MetavoxelData& oldData) {
    MetavoxelClient::dataChanged(oldData);
    QThreadPool::globalInstance()->start(new Augmenter(_node, _data, getAugmentedData(), _remoteDataLOD));
}

void MetavoxelSystemClient::sendDatagram(const QByteArray& data) {
    NodeList::getInstance()->writeDatagram(data, _node);
    Application::getInstance()->getBandwidthMeter()->outputStream(BandwidthMeter::METAVOXELS).updateValue(data.size());
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
    _height(height),
    _color(color),
    _material(material),
    _materials(materials),
    _heightTextureID(0),
    _colorTextureID(0),
    _materialTextureID(0),
    _heightSize(glm::sqrt(height.size())),
    _heightIncrement(scale / (_heightSize - HEIGHT_EXTENSION)),
    _colorSize(glm::sqrt(color.size() / DataBlock::COLOR_BYTES)),
    _colorIncrement(scale / (_colorSize - SHARED_EDGE)) {
    
    _heightBounds.minimum.x -= _heightIncrement * HEIGHT_BORDER;
    _heightBounds.minimum.z -= _heightIncrement * HEIGHT_BORDER;
    _heightBounds.maximum.x += _heightIncrement * (SHARED_EDGE + HEIGHT_BORDER);
    _heightBounds.maximum.z += _heightIncrement * (SHARED_EDGE + HEIGHT_BORDER);
    
    _colorBounds.maximum.x += _colorIncrement * SHARED_EDGE;
    _colorBounds.maximum.z += _colorIncrement * SHARED_EDGE;
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
    int srcSize = glm::sqrt(_height.size());
    int destSize = srcSize - 3;
    QByteArray unextended(destSize * destSize, 0);
    const char* src = _height.constData() + srcSize + 1;
    char* dest = unextended.data();
    for (int z = 0; z < destSize; z++, src += srcSize, dest += destSize) {
        memcpy(dest, src, destSize);
    }
    return unextended;
}

QByteArray HeightfieldBuffer::getUnextendedColor() const {
    int srcSize = glm::sqrt(_color.size() / DataBlock::COLOR_BYTES);
    int destSize = srcSize - 1;
    QByteArray unextended(destSize * destSize * DataBlock::COLOR_BYTES, 0);
    const char* src = _color.constData();
    int srcStride = srcSize * DataBlock::COLOR_BYTES;
    char* dest = unextended.data();
    int destStride = destSize * DataBlock::COLOR_BYTES;
    for (int z = 0; z < destSize; z++, src += srcStride, dest += destStride) {
        memcpy(dest, src, destStride);
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
            int colorSize = glm::sqrt(_color.size() / DataBlock::COLOR_BYTES);    
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, colorSize, colorSize, 0, GL_RGB, GL_UNSIGNED_BYTE, _color.constData());
        }
        
        if (!_material.isEmpty()) {
            glGenTextures(1, &_materialTextureID);
            glBindTexture(GL_TEXTURE_2D, _materialTextureID);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            int materialSize = glm::sqrt(_material.size());    
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
        DefaultMetavoxelRendererImplementation::getBaseHeightfieldProgram().bind();
        DefaultMetavoxelRendererImplementation::getBaseHeightfieldProgram().setUniformValue(
            DefaultMetavoxelRendererImplementation::getBaseHeightScaleLocation(), 1.0f / _heightSize);
        DefaultMetavoxelRendererImplementation::getBaseHeightfieldProgram().setUniformValue(
            DefaultMetavoxelRendererImplementation::getBaseColorScaleLocation(), (float)_heightSize / innerSize);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _colorTextureID);
        
        glDrawRangeElements(GL_TRIANGLES, 0, vertexCount - 1, indexCount, GL_UNSIGNED_INT, 0);
        
        glDepthFunc(GL_LEQUAL);
        glDepthMask(false);
        glEnable(GL_BLEND);
        glDisable(GL_ALPHA_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);
        
        DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().bind();
        DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
            DefaultMetavoxelRendererImplementation::getSplatHeightScaleLocation(), 1.0f / _heightSize);
        DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
            DefaultMetavoxelRendererImplementation::getSplatTextureScaleLocation(), (float)_heightSize / innerSize);
        DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
            DefaultMetavoxelRendererImplementation::getSplatTextureOffsetLocation(),
            _translation.x / _scale, _translation.z / _scale);
            
        glBindTexture(GL_TEXTURE_2D, _materialTextureID);
    
        const int TEXTURES_PER_SPLAT = 4;
        for (int i = 0; i < _materials.size(); i += TEXTURES_PER_SPLAT) {
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
                DefaultMetavoxelRendererImplementation::getSplatTextureScalesSLocation(), scalesS);
            DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
                DefaultMetavoxelRendererImplementation::getSplatTextureScalesTLocation(), scalesT);
            DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
                DefaultMetavoxelRendererImplementation::getSplatTextureValueMinimaLocation(),
                (i + 1) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP, (i + 2) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP,
                (i + 3) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP, (i + 4) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP);
            DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
                DefaultMetavoxelRendererImplementation::getSplatTextureValueMaximaLocation(),
                (i + 1) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP, (i + 2) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP,
                (i + 3) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP, (i + 4) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP);
            glDrawRangeElements(GL_TRIANGLES, 0, vertexCount - 1, indexCount, GL_UNSIGNED_INT, 0);
        }
    
        glEnable(GL_ALPHA_TEST);
        glBlendFunc(GL_DST_COLOR, GL_ZERO);
    
        for (int i = 0; i < SPLAT_COUNT; i++) {
            glActiveTexture(GL_TEXTURE0 + SPLAT_TEXTURE_UNITS[i]);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
    
        if (Menu::getInstance()->isOptionChecked(MenuOption::SimpleShadows)) {
            DefaultMetavoxelRendererImplementation::getShadowLightHeightfieldProgram().bind();
            DefaultMetavoxelRendererImplementation::getShadowLightHeightfieldProgram().setUniformValue(
                DefaultMetavoxelRendererImplementation::getShadowLightHeightScaleLocation(), 1.0f / _heightSize);
            glDrawRangeElements(GL_TRIANGLES, 0, vertexCount - 1, indexCount, GL_UNSIGNED_INT, 0);
            DefaultMetavoxelRendererImplementation::getShadowMapHeightfieldProgram().bind();
                        
        } else if (Menu::getInstance()->isOptionChecked(MenuOption::CascadedShadows)) {
            DefaultMetavoxelRendererImplementation::getCascadedShadowLightHeightfieldProgram().bind();
            DefaultMetavoxelRendererImplementation::getCascadedShadowLightHeightfieldProgram().setUniformValue(
                DefaultMetavoxelRendererImplementation::getCascadedShadowLightHeightScaleLocation(), 1.0f / _heightSize);
            glDrawRangeElements(GL_TRIANGLES, 0, vertexCount - 1, indexCount, GL_UNSIGNED_INT, 0);
            DefaultMetavoxelRendererImplementation::getCascadedShadowMapHeightfieldProgram().bind();
                    
        } else {
            DefaultMetavoxelRendererImplementation::getLightHeightfieldProgram().bind();
            DefaultMetavoxelRendererImplementation::getLightHeightfieldProgram().setUniformValue(
                DefaultMetavoxelRendererImplementation::getLightHeightScaleLocation(), 1.0f / _heightSize);
            glDrawRangeElements(GL_TRIANGLES, 0, vertexCount - 1, indexCount, GL_UNSIGNED_INT, 0);
            DefaultMetavoxelRendererImplementation::getHeightfieldProgram().bind();    
        }
        
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
        glDepthFunc(GL_LESS);
        glDepthMask(true);
        
        glActiveTexture(GL_TEXTURE0);
        
    } else {
        int heightScaleLocation = DefaultMetavoxelRendererImplementation::getHeightScaleLocation();
        int colorScaleLocation = DefaultMetavoxelRendererImplementation::getColorScaleLocation();
        ProgramObject* program = &DefaultMetavoxelRendererImplementation::getHeightfieldProgram();
        if (Menu::getInstance()->isOptionChecked(MenuOption::SimpleShadows)) {
            heightScaleLocation = DefaultMetavoxelRendererImplementation::getShadowMapHeightScaleLocation();
            colorScaleLocation = DefaultMetavoxelRendererImplementation::getShadowMapColorScaleLocation();
            program = &DefaultMetavoxelRendererImplementation::getShadowMapHeightfieldProgram();
            
        } else if (Menu::getInstance()->isOptionChecked(MenuOption::CascadedShadows)) {
            heightScaleLocation = DefaultMetavoxelRendererImplementation::getCascadedShadowMapHeightScaleLocation();
            colorScaleLocation = DefaultMetavoxelRendererImplementation::getCascadedShadowMapColorScaleLocation();
            program = &DefaultMetavoxelRendererImplementation::getCascadedShadowMapHeightfieldProgram();
        }
        program->setUniformValue(heightScaleLocation, 1.0f / _heightSize);
        program->setUniformValue(colorScaleLocation, (float)_heightSize / innerSize);
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
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_EQUAL, 0.0f);
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    DefaultMetavoxelRendererImplementation::getHeightfieldProgram().bind();
    
    glPushMatrix();
    glTranslatef(translation.x, translation.y, translation.z);
    glScalef(scale, scale, scale);
    
    foreach (const BufferDataPointer& buffer, _buffers) {
        buffer->render();
    }
    
    glPopMatrix();
    
    DefaultMetavoxelRendererImplementation::getHeightfieldProgram().release();
    
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
}

VoxelBuffer::VoxelBuffer(const QVector<VoxelPoint>& vertices, const QVector<int>& indices,
        const QVector<SharedObjectPointer>& materials) :
    _vertices(vertices),
    _indices(indices),
    _vertexCount(vertices.size()),
    _indexCount(indices.size()),
    _indexBuffer(QOpenGLBuffer::IndexBuffer),
    _materials(materials) {
}

void VoxelBuffer::render(bool cursor) {
    if (!_vertexBuffer.isCreated()) {
        _vertexBuffer.create();
        _vertexBuffer.bind();
        _vertexBuffer.allocate(_vertices.constData(), _vertices.size() * sizeof(VoxelPoint));
        _vertices.clear();
    
        _indexBuffer.create();
        _indexBuffer.bind();
        _indexBuffer.allocate(_indices.constData(), _indices.size() * sizeof(int));
        _indices.clear();
    
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
    
    _vertexBuffer.release();
    _indexBuffer.release();
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
        _pointProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() + "shaders/metavoxel_point.vert");
        _pointProgram.link();
       
        _pointProgram.bind();
        _pointScaleLocation = _pointProgram.uniformLocation("pointScale");
        _pointProgram.release();
        
        _heightfieldProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/metavoxel_heightfield.vert");
        _heightfieldProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/metavoxel_heightfield.frag");
        _heightfieldProgram.link();
        
        _heightfieldProgram.bind();
        _heightfieldProgram.setUniformValue("heightMap", 0);
        _heightfieldProgram.setUniformValue("diffuseMap", 1);
        _heightScaleLocation = _heightfieldProgram.uniformLocation("heightScale");
        _colorScaleLocation = _heightfieldProgram.uniformLocation("colorScale");
        _heightfieldProgram.release();
        
        _shadowMapHeightfieldProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/metavoxel_heightfield.vert");
        _shadowMapHeightfieldProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/metavoxel_heightfield_shadow_map.frag");
        _shadowMapHeightfieldProgram.link();
        
        _shadowMapHeightfieldProgram.bind();
        _shadowMapHeightfieldProgram.setUniformValue("heightMap", 0);
        _shadowMapHeightfieldProgram.setUniformValue("diffuseMap", 1);
        _shadowMapHeightfieldProgram.setUniformValue("shadowMap", 2);
        _shadowMapHeightScaleLocation = _shadowMapHeightfieldProgram.uniformLocation("heightScale");
        _shadowMapColorScaleLocation = _shadowMapHeightfieldProgram.uniformLocation("colorScale");
        _shadowMapHeightfieldProgram.release();
        
        _cascadedShadowMapHeightfieldProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/metavoxel_heightfield.vert");
        _cascadedShadowMapHeightfieldProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/metavoxel_heightfield_cascaded_shadow_map.frag");
        _cascadedShadowMapHeightfieldProgram.link();
        
        _cascadedShadowMapHeightfieldProgram.bind();
        _cascadedShadowMapHeightfieldProgram.setUniformValue("heightMap", 0);
        _cascadedShadowMapHeightfieldProgram.setUniformValue("diffuseMap", 1);
        _cascadedShadowMapHeightfieldProgram.setUniformValue("shadowMap", 2);
        _cascadedShadowMapHeightScaleLocation = _cascadedShadowMapHeightfieldProgram.uniformLocation("heightScale");
        _cascadedShadowMapColorScaleLocation = _cascadedShadowMapHeightfieldProgram.uniformLocation("colorScale");
        _shadowDistancesLocation = _cascadedShadowMapHeightfieldProgram.uniformLocation("shadowDistances");
        _cascadedShadowMapHeightfieldProgram.release();
        
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
        
        _splatHeightfieldProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/metavoxel_heightfield_splat.vert");
        _splatHeightfieldProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/metavoxel_heightfield_splat.frag");
        _splatHeightfieldProgram.link();
        
        _splatHeightfieldProgram.bind();
        _splatHeightfieldProgram.setUniformValue("heightMap", 0);
        _splatHeightfieldProgram.setUniformValue("textureMap", 1);
        _splatHeightfieldProgram.setUniformValueArray("diffuseMaps", SPLAT_TEXTURE_UNITS, SPLAT_COUNT);
        _splatHeightScaleLocation = _splatHeightfieldProgram.uniformLocation("heightScale");
        _splatTextureScaleLocation = _splatHeightfieldProgram.uniformLocation("textureScale");
        _splatTextureOffsetLocation = _splatHeightfieldProgram.uniformLocation("splatTextureOffset");
        _splatTextureScalesSLocation = _splatHeightfieldProgram.uniformLocation("splatTextureScalesS");
        _splatTextureScalesTLocation = _splatHeightfieldProgram.uniformLocation("splatTextureScalesT");
        _splatTextureValueMinimaLocation = _splatHeightfieldProgram.uniformLocation("textureValueMinima");
        _splatTextureValueMaximaLocation = _splatHeightfieldProgram.uniformLocation("textureValueMaxima");
        _splatHeightfieldProgram.release();
        
        _lightHeightfieldProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/metavoxel_heightfield_light.vert");
        _lightHeightfieldProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/metavoxel_heightfield_light.frag");
        _lightHeightfieldProgram.link();
        
        _lightHeightfieldProgram.bind();
        _lightHeightfieldProgram.setUniformValue("heightMap", 0);
        _lightHeightScaleLocation = _lightHeightfieldProgram.uniformLocation("heightScale");
        _lightHeightfieldProgram.release();
        
        _shadowLightHeightfieldProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/metavoxel_heightfield_light.vert");
        _shadowLightHeightfieldProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/metavoxel_heightfield_light_shadow_map.frag");
        _shadowLightHeightfieldProgram.link();
        
        _shadowLightHeightfieldProgram.bind();
        _shadowLightHeightfieldProgram.setUniformValue("heightMap", 0);
        _shadowLightHeightfieldProgram.setUniformValue("shadowMap", 2);
        _shadowLightHeightScaleLocation = _shadowLightHeightfieldProgram.uniformLocation("heightScale");
        _shadowLightHeightfieldProgram.release();
        
        _cascadedShadowLightHeightfieldProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/metavoxel_heightfield_light.vert");
        _cascadedShadowLightHeightfieldProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/metavoxel_heightfield_light_cascaded_shadow_map.frag");
        _cascadedShadowLightHeightfieldProgram.link();
        
        _cascadedShadowLightHeightfieldProgram.bind();
        _cascadedShadowLightHeightfieldProgram.setUniformValue("heightMap", 0);
        _cascadedShadowLightHeightfieldProgram.setUniformValue("shadowMap", 2);
        _cascadedShadowLightHeightScaleLocation = _cascadedShadowLightHeightfieldProgram.uniformLocation("heightScale");
        _shadowLightDistancesLocation = _cascadedShadowLightHeightfieldProgram.uniformLocation("shadowDistances");
        _cascadedShadowLightHeightfieldProgram.release();
        
        _heightfieldCursorProgram.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() +
            "shaders/metavoxel_heightfield_cursor.vert");
        _heightfieldCursorProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() +
            "shaders/metavoxel_heightfield_cursor.frag");
        _heightfieldCursorProgram.link();
        
        _heightfieldCursorProgram.bind();
        _heightfieldCursorProgram.setUniformValue("heightMap", 0);
        _heightfieldCursorProgram.release();
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
        info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline(BufferDataPointer(buffer)));
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
    info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline(BufferDataPointer(buffer)));
    return true;
}

class HeightfieldFetchVisitor : public MetavoxelVisitor {
public:
    
    HeightfieldFetchVisitor(const MetavoxelLOD& lod, const QVector<Box>& intersections);
    
    void init(HeightfieldBuffer* buffer) { _buffer = buffer; }
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    const QVector<Box>& _intersections;
    HeightfieldBuffer* _buffer;
};

HeightfieldFetchVisitor::HeightfieldFetchVisitor(const MetavoxelLOD& lod, const QVector<Box>& intersections) :
    MetavoxelVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getHeightfieldAttribute() <<
        AttributeRegistry::getInstance()->getHeightfieldColorAttribute(), QVector<AttributePointer>(), lod),
    _intersections(intersections) {
}

int HeightfieldFetchVisitor::visit(MetavoxelInfo& info) {
    Box bounds = info.getBounds();
    const Box& heightBounds = _buffer->getHeightBounds();
    if (!bounds.intersects(heightBounds)) {
        return STOP_RECURSION;
    }
    if (!info.isLeaf && info.size > _buffer->getScale()) {
        return DEFAULT_ORDER;
    }
    HeightfieldHeightDataPointer height = info.inputValues.at(0).getInlineValue<HeightfieldHeightDataPointer>();
    if (!height) {
        return STOP_RECURSION;
    }
    foreach (const Box& intersection, _intersections) {
        Box overlap = intersection.getIntersection(bounds);
        if (overlap.isEmpty()) {
            continue;
        }
        float heightIncrement = _buffer->getHeightIncrement();
        int destX = (overlap.minimum.x - heightBounds.minimum.x) / heightIncrement;
        int destY = (overlap.minimum.z - heightBounds.minimum.z) / heightIncrement;
        int destWidth = glm::ceil((overlap.maximum.x - overlap.minimum.x) / heightIncrement);
        int destHeight = glm::ceil((overlap.maximum.z - overlap.minimum.z) / heightIncrement);
        int heightSize = _buffer->getHeightSize();
        char* dest = _buffer->getHeight().data() + destY * heightSize + destX;
        
        const QByteArray& srcHeight = height->getContents();
        int srcSize = glm::sqrt(srcHeight.size());
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
        
        int colorSize = _buffer->getColorSize();
        if (colorSize == 0) {
            continue;
        }
        HeightfieldColorDataPointer color = info.inputValues.at(1).getInlineValue<HeightfieldColorDataPointer>();
        if (!color) {
            continue;
        }
        const Box& colorBounds = _buffer->getColorBounds();
        overlap = colorBounds.getIntersection(overlap);
        float colorIncrement = _buffer->getColorIncrement();
        destX = (overlap.minimum.x - colorBounds.minimum.x) / colorIncrement;
        destY = (overlap.minimum.z - colorBounds.minimum.z) / colorIncrement;
        destWidth = glm::ceil((overlap.maximum.x - overlap.minimum.x) / colorIncrement);
        destHeight = glm::ceil((overlap.maximum.z - overlap.minimum.z) / colorIncrement);
        dest = _buffer->getColor().data() + (destY * colorSize + destX) * DataBlock::COLOR_BYTES;
        int destStride = colorSize * DataBlock::COLOR_BYTES;
        int destBytes = destWidth * DataBlock::COLOR_BYTES;
        
        const QByteArray& srcColor = color->getContents();
        srcSize = glm::sqrt(srcColor.size() / DataBlock::COLOR_BYTES);
        int srcStride = srcSize * DataBlock::COLOR_BYTES;
        srcIncrement = info.size / srcSize;
        
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
    return STOP_RECURSION;
}

class HeightfieldRegionVisitor : public MetavoxelVisitor {
public:
    
    QVector<Box> regions;
    Box regionBounds;

    HeightfieldRegionVisitor(const MetavoxelLOD& lod);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    void addRegion(const Box& unextended, const Box& extended);
    
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
    _fetchVisitor(lod, _intersections) {
}

int HeightfieldRegionVisitor::visit(MetavoxelInfo& info) {
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    HeightfieldBuffer* buffer = NULL;
    HeightfieldHeightDataPointer height = info.inputValues.at(0).getInlineValue<HeightfieldHeightDataPointer>();
    if (height) {
        const QByteArray& heightContents = height->getContents();
        int size = glm::sqrt(heightContents.size());
        int extendedSize = size + HeightfieldBuffer::HEIGHT_EXTENSION;
        int heightContentsSize = extendedSize * extendedSize;
        
        HeightfieldColorDataPointer color = info.inputValues.at(1).getInlineValue<HeightfieldColorDataPointer>();
        int colorContentsSize = 0;
        if (color) {
            const QByteArray& colorContents = color->getContents();
            int colorSize = glm::sqrt(colorContents.size() / DataBlock::COLOR_BYTES);
            int extendedColorSize = colorSize + HeightfieldBuffer::SHARED_EDGE;
            colorContentsSize = extendedColorSize * extendedColorSize * DataBlock::COLOR_BYTES;
        }
        
        HeightfieldMaterialDataPointer material = info.inputValues.at(2).getInlineValue<HeightfieldMaterialDataPointer>();
        QByteArray materialContents;
        QVector<SharedObjectPointer> materials;
        if (material) {
            materialContents = material->getContents();
            materials = material->getMaterials();
        }
        
        const HeightfieldBuffer* existingBuffer = static_cast<const HeightfieldBuffer*>(
            info.inputValues.at(3).getInlineValue<BufferDataPointer>().data());
        Box bounds = info.getBounds();
        if (existingBuffer && existingBuffer->getHeight().size() == heightContentsSize &&
                existingBuffer->getColor().size() == colorContentsSize) {
            // we already have a buffer of the correct resolution    
            addRegion(bounds, existingBuffer->getHeightBounds());
            buffer = new HeightfieldBuffer(info.minimum, info.size, existingBuffer->getHeight(),
                existingBuffer->getColor(), materialContents, materials);

        } else {
            // we must create a new buffer and update its borders
            buffer = new HeightfieldBuffer(info.minimum, info.size, QByteArray(heightContentsSize, 0),
                QByteArray(colorContentsSize, 0), materialContents, materials);
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
    }
    info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline(BufferDataPointer(buffer)));
    return STOP_RECURSION;
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
    info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline(BufferDataPointer(newBuffer)));
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
};

int VoxelAugmentVisitor::visit(MetavoxelInfo& info) {
    if (!info.isLeaf) {
        return DEFAULT_ORDER;
    }
    VoxelBuffer* buffer = NULL;
    VoxelColorDataPointer color = info.inputValues.at(0).getInlineValue<VoxelColorDataPointer>();
    VoxelMaterialDataPointer material = info.inputValues.at(1).getInlineValue<VoxelMaterialDataPointer>();
    VoxelHermiteDataPointer hermite = info.inputValues.at(2).getInlineValue<VoxelHermiteDataPointer>();
    if (color && material && hermite) {
        QVector<VoxelPoint> vertices;
        QVector<int> indices;
        
        const QVector<QRgb>& colorContents = color->getContents();
        const QByteArray& materialContents = material->getContents();
        const QVector<QRgb>& hermiteContents = hermite->getContents();
        int size = color->getSize();
        int area = size * size;
        int offset3 = size + 1;
        int offset5 = area + 1;
        int offset6 = area + size;
        int offset7 = area + size + 1;
        
        const QRgb* colorZ = colorContents.constData();
        const QRgb* hermiteData = hermiteContents.constData();
        int hermiteStride = hermite->getSize() * VoxelHermiteData::EDGE_COUNT;
        int hermiteArea = hermiteStride * hermite->getSize();
        
        const char* materialData = materialContents.constData();
        
        int expanded = size + 1;
        QVector<int> lineIndices(expanded, -1);
        QVector<int> lastLineIndices(expanded, -1);
        QVector<int> planeIndices(expanded * expanded, -1);
        QVector<int> lastPlaneIndices(expanded * expanded, -1);
        
        const int EDGES_PER_CUBE = 12;
        EdgeCrossing crossings[EDGES_PER_CUBE];
        
        float highest = size - 1.0f;
        float scale = info.size / highest;
        const int ALPHA_OFFSET = 24;
        for (int z = 0; z < expanded; z++) {
            const QRgb* colorY = colorZ;
            for (int y = 0; y < expanded; y++) {
                int lastIndex;
                const QRgb* colorX = colorY;
                for (int x = 0; x < expanded; x++) {
                    int alpha0 = colorX[0] >> ALPHA_OFFSET;
                    int alpha1 = alpha0, alpha2 = alpha0, alpha4 = alpha0;
                    int alphaTotal = alpha0;
                    int possibleTotal = EIGHT_BIT_MAXIMUM;
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
                    int clampedX = qMax(x - 1, 0), clampedY = qMax(y - 1, 0), clampedZ = qMax(z - 1, 0);
                    const QRgb* hermiteBase = hermiteData + clampedZ * hermiteArea + clampedY * hermiteStride +
                        clampedX * VoxelHermiteData::EDGE_COUNT;
                    const char* materialBase = materialData + clampedZ * area + clampedY * size + clampedX;
                    int crossingCount = 0;
                    if (middleX) {
                        if (alpha0 != alpha1) {
                            QRgb hermite = hermiteBase[0];
                            EdgeCrossing& crossing = crossings[crossingCount++];
                            crossing.normal = unpackNormal(hermite);
                            if (alpha0 == 0) {
                                crossing.color = colorX[1];
                                crossing.material = materialBase[1];
                            } else {
                                crossing.color = colorX[0];
                                crossing.material = materialBase[0];
                            }
                            crossing.point = glm::vec3(qAlpha(hermite), 0.0f, 0.0f);
                        }
                        if (middleY) {
                            if (alpha1 != alpha3) {
                                QRgb hermite = hermiteBase[VoxelHermiteData::EDGE_COUNT + 1];
                                EdgeCrossing& crossing = crossings[crossingCount++];
                                crossing.normal = unpackNormal(hermite);
                                if (alpha1 == 0) {
                                    crossing.color = colorX[offset3];
                                    crossing.material = materialBase[offset3];
                                } else {
                                    crossing.color = colorX[1];
                                    crossing.material = materialBase[1];
                                }
                                crossing.point = glm::vec3(EIGHT_BIT_MAXIMUM, qAlpha(hermite), 0.0f);
                            }
                            if (alpha2 != alpha3) {
                                QRgb hermite = hermiteBase[hermiteStride];
                                EdgeCrossing& crossing = crossings[crossingCount++];
                                crossing.normal = unpackNormal(hermite);
                                if (alpha2 == 0) {
                                    crossing.color = colorX[offset3];
                                    crossing.material = materialBase[offset3];
                                } else {
                                    crossing.color = colorX[size];
                                    crossing.material = materialBase[size];
                                }
                                crossing.point = glm::vec3(qAlpha(hermite), EIGHT_BIT_MAXIMUM, 0.0f);
                            }
                            if (middleZ) {
                                if (alpha3 != alpha7) {
                                    QRgb hermite = hermiteBase[hermiteStride + VoxelHermiteData::EDGE_COUNT + 2];
                                    EdgeCrossing& crossing = crossings[crossingCount++];
                                    crossing.normal = unpackNormal(hermite);
                                    if (alpha3 == 0) {
                                        crossing.color = colorX[offset7];
                                        crossing.material = materialBase[offset7];
                                    } else {
                                        crossing.color = colorX[offset3];
                                        crossing.material = materialBase[offset3];
                                    }
                                    crossing.point = glm::vec3(EIGHT_BIT_MAXIMUM, EIGHT_BIT_MAXIMUM, qAlpha(hermite));
                                }
                                if (alpha5 != alpha7) {
                                    QRgb hermite = hermiteBase[hermiteArea + VoxelHermiteData::EDGE_COUNT + 1];
                                    EdgeCrossing& crossing = crossings[crossingCount++];
                                    crossing.normal = unpackNormal(hermite);
                                    if (alpha5 == 0) {
                                        crossing.color = colorX[offset7];
                                        crossing.material = materialBase[offset7];
                                    } else {
                                        crossing.color = colorX[offset5];
                                        crossing.material = materialBase[offset5];
                                    }
                                    crossing.point = glm::vec3(EIGHT_BIT_MAXIMUM, qAlpha(hermite), EIGHT_BIT_MAXIMUM);
                                }
                                if (alpha6 != alpha7) {
                                    QRgb hermite = hermiteBase[hermiteArea + hermiteStride];
                                    EdgeCrossing& crossing = crossings[crossingCount++];
                                    crossing.normal = unpackNormal(hermite);
                                    if (alpha6 == 0) {
                                        crossing.color = colorX[offset7];
                                        crossing.material = materialBase[offset7];
                                    } else {
                                        crossing.color = colorX[offset6];
                                        crossing.material = materialBase[offset6];
                                    }
                                    crossing.point = glm::vec3(qAlpha(hermite), EIGHT_BIT_MAXIMUM, EIGHT_BIT_MAXIMUM);
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
                                    crossing.material = materialBase[offset5];
                                } else {
                                    crossing.color = colorX[1];
                                    crossing.material = materialBase[1];
                                }
                                crossing.point = glm::vec3(EIGHT_BIT_MAXIMUM, 0.0f, qAlpha(hermite));
                            }
                            if (alpha4 != alpha5) {
                                QRgb hermite = hermiteBase[hermiteArea];
                                EdgeCrossing& crossing = crossings[crossingCount++];
                                crossing.normal = unpackNormal(hermite);
                                if (alpha4 == 0) {
                                    crossing.color = colorX[offset5];
                                    crossing.material = materialBase[offset5];
                                } else {
                                    crossing.color = colorX[area];
                                    crossing.material = materialBase[area];
                                }
                                crossing.point = glm::vec3(qAlpha(hermite), 0.0f, EIGHT_BIT_MAXIMUM);
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
                                crossing.material = materialBase[size];
                            } else {
                                crossing.color = colorX[0];
                                crossing.material = materialBase[0];
                            }
                            crossing.point = glm::vec3(0.0f, qAlpha(hermite), 0.0f);
                        }
                        if (middleZ) {
                            if (alpha2 != alpha6) {
                                QRgb hermite = hermiteBase[hermiteStride + 2];
                                EdgeCrossing& crossing = crossings[crossingCount++];
                                crossing.normal = unpackNormal(hermite);
                                if (alpha2 == 0) {
                                    crossing.color = colorX[offset6];
                                    crossing.material = materialBase[offset6];
                                } else {
                                    crossing.color = colorX[size];
                                    crossing.material = materialBase[size];
                                }
                                crossing.point = glm::vec3(0.0f, EIGHT_BIT_MAXIMUM, qAlpha(hermite));
                            }
                            if (alpha4 != alpha6) {
                                QRgb hermite = hermiteBase[hermiteArea + 1];
                                EdgeCrossing& crossing = crossings[crossingCount++];
                                crossing.normal = unpackNormal(hermite);
                                if (alpha4 == 0) {
                                    crossing.color = colorX[offset6];
                                    crossing.material = materialBase[offset6];
                                } else {
                                    crossing.color = colorX[area];
                                    crossing.material = materialBase[area];
                                }
                                crossing.point = glm::vec3(0.0f, qAlpha(hermite), EIGHT_BIT_MAXIMUM);
                            }
                        }
                    }
                    if (middleZ && alpha0 != alpha4) {
                        QRgb hermite = hermiteBase[2];
                        EdgeCrossing& crossing = crossings[crossingCount++];
                        crossing.normal = unpackNormal(hermite);
                        if (alpha0 == 0) {
                            crossing.color = colorX[area];
                            crossing.material = materialBase[area];
                        } else {
                            crossing.color = colorX[0];
                            crossing.material = materialBase[0];
                        }
                        crossing.point = glm::vec3(0.0f, 0.0f, qAlpha(hermite));
                    }
                    glm::vec3 center;
                    glm::vec3 normal;
                    int red = 0, green = 0, blue = 0;
                    for (int i = 0; i < crossingCount; i++) {
                        const EdgeCrossing& crossing = crossings[i];
                        center += crossing.point;
                        normal += crossing.normal;
                        red += qRed(crossing.color);
                        green += qGreen(crossing.color);
                        blue += qBlue(crossing.color);
                    }
                    normal = glm::normalize(normal);
                    center /= crossingCount;
                    VoxelPoint point = { info.minimum + (glm::vec3(clampedX, clampedY, clampedZ) +
                        center * EIGHT_BIT_MAXIMUM_RECIPROCAL) * scale,
                        { (quint8)(red / crossingCount), (quint8)(green / crossingCount), (quint8)(blue / crossingCount) },
                        { (char)(normal.x * 127.0f), (char)(normal.y * 127.0f), (char)(normal.z * 127.0f) } };
                    int index = vertices.size();
                    vertices.append(point);
                    
                    if (x != 0 && y != 0 && z != 0) {
                        if (alpha0 != alpha1) {
                            indices.append(index);
                            int index1 = lastLineIndices.at(x);
                            int index2 = lastPlaneIndices.at((y - 1) * expanded + x);
                            int index3 = lastPlaneIndices.at(y * expanded + x);
                            if (alpha0 == 0) {
                                indices.append(index3);
                                indices.append(index2);
                                indices.append(index1);
                            } else {
                                indices.append(index1);
                                indices.append(index2);
                                indices.append(index3);
                            }
                        }
                        
                        if (alpha0 != alpha2) {
                            indices.append(index);
                            int index1 = lastIndex;
                            int index2 = lastPlaneIndices.at(y * expanded + x - 1);
                            int index3 = lastPlaneIndices.at(y * expanded + x);
                            if (alpha0 == 0) {
                                indices.append(index1);
                                indices.append(index2);
                                indices.append(index3);
                            } else {
                                indices.append(index3);
                                indices.append(index2);
                                indices.append(index1);
                            }
                        }
                        
                        if (alpha0 != alpha4) {
                            indices.append(index);
                            int index1 = lastIndex;
                            int index2 = lastLineIndices.at(x - 1);
                            int index3 = lastLineIndices.at(x);
                            if (alpha0 == 0) {
                                indices.append(index3);
                                indices.append(index2);
                                indices.append(index1);
                            } else {
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
        
        buffer = new VoxelBuffer(vertices, indices, material->getMaterials());
    }
    info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline(BufferDataPointer(buffer)));
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
        lod, encodeOrder(Application::getInstance()->getViewFrustum()->getDirection())),
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
    spanner->getRenderer()->render(1.0f, SpannerRenderer::DEFAULT_MODE, clipMinimum, clipSize);
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
    _order(encodeOrder(Application::getInstance()->getViewFrustum()->getDirection())),
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
    float worldDiagonal = glm::distance(Application::getInstance()->getViewFrustum()->getNearBottomLeft(),
        Application::getInstance()->getViewFrustum()->getNearTopRight());

    _pointProgram.bind();
    _pointProgram.setUniformValue(_pointScaleLocation, viewportDiagonal *
        Application::getInstance()->getViewFrustum()->getNearClip() / worldDiagonal);
        
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
    
    glEnable(GL_CULL_FACE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_EQUAL, 0.0f);
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    ProgramObject* program = &_heightfieldProgram;
    if (Menu::getInstance()->getShadowsEnabled()) {
        if (Menu::getInstance()->isOptionChecked(MenuOption::CascadedShadows)) {
            _cascadedShadowLightHeightfieldProgram.bind();
            _cascadedShadowLightHeightfieldProgram.setUniform(_shadowLightDistancesLocation,
                Application::getInstance()->getShadowDistances());
            program = &_cascadedShadowMapHeightfieldProgram;
            program->bind();
            program->setUniform(_shadowDistancesLocation, Application::getInstance()->getShadowDistances());
            
        } else {
            program = &_shadowMapHeightfieldProgram;
        }
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, Application::getInstance()->getTextureCache()->getShadowDepthTextureID());
        glActiveTexture(GL_TEXTURE0);
    }
    
    program->bind();
    
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    BufferRenderVisitor heightfieldRenderVisitor(Application::getInstance()->getMetavoxels()->getHeightfieldBufferAttribute());
    data.guide(heightfieldRenderVisitor);
    
    program->release();
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
        
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    
    glEnable(GL_CULL_FACE);
    
    BufferRenderVisitor voxelRenderVisitor(Application::getInstance()->getMetavoxels()->getVoxelBufferAttribute());
    data.guide(voxelRenderVisitor);
    
    glDisable(GL_CULL_FACE);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
}

ProgramObject DefaultMetavoxelRendererImplementation::_pointProgram;
int DefaultMetavoxelRendererImplementation::_pointScaleLocation;
ProgramObject DefaultMetavoxelRendererImplementation::_heightfieldProgram;
int DefaultMetavoxelRendererImplementation::_heightScaleLocation;
int DefaultMetavoxelRendererImplementation::_colorScaleLocation;
ProgramObject DefaultMetavoxelRendererImplementation::_shadowMapHeightfieldProgram;
int DefaultMetavoxelRendererImplementation::_shadowMapHeightScaleLocation;
int DefaultMetavoxelRendererImplementation::_shadowMapColorScaleLocation;
ProgramObject DefaultMetavoxelRendererImplementation::_cascadedShadowMapHeightfieldProgram;
int DefaultMetavoxelRendererImplementation::_cascadedShadowMapHeightScaleLocation;
int DefaultMetavoxelRendererImplementation::_cascadedShadowMapColorScaleLocation;
int DefaultMetavoxelRendererImplementation::_shadowDistancesLocation;
ProgramObject DefaultMetavoxelRendererImplementation::_baseHeightfieldProgram;
int DefaultMetavoxelRendererImplementation::_baseHeightScaleLocation;
int DefaultMetavoxelRendererImplementation::_baseColorScaleLocation;
ProgramObject DefaultMetavoxelRendererImplementation::_splatHeightfieldProgram;
int DefaultMetavoxelRendererImplementation::_splatHeightScaleLocation;
int DefaultMetavoxelRendererImplementation::_splatTextureScaleLocation;
int DefaultMetavoxelRendererImplementation::_splatTextureOffsetLocation;
int DefaultMetavoxelRendererImplementation::_splatTextureScalesSLocation;
int DefaultMetavoxelRendererImplementation::_splatTextureScalesTLocation;
int DefaultMetavoxelRendererImplementation::_splatTextureValueMinimaLocation;
int DefaultMetavoxelRendererImplementation::_splatTextureValueMaximaLocation;
ProgramObject DefaultMetavoxelRendererImplementation::_lightHeightfieldProgram;
int DefaultMetavoxelRendererImplementation::_lightHeightScaleLocation; 
ProgramObject DefaultMetavoxelRendererImplementation::_shadowLightHeightfieldProgram;
int DefaultMetavoxelRendererImplementation::_shadowLightHeightScaleLocation;
ProgramObject DefaultMetavoxelRendererImplementation::_cascadedShadowLightHeightfieldProgram;
int DefaultMetavoxelRendererImplementation::_cascadedShadowLightHeightScaleLocation;
int DefaultMetavoxelRendererImplementation::_shadowLightDistancesLocation; 
ProgramObject DefaultMetavoxelRendererImplementation::_heightfieldCursorProgram;

static void enableClipPlane(GLenum plane, float x, float y, float z, float w) {
    GLdouble coefficients[] = { x, y, z, w };
    glClipPlane(plane, coefficients);
    glEnable(plane);
}

void ClippedRenderer::render(float alpha, Mode mode, const glm::vec3& clipMinimum, float clipSize) {
    if (clipSize == 0.0f) {
        renderUnclipped(alpha, mode);
        return;
    }
    enableClipPlane(GL_CLIP_PLANE0, -1.0f, 0.0f, 0.0f, clipMinimum.x + clipSize);
    enableClipPlane(GL_CLIP_PLANE1, 1.0f, 0.0f, 0.0f, -clipMinimum.x);
    enableClipPlane(GL_CLIP_PLANE2, 0.0f, -1.0f, 0.0f, clipMinimum.y + clipSize);
    enableClipPlane(GL_CLIP_PLANE3, 0.0f, 1.0f, 0.0f, -clipMinimum.y);
    enableClipPlane(GL_CLIP_PLANE4, 0.0f, 0.0f, -1.0f, clipMinimum.z + clipSize);
    enableClipPlane(GL_CLIP_PLANE5, 0.0f, 0.0f, 1.0f, -clipMinimum.z);
    
    renderUnclipped(alpha, mode);
    
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDisable(GL_CLIP_PLANE2);
    glDisable(GL_CLIP_PLANE3);
    glDisable(GL_CLIP_PLANE4);
    glDisable(GL_CLIP_PLANE5);
}

SphereRenderer::SphereRenderer() {
}

void SphereRenderer::render(float alpha, Mode mode, const glm::vec3& clipMinimum, float clipSize) {
    if (clipSize == 0.0f) {
        renderUnclipped(alpha, mode);
        return;
    }
    // slight performance optimization: don't render if clip bounds are entirely within sphere
    Sphere* sphere = static_cast<Sphere*>(_spanner);
    Box clipBox(clipMinimum, clipMinimum + glm::vec3(clipSize, clipSize, clipSize));
    for (int i = 0; i < Box::VERTEX_COUNT; i++) {
        const float CLIP_PROPORTION = 0.95f;
        if (glm::distance(sphere->getTranslation(), clipBox.getVertex(i)) >= sphere->getScale() * CLIP_PROPORTION) {
            ClippedRenderer::render(alpha, mode, clipMinimum, clipSize);
            return;
        }
    }
}

void SphereRenderer::renderUnclipped(float alpha, Mode mode) {
    Sphere* sphere = static_cast<Sphere*>(_spanner);
    const QColor& color = sphere->getColor();
    glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF() * alpha);
    
    glPushMatrix();
    const glm::vec3& translation = sphere->getTranslation();
    glTranslatef(translation.x, translation.y, translation.z);
    glm::quat rotation = sphere->getRotation();
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(glm::angle(rotation), axis.x, axis.y, axis.z);
    
    glutSolidSphere(sphere->getScale(), 10, 10);
    
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

void StaticModelRenderer::renderUnclipped(float alpha, Mode mode) {
    switch (mode) {
        case DIFFUSE_MODE:
            _model->render(alpha, Model::DIFFUSE_RENDER_MODE);
            break;
            
        case NORMAL_MODE:
            _model->render(alpha, Model::NORMAL_RENDER_MODE);
            break;
            
        default:
            _model->render(alpha);
            break;
    }
    _model->render(alpha);
}

bool StaticModelRenderer::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& clipMinimum, float clipSize, float& distance) const {
    return _model->findRayIntersection(origin, direction, distance);
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
