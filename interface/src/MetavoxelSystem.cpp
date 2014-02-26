//
//  MetavoxelSystem.cpp
//  interface
//
//  Created by Andrzej Kapolka on 12/10/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QMutexLocker>
#include <QtDebug>

#include <glm/gtx/transform.hpp>

#include <SharedUtil.h>

#include <MetavoxelUtil.h>
#include <ScriptCache.h>

#include "Application.h"
#include "MetavoxelSystem.h"
#include "renderer/Model.h"

REGISTER_META_OBJECT(SphereRenderer)
REGISTER_META_OBJECT(StaticModelRenderer)

ProgramObject MetavoxelSystem::_program;
int MetavoxelSystem::_pointScaleLocation;

MetavoxelSystem::MetavoxelSystem() :
    _simulateVisitor(_points),
    _buffer(QOpenGLBuffer::VertexBuffer) {
}

void MetavoxelSystem::init() {
    if (!_program.isLinked()) {
        switchToResourcesParentIfRequired();
        _program.addShaderFromSourceFile(QGLShader::Vertex, "resources/shaders/metavoxel_point.vert");
        _program.link();
       
        _pointScaleLocation = _program.uniformLocation("pointScale");
        
        // let the script cache know to use our common access manager
        ScriptCache::getInstance()->setNetworkAccessManager(Application::getInstance()->getNetworkAccessManager());
    }
    _buffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    _buffer.create();
    
    connect(NodeList::getInstance(), SIGNAL(nodeAdded(SharedNodePointer)), SLOT(maybeAttachClient(const SharedNodePointer&)));
}

void MetavoxelSystem::applyEdit(const MetavoxelEditMessage& edit) {
    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        if (node->getType() == NodeType::MetavoxelServer) {
            QMutexLocker locker(&node->getMutex());
            MetavoxelClient* client = static_cast<MetavoxelClient*>(node->getLinkedData());
            if (client) {
                client->applyEdit(edit);
            }
        }
    }
}

void MetavoxelSystem::simulate(float deltaTime) {
    // simulate the clients
    _points.clear();
    _simulateVisitor.setDeltaTime(deltaTime);
    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        if (node->getType() == NodeType::MetavoxelServer) {
            QMutexLocker locker(&node->getMutex());
            MetavoxelClient* client = static_cast<MetavoxelClient*>(node->getLinkedData());
            if (client) {
                client->simulate(deltaTime);
                client->getData().guide(_simulateVisitor);
            }
        }
    }
    
    _buffer.bind();
    int bytes = _points.size() * sizeof(Point);
    if (_buffer.size() < bytes) {
        _buffer.allocate(_points.constData(), bytes);
    } else {
        _buffer.write(0, _points.constData(), bytes);
    }
    _buffer.release();
}

void MetavoxelSystem::render() {
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int VIEWPORT_WIDTH_INDEX = 2;
    const int VIEWPORT_HEIGHT_INDEX = 3;
    float viewportWidth = viewport[VIEWPORT_WIDTH_INDEX];
    float viewportHeight = viewport[VIEWPORT_HEIGHT_INDEX];
    float viewportDiagonal = sqrtf(viewportWidth*viewportWidth + viewportHeight*viewportHeight);
    float worldDiagonal = glm::distance(Application::getInstance()->getViewFrustum()->getNearBottomLeft(),
        Application::getInstance()->getViewFrustum()->getNearTopRight());

    _program.bind();
    _program.setUniformValue(_pointScaleLocation, viewportDiagonal *
        Application::getInstance()->getViewFrustum()->getNearClip() / worldDiagonal);
        
    _buffer.bind();

    Point* pt = 0;
    glVertexPointer(4, GL_FLOAT, sizeof(Point), &pt->vertex);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Point), &pt->color);
    glNormalPointer(GL_BYTE, sizeof(Point), &pt->normal);    

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
    
    glDrawArrays(GL_POINTS, 0, _points.size());
    
    glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    _buffer.release();
    
    _program.release();
    
    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        if (node->getType() == NodeType::MetavoxelServer) {
            QMutexLocker locker(&node->getMutex());
            MetavoxelClient* client = static_cast<MetavoxelClient*>(node->getLinkedData());
            if (client) {
                client->getData().guide(_renderVisitor);
            }
        }
    }
}

void MetavoxelSystem::maybeAttachClient(const SharedNodePointer& node) {
    if (node->getType() == NodeType::MetavoxelServer) {
        QMutexLocker locker(&node->getMutex());
        node->setLinkedData(new MetavoxelClient(NodeList::getInstance()->nodeWithUUID(node->getUUID())));
    }
}

MetavoxelSystem::SimulateVisitor::SimulateVisitor(QVector<Point>& points) :
    SpannerVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getSpannersAttribute(),
        QVector<AttributePointer>() << AttributeRegistry::getInstance()->getColorAttribute() <<
            AttributeRegistry::getInstance()->getNormalAttribute()),
    _points(points) {
}

void MetavoxelSystem::SimulateVisitor::visit(Spanner* spanner) {
    spanner->getRenderer()->simulate(_deltaTime);
}

bool MetavoxelSystem::SimulateVisitor::visit(MetavoxelInfo& info) {
    SpannerVisitor::visit(info);

    if (!info.isLeaf) {
        return true;
    }
    QRgb color = info.inputValues.at(0).getInlineValue<QRgb>();
    QRgb normal = info.inputValues.at(1).getInlineValue<QRgb>();
    int alpha = qAlpha(color);
    if (alpha > 0) {
        Point point = { glm::vec4(info.minimum + glm::vec3(info.size, info.size, info.size) * 0.5f, info.size),
            { qRed(color), qGreen(color), qBlue(color), alpha }, { qRed(normal), qGreen(normal), qBlue(normal) } };
        _points.append(point);
    }
    return false;
}

MetavoxelSystem::RenderVisitor::RenderVisitor() :
    SpannerVisitor(QVector<AttributePointer>() << AttributeRegistry::getInstance()->getSpannersAttribute()) {
}

void MetavoxelSystem::RenderVisitor::visit(Spanner* spanner) {
    spanner->getRenderer()->render(1.0f);
}

MetavoxelClient::MetavoxelClient(const SharedNodePointer& node) :
    _node(node),
    _sequencer(byteArrayWithPopulatedHeader(PacketTypeMetavoxelData)) {
    
    connect(&_sequencer, SIGNAL(readyToWrite(const QByteArray&)), SLOT(sendData(const QByteArray&)));
    connect(&_sequencer, SIGNAL(readyToRead(Bitstream&)), SLOT(readPacket(Bitstream&)));
    connect(&_sequencer, SIGNAL(receiveAcknowledged(int)), SLOT(clearReceiveRecordsBefore(int)));
    
    // insert the baseline receive record
    ReceiveRecord record = { 0, _data };
    _receiveRecords.append(record);
}

MetavoxelClient::~MetavoxelClient() {
    // close the session
    Bitstream& out = _sequencer.startPacket();
    out << QVariant::fromValue(CloseSessionMessage());
    _sequencer.endPacket();
}

void MetavoxelClient::applyEdit(const MetavoxelEditMessage& edit) {
    // apply immediately to local tree
    edit.apply(_data);

    // start sending it out
    _sequencer.sendHighPriorityMessage(QVariant::fromValue(edit));
}

void MetavoxelClient::simulate(float deltaTime) {
    Bitstream& out = _sequencer.startPacket();
    ClientStateMessage state = { Application::getInstance()->getCamera()->getPosition() };
    out << QVariant::fromValue(state);
    _sequencer.endPacket();
}

int MetavoxelClient::parseData(const QByteArray& packet) {
    // process through sequencer
    QMetaObject::invokeMethod(&_sequencer, "receivedDatagram", Q_ARG(const QByteArray&, packet));
    return packet.size();
}

void MetavoxelClient::sendData(const QByteArray& data) {
    NodeList::getInstance()->writeDatagram(data, _node);
}

void MetavoxelClient::readPacket(Bitstream& in) {
    QVariant message;
    in >> message;
    handleMessage(message, in);
    
    // record the receipt
    ReceiveRecord record = { _sequencer.getIncomingPacketNumber(), _data };
    _receiveRecords.append(record);
    
    // reapply local edits
    foreach (const DatagramSequencer::HighPriorityMessage& message, _sequencer.getHighPriorityMessages()) {
        if (message.data.userType() == MetavoxelEditMessage::Type) {
            message.data.value<MetavoxelEditMessage>().apply(_data);
        }
    }
}

void MetavoxelClient::clearReceiveRecordsBefore(int index) {
    _receiveRecords.erase(_receiveRecords.begin(), _receiveRecords.begin() + index + 1);
}

void MetavoxelClient::handleMessage(const QVariant& message, Bitstream& in) {
    int userType = message.userType();
    if (userType == MetavoxelDeltaMessage::Type) {
        _data.readDelta(_receiveRecords.first().data, in);
        
    } else if (userType == QMetaType::QVariantList) {
        foreach (const QVariant& element, message.toList()) {
            handleMessage(element, in);
        }
    }
}

SphereRenderer::SphereRenderer() {
}

void SphereRenderer::render(float alpha) {
    Sphere* sphere = static_cast<Sphere*>(parent());
    const QColor& color = sphere->getColor();
    glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF() * alpha);
    
    glPushMatrix();
    const glm::vec3& translation = sphere->getTranslation();
    glTranslatef(translation.x, translation.y, translation.z);
    glm::quat rotation = glm::quat(glm::radians(sphere->getRotation()));
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(glm::angle(rotation), axis.x, axis.y, axis.z);
    
    glutSolidSphere(sphere->getScale(), 10, 10);
    
    glPopMatrix();
}

StaticModelRenderer::StaticModelRenderer() :
    _model(new Model(this)) {
}

void StaticModelRenderer::init(Spanner* spanner) {
    _model->init();
    
    StaticModel* staticModel = static_cast<StaticModel*>(spanner);
    applyTranslation(staticModel->getTranslation());
    applyRotation(staticModel->getRotation());
    applyScale(staticModel->getScale());
    applyURL(staticModel->getURL());
    
    connect(spanner, SIGNAL(translationChanged(const glm::vec3&)), SLOT(applyTranslation(const glm::vec3&)));
    connect(spanner, SIGNAL(rotationChanged(const glm::vec3&)), SLOT(applyRotation(const glm::vec3&)));
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
    static_cast<StaticModel*>(parent())->setBounds(glm::translate(_model->getTranslation()) *
        glm::mat4_cast(_model->getRotation()) * glm::scale(_model->getScale()) * bounds);
    _model->simulate(deltaTime);
}

void StaticModelRenderer::render(float alpha) {
    _model->render(alpha);
}

void StaticModelRenderer::applyTranslation(const glm::vec3& translation) {
    _model->setTranslation(translation);
}

void StaticModelRenderer::applyRotation(const glm::vec3& rotation) {
    _model->setRotation(glm::quat(glm::radians(rotation)));
}

void StaticModelRenderer::applyScale(float scale) {
    const float SCALE_MULTIPLIER = 0.0006f;
    _model->setScale(glm::vec3(scale, scale, scale) * SCALE_MULTIPLIER);
}

void StaticModelRenderer::applyURL(const QUrl& url) {
    _model->setURL(url);
}
