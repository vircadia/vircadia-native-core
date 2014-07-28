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

REGISTER_META_OBJECT(PointMetavoxelRendererImplementation)
REGISTER_META_OBJECT(SphereRenderer)
REGISTER_META_OBJECT(StaticModelRenderer)

static int bufferPointVectorMetaTypeId = qRegisterMetaType<BufferPointVector>();

void MetavoxelSystem::init() {
    MetavoxelClientManager::init();
    PointMetavoxelRendererImplementation::init();
    _pointBufferAttribute = AttributeRegistry::getInstance()->registerAttribute(new PointBufferAttribute());
}

MetavoxelLOD MetavoxelSystem::getLOD() {
    QReadLocker locker(&_lodLock);
    return _lod;
}

class SpannerSimulateVisitor : public SpannerVisitor {
public:
    
    SpannerSimulateVisitor(float deltaTime);
    
    virtual bool visit(Spanner* spanner, const glm::vec3& clipMinimum, float clipSize);

private:
    
    float _deltaTime;
};

SpannerSimulateVisitor::SpannerSimulateVisitor(float deltaTime) :
    SpannerVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getSpannersAttribute(),
        QVector<AttributePointer>(), QVector<AttributePointer>(), QVector<AttributePointer>(),
            Application::getInstance()->getMetavoxels()->getLOD()),
    _deltaTime(deltaTime) {
}

bool SpannerSimulateVisitor::visit(Spanner* spanner, const glm::vec3& clipMinimum, float clipSize) {
    spanner->getRenderer()->simulate(_deltaTime);
    return true;
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

    SpannerSimulateVisitor spannerSimulateVisitor(deltaTime);
    guide(spannerSimulateVisitor);
}

class SpannerRenderVisitor : public SpannerVisitor {
public:
    
    SpannerRenderVisitor();
    
    virtual bool visit(Spanner* spanner, const glm::vec3& clipMinimum, float clipSize);
};

SpannerRenderVisitor::SpannerRenderVisitor() :
    SpannerVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getSpannersAttribute(),
        QVector<AttributePointer>(), QVector<AttributePointer>(), QVector<AttributePointer>(),
        Application::getInstance()->getMetavoxels()->getLOD(),
        encodeOrder(Application::getInstance()->getViewFrustum()->getDirection())) {
}

bool SpannerRenderVisitor::visit(Spanner* spanner, const glm::vec3& clipMinimum, float clipSize) {
    spanner->getRenderer()->render(1.0f, SpannerRenderer::DEFAULT_MODE, clipMinimum, clipSize);
    return true;
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
    RenderVisitor renderVisitor(getLOD());
    guideToAugmented(renderVisitor);
    
    SpannerRenderVisitor spannerRenderVisitor;
    guide(spannerRenderVisitor);
}

MetavoxelClient* MetavoxelSystem::createClient(const SharedNodePointer& node) {
    return new MetavoxelSystemClient(node, _updater);
}

void MetavoxelSystem::guideToAugmented(MetavoxelVisitor& visitor) {
    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        if (node->getType() == NodeType::MetavoxelServer) {
            QMutexLocker locker(&node->getMutex());
            MetavoxelSystemClient* client = static_cast<MetavoxelSystemClient*>(node->getLinkedData());
            if (client) {
                client->getAugmentedData().guide(visitor);
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

PointBuffer::PointBuffer(const BufferPointVector& points) :
    _points(points) {
}

void PointBuffer::render() {
    // initalize buffer, etc. on first render
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

PointBufferAttribute::PointBufferAttribute() :
    InlineAttribute<PointBufferPointer>("pointBuffer") {
}

bool PointBufferAttribute::merge(void*& parent, void* children[], bool postRead) const {
    PointBufferPointer firstChild = decodeInline<PointBufferPointer>(children[0]);
    for (int i = 1; i < MERGE_COUNT; i++) {
        if (firstChild != decodeInline<PointBufferPointer>(children[i])) {
            *(PointBufferPointer*)&parent = _defaultValue;
            return false;
        }
    }
    *(PointBufferPointer*)&parent = firstChild;
    return true;
}

void PointMetavoxelRendererImplementation::init() {
    if (!_program.isLinked()) {
        _program.addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() + "shaders/metavoxel_point.vert");
        _program.link();
       
        _program.bind();
        _pointScaleLocation = _program.uniformLocation("pointScale");
        _program.release();
    }
}

PointMetavoxelRendererImplementation::PointMetavoxelRendererImplementation() {
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
        BufferPointVector swapPoints;
        _points.swap(swapPoints);
        info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline(PointBufferPointer(
            new PointBuffer(swapPoints))));
    }
    return STOP_RECURSION;
}

bool PointAugmentVisitor::postVisit(MetavoxelInfo& info) {
    if (info.size != _pointLeafSize) {
        return false;
    }
    BufferPointVector swapPoints;
    _points.swap(swapPoints);
    info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline(PointBufferPointer(
        new PointBuffer(swapPoints))));
    return true;
}

void PointMetavoxelRendererImplementation::augment(MetavoxelData& data, const MetavoxelData& previous,
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
    
    PointAugmentVisitor visitor(lod);
    data.guideToDifferent(expandedPrevious, visitor);
}

class PointRenderVisitor : public MetavoxelVisitor {
public:
    
    PointRenderVisitor(const MetavoxelLOD& lod);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    int _order;
};

PointRenderVisitor::PointRenderVisitor(const MetavoxelLOD& lod) :
    MetavoxelVisitor(QVector<AttributePointer>() << Application::getInstance()->getMetavoxels()->getPointBufferAttribute(),
        QVector<AttributePointer>(), lod),
    _order(encodeOrder(Application::getInstance()->getViewFrustum()->getDirection())) {
}

int PointRenderVisitor::visit(MetavoxelInfo& info) {
    PointBufferPointer buffer = info.inputValues.at(0).getInlineValue<PointBufferPointer>();
    if (buffer) {
        buffer->render();
    }
    return info.isLeaf ? STOP_RECURSION : _order;
}

void PointMetavoxelRendererImplementation::render(MetavoxelData& data, MetavoxelInfo& info, const MetavoxelLOD& lod) {
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int VIEWPORT_WIDTH_INDEX = 2;
    const int VIEWPORT_HEIGHT_INDEX = 3;
    float viewportWidth = viewport[VIEWPORT_WIDTH_INDEX];
    float viewportHeight = viewport[VIEWPORT_HEIGHT_INDEX];
    float viewportDiagonal = sqrtf(viewportWidth * viewportWidth + viewportHeight * viewportHeight);
    float worldDiagonal = glm::distance(Application::getInstance()->getViewFrustum()->getNearBottomLeft(),
        Application::getInstance()->getViewFrustum()->getNearTopRight());

    _program.bind();
    _program.setUniformValue(_pointScaleLocation, viewportDiagonal *
        Application::getInstance()->getViewFrustum()->getNearClip() / worldDiagonal);
        
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);

    glDisable(GL_BLEND);
    
    PointRenderVisitor visitor(lod);
    data.guide(visitor);
    
    glEnable(GL_BLEND);
    
    glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    _program.release();
}
 
ProgramObject PointMetavoxelRendererImplementation::_program;
int PointMetavoxelRendererImplementation::_pointScaleLocation;

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
