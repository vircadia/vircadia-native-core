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
REGISTER_META_OBJECT(HeightfieldRenderer)

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

class SpannerCursorRenderVisitor : public SpannerVisitor {
public:
    
    SpannerCursorRenderVisitor(const Box& bounds);
    
    virtual bool visit(Spanner* spanner);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    Box _bounds;
};

SpannerCursorRenderVisitor::SpannerCursorRenderVisitor(const Box& bounds) :
    SpannerVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getSpannersAttribute()),
    _bounds(bounds) {
}

bool SpannerCursorRenderVisitor::visit(Spanner* spanner) {
    if (spanner->isHeightfield()) {
        spanner->getRenderer()->render(true);
    }
    return true;
}

int SpannerCursorRenderVisitor::visit(MetavoxelInfo& info) {
    return info.getBounds().intersects(_bounds) ? SpannerVisitor::visit(info) : STOP_RECURSION;
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
    SpannerCursorRenderVisitor visitor(Box(position - extents, position + extents));
    guide(visitor);
    
    DefaultMetavoxelRendererImplementation::getHeightfieldCursorProgram().release();
    
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
}

class BufferCursorRenderVisitor : public MetavoxelVisitor {
public:
    
    BufferCursorRenderVisitor(const AttributePointer& attribute, const Box& bounds);

    virtual int visit(MetavoxelInfo& info);

private:
    
    Box _bounds;
};

BufferCursorRenderVisitor::BufferCursorRenderVisitor(const AttributePointer& attribute, const Box& bounds) :
    MetavoxelVisitor(QVector<AttributePointer>() << attribute),
    _bounds(bounds) {
}

int BufferCursorRenderVisitor::visit(MetavoxelInfo& info) {
    if (!info.getBounds().intersects(_bounds)) {
        return STOP_RECURSION;
    }
    BufferData* buffer = info.inputValues.at(0).getInlineValue<BufferDataPointer>().data();
    if (buffer) {
        buffer->render(true);
    }
    return info.isLeaf ? STOP_RECURSION : DEFAULT_ORDER;
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
    BufferCursorRenderVisitor voxelVisitor(Application::getInstance()->getMetavoxels()->getVoxelBufferAttribute(), bounds);
    guideToAugmented(voxelVisitor);
    
    DefaultMetavoxelRendererImplementation::getVoxelCursorProgram().release();
    
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    DefaultMetavoxelRendererImplementation::getHeightfieldCursorProgram().bind();
    
    SpannerCursorRenderVisitor spannerVisitor(bounds);
    guide(spannerVisitor);
    
    DefaultMetavoxelRendererImplementation::getHeightfieldCursorProgram().release();
    
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
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

const int SPLAT_COUNT = 4;
const GLint SPLAT_TEXTURE_UNITS[] = { 3, 4, 5, 6 };

static const int EIGHT_BIT_MAXIMUM = 255;
static const float EIGHT_BIT_MAXIMUM_RECIPROCAL = 1.0f / EIGHT_BIT_MAXIMUM;

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
    if (!_baseHeightfieldProgram.isLinked()) {
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
    const AttributePointer& voxelBufferAttribute =
        Application::getInstance()->getMetavoxels()->getVoxelBufferAttribute();
    MetavoxelNode* root = expandedPrevious.getRoot(voxelBufferAttribute);
    if (root) {
        data.setRoot(voxelBufferAttribute, root);
        root->incrementReferenceCount();
    }
    VoxelAugmentVisitor voxelAugmentVisitor(lod);
    data.guideToDifferent(expandedPrevious, voxelAugmentVisitor);
}

class SpannerSimulateVisitor : public SpannerVisitor {
public:
    
    SpannerSimulateVisitor(float deltaTime, const MetavoxelLOD& lod);
    
    virtual bool visit(Spanner* spanner);

private:
    
    float _deltaTime;
};

SpannerSimulateVisitor::SpannerSimulateVisitor(float deltaTime, const MetavoxelLOD& lod) :
    SpannerVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getSpannersAttribute(),
        QVector<AttributePointer>(), QVector<AttributePointer>(), lod),
    _deltaTime(deltaTime) {
}

bool SpannerSimulateVisitor::visit(Spanner* spanner) {
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
    virtual bool visit(Spanner* spanner);

private:
    
    int _containmentDepth;
};

SpannerRenderVisitor::SpannerRenderVisitor(const MetavoxelLOD& lod) :
    SpannerVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getSpannersAttribute(),
        QVector<AttributePointer>(), QVector<AttributePointer>(), lod,
        encodeOrder(Application::getInstance()->getDisplayViewFrustum()->getDirection())),
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

bool SpannerRenderVisitor::visit(Spanner* spanner) {
    spanner->getRenderer()->render();
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
    if (Menu::getInstance()->isOptionChecked(MenuOption::RenderSpanners)) {
        SpannerRenderVisitor spannerRenderVisitor(lod);
        data.guide(spannerRenderVisitor);
    }
    
    Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(true, true);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_EQUAL, 0.0f);
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
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

SphereRenderer::SphereRenderer() {
}


void SphereRenderer::render(bool cursor) {
    Sphere* sphere = static_cast<Sphere*>(_spanner);
    const QColor& color = sphere->getColor();
    glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    
    glPushMatrix();
    const glm::vec3& translation = sphere->getTranslation();
    glTranslatef(translation.x, translation.y, translation.z);
    glm::quat rotation = sphere->getRotation();
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
    
    Application::getInstance()->getDeferredLightingEffect()->renderSolidSphere(sphere->getScale(), 32, 32);
    
    glPopMatrix();
}

CuboidRenderer::CuboidRenderer() {
}

void CuboidRenderer::render(bool cursor) {
    Cuboid* cuboid = static_cast<Cuboid*>(_spanner);
    const QColor& color = cuboid->getColor();
    glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    
    glPushMatrix();
    const glm::vec3& translation = cuboid->getTranslation();
    glTranslatef(translation.x, translation.y, translation.z);
    glm::quat rotation = cuboid->getRotation();
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
    glScalef(1.0f, cuboid->getAspectY(), cuboid->getAspectZ());
    
    Application::getInstance()->getDeferredLightingEffect()->renderSolidCube(cuboid->getScale() * 2.0f);
    
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

void StaticModelRenderer::render(bool cursor) {
    _model->render();
}

bool StaticModelRenderer::findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance) const {
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
    _model->setScale(glm::vec3(scale, scale, scale));
}

void StaticModelRenderer::applyURL(const QUrl& url) {
    _model->setURL(url);
}

HeightfieldRenderer::HeightfieldRenderer() {
    glGenTextures(1, &_heightTextureID);
    glBindTexture(GL_TEXTURE_2D, _heightTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
    glGenTextures(1, &_colorTextureID);
    glBindTexture(GL_TEXTURE_2D, _colorTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glGenTextures(1, &_materialTextureID);
    glBindTexture(GL_TEXTURE_2D, _materialTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

HeightfieldRenderer::~HeightfieldRenderer() {
    glDeleteTextures(1, &_heightTextureID);
    glDeleteTextures(1, &_colorTextureID);
    glDeleteTextures(1, &_materialTextureID);
}

void HeightfieldRenderer::init(Spanner* spanner) {
    SpannerRenderer::init(spanner);
    
    Heightfield* heightfield = static_cast<Heightfield*>(spanner);
    applyHeight(heightfield->getHeight());
    applyColor(heightfield->getColor());
    applyMaterial(heightfield->getMaterial());
    
    connect(heightfield, &Heightfield::heightChanged, this, &HeightfieldRenderer::applyHeight);
    connect(heightfield, &Heightfield::colorChanged, this, &HeightfieldRenderer::applyColor);
    connect(heightfield, &Heightfield::materialChanged, this, &HeightfieldRenderer::applyMaterial);
}

class HeightfieldPoint {
public:
    glm::vec3 vertex;
    glm::vec2 textureCoord;
};

void HeightfieldRenderer::render(bool cursor) {
    // create the buffer objects lazily
    Heightfield* heightfield = static_cast<Heightfield*>(_spanner);
    if (!heightfield->getHeight()) {
        return;
    }
    int width = heightfield->getHeight()->getWidth();
    int height = heightfield->getHeight()->getContents().size() / width;
    
    int innerWidth = width - 2 * HeightfieldHeight::HEIGHT_BORDER;
    int innerHeight = height - 2 * HeightfieldHeight::HEIGHT_BORDER;
    int vertexCount = width * height;
    int rows = height - 1;
    int columns = width - 1;
    int indexCount = rows * columns * 3 * 2;
    BufferPair& bufferPair = _bufferPairs[IntPair(width, height)];
    if (!bufferPair.first.isCreated()) {
        QVector<HeightfieldPoint> vertices(vertexCount);
        HeightfieldPoint* point = vertices.data();
        
        float xStep = 1.0f / (innerWidth - 1);
        float zStep = 1.0f / (innerHeight - 1);
        float z = -zStep;
        float sStep = 1.0f / width;
        float tStep = 1.0f / height;
        float t = tStep / 2.0f;
        for (int i = 0; i < height; i++, z += zStep, t += tStep) {
            float x = -xStep;
            float s = sStep / 2.0f;
            const float SKIRT_LENGTH = 0.25f;
            float baseY = (i == 0 || i == height - 1) ? -SKIRT_LENGTH : 0.0f;
            for (int j = 0; j < width; j++, point++, x += xStep, s += sStep) {
                point->vertex = glm::vec3(x, (j == 0 || j == width - 1) ? -SKIRT_LENGTH : baseY, z);
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
            int lineIndex = i * width;
            int nextLineIndex = (i + 1) * width;
            for (int j = 0; j < columns; j++) {
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
    glPushMatrix();
    glTranslatef(heightfield->getTranslation().x, heightfield->getTranslation().y, heightfield->getTranslation().z);
    glm::vec3 axis = glm::axis(heightfield->getRotation());
    glRotatef(glm::degrees(glm::angle(heightfield->getRotation())), axis.x, axis.y, axis.z);
    float xScale = heightfield->getScale(), zScale = xScale * heightfield->getAspectZ();
    glScalef(xScale, xScale * heightfield->getAspectY(), zScale);
    
    HeightfieldPoint* point = 0;
    glVertexPointer(3, GL_FLOAT, sizeof(HeightfieldPoint), &point->vertex);
    glTexCoordPointer(2, GL_FLOAT, sizeof(HeightfieldPoint), &point->textureCoord);
    
    glBindTexture(GL_TEXTURE_2D, _heightTextureID);
    
    if (cursor) {
        glDrawRangeElements(GL_TRIANGLES, 0, vertexCount - 1, indexCount, GL_UNSIGNED_INT, 0);
        
    } else {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
        Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(true, true);
    
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_EQUAL, 0.0f);
        
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        
        DefaultMetavoxelRendererImplementation::getBaseHeightfieldProgram().bind();
        DefaultMetavoxelRendererImplementation::getBaseHeightfieldProgram().setUniformValue(
            DefaultMetavoxelRendererImplementation::getBaseHeightScaleLocation(), 1.0f / width, 1.0f / height,
            (innerWidth - 1) / -2.0f, (innerHeight - 1) / -2.0f);
        DefaultMetavoxelRendererImplementation::getBaseHeightfieldProgram().setUniformValue(
            DefaultMetavoxelRendererImplementation::getBaseColorScaleLocation(), (float)width / innerWidth,
                (float)height / innerHeight);
            
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _colorTextureID);
        
        glDrawRangeElements(GL_TRIANGLES, 0, vertexCount - 1, indexCount, GL_UNSIGNED_INT, 0);
        
        glBindTexture(GL_TEXTURE_2D, 0);
         
        Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(true, false);
        
        DefaultMetavoxelRendererImplementation::getBaseHeightfieldProgram().release();
        
        glDisable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        
        if (heightfield->getMaterial() && !_networkTextures.isEmpty()) {
            glDepthFunc(GL_LEQUAL);
            glDepthMask(false);
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1.0f, -1.0f);
            
            DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().bind();
            const DefaultMetavoxelRendererImplementation::SplatLocations& locations =
                DefaultMetavoxelRendererImplementation::getSplatHeightfieldLocations();
            DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
                locations.heightScale, 1.0f / width, 1.0f / height);
            DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
                locations.textureScale, (float)width / innerWidth, (float)height / innerHeight);
            DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(locations.splatTextureOffset,
                glm::dot(heightfield->getTranslation(), heightfield->getRotation() * glm::vec3(1.0f, 0.0f, 0.0f)) / xScale,
                glm::dot(heightfield->getTranslation(), heightfield->getRotation() * glm::vec3(0.0f, 0.0f, 1.0f)) / zScale);
                
            glBindTexture(GL_TEXTURE_2D, _materialTextureID);
        
            const QVector<SharedObjectPointer>& materials = heightfield->getMaterial()->getMaterials();
            for (int i = 0; i < materials.size(); i += SPLAT_COUNT) {
                QVector4D scalesS, scalesT;
                
                for (int j = 0; j < SPLAT_COUNT; j++) {
                    glActiveTexture(GL_TEXTURE0 + SPLAT_TEXTURE_UNITS[j]);
                    int index = i + j;
                    if (index < _networkTextures.size()) {
                        const NetworkTexturePointer& texture = _networkTextures.at(index);
                        if (texture) {
                            MaterialObject* material = static_cast<MaterialObject*>(materials.at(index).data());
                            scalesS[j] = xScale / material->getScaleS();
                            scalesT[j] = zScale / material->getScaleT();
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
                    (i + 1) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP,
                    (i + 2) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP,
                    (i + 3) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP,
                    (i + 4) * EIGHT_BIT_MAXIMUM_RECIPROCAL - QUARTER_STEP);
                DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().setUniformValue(
                    locations.textureValueMaxima,
                    (i + 1) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP,
                    (i + 2) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP,
                    (i + 3) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP,
                    (i + 4) * EIGHT_BIT_MAXIMUM_RECIPROCAL + QUARTER_STEP);
                glDrawRangeElements(GL_TRIANGLES, 0, vertexCount - 1, indexCount, GL_UNSIGNED_INT, 0);
            }
        
            for (int i = 0; i < SPLAT_COUNT; i++) {
                glActiveTexture(GL_TEXTURE0 + SPLAT_TEXTURE_UNITS[i]);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            
            DefaultMetavoxelRendererImplementation::getSplatHeightfieldProgram().release();
            
            glDisable(GL_POLYGON_OFFSET_FILL);
            glDepthMask(true);
            glDepthFunc(GL_LESS);
        }
        glActiveTexture(GL_TEXTURE0);
        
        glDisable(GL_CULL_FACE);
        
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);    
        glDisableClientState(GL_VERTEX_ARRAY);  
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glPopMatrix();
    
    bufferPair.first.release();
    bufferPair.second.release();
}

void HeightfieldRenderer::applyHeight(const HeightfieldHeightPointer& height) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, _heightTextureID);
    if (height) {
        const QVector<quint16>& contents = height->getContents();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, height->getWidth(), contents.size() / height->getWidth(), 0,
            GL_RED, GL_UNSIGNED_SHORT, contents.constData());
        
    } else {
        const quint16 ZERO_VALUE = 0;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, 1, 1, 0, GL_RED, GL_UNSIGNED_SHORT, &ZERO_VALUE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void HeightfieldRenderer::applyColor(const HeightfieldColorPointer& color) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, _colorTextureID);
    if (color) {
        const QByteArray& contents = color->getContents();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, color->getWidth(),
            contents.size() / (color->getWidth() * DataBlock::COLOR_BYTES), 0, GL_RGB, GL_UNSIGNED_BYTE, contents.constData());
        
    } else {
        const quint8 WHITE_COLOR[] = { 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, WHITE_COLOR);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void HeightfieldRenderer::applyMaterial(const HeightfieldMaterialPointer& material) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, _materialTextureID);
    if (material) {
        const QByteArray& contents = material->getContents();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, material->getWidth(), contents.size() / material->getWidth(), 0,
            GL_RED, GL_UNSIGNED_BYTE, contents.constData());
            
        const QVector<SharedObjectPointer>& materials = material->getMaterials();
        _networkTextures.resize(materials.size());
        for (int i = 0; i < materials.size(); i++) {
            const SharedObjectPointer& material = materials.at(i);
            if (material) {
                _networkTextures[i] = Application::getInstance()->getTextureCache()->getTexture(
                    static_cast<MaterialObject*>(material.data())->getDiffuse(), SPLAT_TEXTURE);
            } else {
                _networkTextures[i].clear();
            }
        }
    } else {
        const quint8 ZERO_VALUE = 0;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &ZERO_VALUE);
        _networkTextures.clear();
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

QHash<HeightfieldRenderer::IntPair, HeightfieldRenderer::BufferPair> HeightfieldRenderer::_bufferPairs;

