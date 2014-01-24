//
//  MetavoxelSystem.cpp
//  interface
//
//  Created by Andrzej Kapolka on 12/10/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QtDebug>

#include <SharedUtil.h>

#include <MetavoxelMessages.h>
#include <MetavoxelUtil.h>

#include "Application.h"
#include "MetavoxelSystem.h"

ProgramObject MetavoxelSystem::_program;
int MetavoxelSystem::_pointScaleLocation;

MetavoxelSystem::MetavoxelSystem() :
    _pointVisitor(_points),
    _buffer(QOpenGLBuffer::VertexBuffer) {
}

void MetavoxelSystem::init() {
    if (!_program.isLinked()) {
        switchToResourcesParentIfRequired();
        _program.addShaderFromSourceFile(QGLShader::Vertex, "resources/shaders/metavoxel_point.vert");
        _program.link();
       
        _pointScaleLocation = _program.uniformLocation("pointScale");
    }
    
    NodeList* nodeList = NodeList::getInstance();
    
    connect(nodeList, SIGNAL(nodeAdded(SharedNodePointer)), SLOT(nodeAdded(SharedNodePointer)));
    connect(nodeList, SIGNAL(nodeKilled(SharedNodePointer)), SLOT(nodeKilled(SharedNodePointer)));
    
    _buffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    _buffer.create();
}

void MetavoxelSystem::processData(const QByteArray& data, const HifiSockAddr& sender) {
    QMetaObject::invokeMethod(this, "receivedData", Q_ARG(const QByteArray&, data), Q_ARG(const HifiSockAddr&, sender));
}

void MetavoxelSystem::simulate(float deltaTime) {
    // simulate the clients
    _points.clear();
    _data.guide(_pointVisitor);
    foreach (MetavoxelClient* client, _clients) {
        client->simulate(deltaTime, _pointVisitor);
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
}

void MetavoxelSystem::nodeAdded(SharedNodePointer node) {
    if (node->getType() == NODE_TYPE_METAVOXEL_SERVER) {
        QMetaObject::invokeMethod(this, "addClient", Q_ARG(const QUuid&, node->getUUID()),
            Q_ARG(const HifiSockAddr&, node->getLocalSocket()));
    }
}

void MetavoxelSystem::nodeKilled(SharedNodePointer node) {
    if (node->getType() == NODE_TYPE_METAVOXEL_SERVER) {
        QMetaObject::invokeMethod(this, "removeClient", Q_ARG(const QUuid&, node->getUUID()));
    }
}

void MetavoxelSystem::addClient(const QUuid& uuid, const HifiSockAddr& address) {
    MetavoxelClient* client = new MetavoxelClient(address);
    _clients.insert(uuid, client);
    _clientsBySessionID.insert(client->getSessionID(), client);
}

void MetavoxelSystem::removeClient(const QUuid& uuid) {
    MetavoxelClient* client = _clients.take(uuid);
    _clientsBySessionID.remove(client->getSessionID());
    delete client;
}

void MetavoxelSystem::receivedData(const QByteArray& data, const HifiSockAddr& sender) {
    int headerPlusIDSize;
    QUuid sessionID = readSessionID(data, sender, headerPlusIDSize);
    if (sessionID.isNull()) {
        return;
    }
    MetavoxelClient* client = _clientsBySessionID.value(sessionID);
    if (client) {
        client->receivedData(data, sender);
    }
}

MetavoxelSystem::PointVisitor::PointVisitor(QVector<Point>& points) :
    MetavoxelVisitor(QVector<AttributePointer>() <<
        AttributeRegistry::getInstance()->getColorAttribute() <<
        AttributeRegistry::getInstance()->getNormalAttribute(),
        QVector<AttributePointer>()),
    _points(points) {
}

bool MetavoxelSystem::PointVisitor::visit(MetavoxelInfo& info) {
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

static QByteArray createDatagramHeader(const QUuid& sessionID) {
    QByteArray header(MAX_PACKET_HEADER_BYTES, 0);
    populateTypeAndVersion(reinterpret_cast<unsigned char*>(header.data()), PACKET_TYPE_METAVOXEL_DATA);
    header += sessionID.toRfc4122();
    return header;
}

MetavoxelClient::MetavoxelClient(const HifiSockAddr& address) :
    _address(address),
    _sessionID(QUuid::createUuid()),
    _sequencer(createDatagramHeader(_sessionID)),
    _data(new MetavoxelData()) {
    
    connect(&_sequencer, SIGNAL(readyToWrite(const QByteArray&)), SLOT(sendData(const QByteArray&)));
    connect(&_sequencer, SIGNAL(readyToRead(Bitstream&)), SLOT(readPacket(Bitstream&)));
    connect(&_sequencer, SIGNAL(receiveAcknowledged(int)), SLOT(clearReceiveRecordsBefore(int)));
    
    // insert the baseline receive record
    ReceiveRecord record = { 0, _data };
    _receiveRecords.append(record);
}

void MetavoxelClient::simulate(float deltaTime, MetavoxelVisitor& visitor) {
    Bitstream& out = _sequencer.startPacket();
    ClientStateMessage state = { Application::getInstance()->getCamera()->getPosition() };
    out << QVariant::fromValue(state);
    _sequencer.endPacket();
    
    _data->guide(visitor);
}

void MetavoxelClient::receivedData(const QByteArray& data, const HifiSockAddr& sender) {
    // save the most recent sender
    _address = sender;
    
    // process through sequencer
    _sequencer.receivedDatagram(data);
}

void MetavoxelClient::sendData(const QByteArray& data) {
    NodeList::getInstance()->getNodeSocket().writeDatagram(data, _address.getAddress(), _address.getPort());
}

void MetavoxelClient::readPacket(Bitstream& in) {
    QVariant message;
    in >> message;
    handleMessage(message, in);
    
    // record the receipt
    ReceiveRecord record = { _sequencer.getIncomingPacketNumber(), _data };
    _receiveRecords.append(record);
}

void MetavoxelClient::clearReceiveRecordsBefore(int index) {
    _receiveRecords.erase(_receiveRecords.begin(), _receiveRecords.begin() + index + 1);
}

void MetavoxelClient::handleMessage(const QVariant& message, Bitstream& in) {
    int userType = message.userType();
    if (userType == MetavoxelDeltaMessage::Type) {
        readDelta(_data, _receiveRecords.first().data, in);
        
    } else if (userType == QMetaType::QVariantList) {
        foreach (const QVariant& element, message.toList()) {
            handleMessage(element, in);
        }
    }
}
