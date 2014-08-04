//
//  DdeFaceTracker.cpp
//
//
//  Created by Clement on 8/2/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <SharedUtil.h>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QElapsedTimer>

#include "DdeFaceTracker.h"

static const QHostAddress DDE_FEATURE_POINT_SERVER_ADDR("127.0.0.1");
static const quint16 DDE_FEATURE_POINT_SERVER_PORT = 5555;

static const int NUM_EXPRESSION = 46;
static const int MIN_PACKET_SIZE = (8 + NUM_EXPRESSION) * sizeof(float) + sizeof(int);
static const int MAX_NAME_SIZE = 31;

struct Packet{
    //roughly in mm
    float focal_length[1];
    float translation[3];
    
    //quaternion
    float rotation[4];
    
    //blendshape coefficients ranging between -0.2 and 1.5
    float expressions[NUM_EXPRESSION];
    
    //avatar id selected on the UI
    int avatar_id;
    
    //client name, arbitrary length
    char name[MAX_NAME_SIZE + 1];
};

DdeFaceTracker::DdeFaceTracker() :
_lastReceiveTimestamp(0),
_reset(false),
_leftBlinkIndex(0), // see http://support.faceshift.com/support/articles/35129-export-of-blendshapes
_rightBlinkIndex(1),
_leftEyeOpenIndex(8),
_rightEyeOpenIndex(9),
_browDownLeftIndex(14),
_browDownRightIndex(15),
_browUpCenterIndex(16),
_browUpLeftIndex(17),
_browUpRightIndex(18),
_mouthSmileLeftIndex(28),
_mouthSmileRightIndex(29),
_jawOpenIndex(21)
{
    _blendshapeCoefficients.resize(NUM_EXPRESSION);
    
    connect(&_udpSocket, SIGNAL(readyRead()), SLOT(readPendingDatagrams()));
    connect(&_udpSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(socketErrorOccurred(QAbstractSocket::SocketError)));
    connect(&_udpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), SLOT(socketStateChanged(QAbstractSocket::SocketState)));
    
    bindTo(DDE_FEATURE_POINT_SERVER_PORT);
}

DdeFaceTracker::DdeFaceTracker(const QHostAddress& host, quint16 port) :
_lastReceiveTimestamp(0),
_reset(false),
_leftBlinkIndex(0), // see http://support.faceshift.com/support/articles/35129-export-of-blendshapes
_rightBlinkIndex(1),
_leftEyeOpenIndex(8),
_rightEyeOpenIndex(9),
_browDownLeftIndex(14),
_browDownRightIndex(15),
_browUpCenterIndex(16),
_browUpLeftIndex(17),
_browUpRightIndex(18),
_mouthSmileLeftIndex(28),
_mouthSmileRightIndex(29),
_jawOpenIndex(21)
{
    _blendshapeCoefficients.resize(NUM_EXPRESSION);
    
    connect(&_udpSocket, SIGNAL(readyRead()), SLOT(readPendingDatagrams()));
    connect(&_udpSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(socketErrorOccurred(QAbstractSocket::SocketError)));
    connect(&_udpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), SIGNAL(socketStateChanged(QAbstractSocket::SocketState)));
    
    bindTo(host, port);
}

DdeFaceTracker::~DdeFaceTracker() {
    if(_udpSocket.isOpen())
        _udpSocket.close();
}

void DdeFaceTracker::init() {
    
}

void DdeFaceTracker::reset() {
    _reset  = true;
}

void DdeFaceTracker::update() {
    
}

void DdeFaceTracker::bindTo(quint16 port) {
    bindTo(QHostAddress::Any, port);
}

void DdeFaceTracker::bindTo(const QHostAddress& host, quint16 port) {
    if(_udpSocket.isOpen()) {
        _udpSocket.close();
    }
    _udpSocket.bind(host, port);
}

bool DdeFaceTracker::isActive() const {
    static const int ACTIVE_TIMEOUT_USECS = 3000000; //3 secs
    return (usecTimestampNow() - _lastReceiveTimestamp < ACTIVE_TIMEOUT_USECS);
}

//private slots and methods
void DdeFaceTracker::socketErrorOccurred(QAbstractSocket::SocketError socketError) {
    qDebug() << "[Error] DDE Face Tracker Socket Error: " << _udpSocket.errorString();
}

void DdeFaceTracker::socketStateChanged(QAbstractSocket::SocketState socketState) {
    QString state;
    switch(socketState) {
        case QAbstractSocket::BoundState:
            state = "Bounded";
            break;
        case QAbstractSocket::ClosingState:
            state = "Closing";
            break;
        case QAbstractSocket::ConnectedState:
            state = "Connected";
            break;
        case QAbstractSocket::ConnectingState:
            state = "Connecting";
            break;
        case QAbstractSocket::HostLookupState:
            state = "Host Lookup";
            break;
        case QAbstractSocket::ListeningState:
            state = "Listening";
            break;
        case QAbstractSocket::UnconnectedState:
            state = "Unconnected";
            break;
    }
    qDebug() << "[Info] DDE Face Tracker Socket: " << socketState;
}

void DdeFaceTracker::readPendingDatagrams() {
    QByteArray buffer;
    while (_udpSocket.hasPendingDatagrams()) {
        buffer.resize(_udpSocket.pendingDatagramSize());
        _udpSocket.readDatagram(buffer.data(), buffer.size());
    }
    decodePacket(buffer);
}

float DdeFaceTracker::getBlendshapeCoefficient(int index) const {
    return (index >= 0 && index < (int)_blendshapeCoefficients.size()) ? _blendshapeCoefficients[index] : 0.0f;
}

void DdeFaceTracker::decodePacket(const QByteArray& buffer) {
    if(buffer.size() > MIN_PACKET_SIZE) {
        Packet packet;
        int bytesToCopy = glm::min((int)sizeof(packet), buffer.size());
        memset(&packet.name, '\n', MAX_NAME_SIZE + 1);
        memcpy(&packet, buffer.data(), bytesToCopy);
        
        glm::vec3 translation;
        memcpy(&translation, packet.translation, sizeof(packet.translation));
        glm::quat rotation;
        memcpy(&rotation, &packet.rotation, sizeof(packet.rotation));
        if (_reset) {
            memcpy(&_referenceTranslation, &translation, sizeof(glm::vec3));
            memcpy(&_referenceRotation, &rotation, sizeof(glm::quat));
            _reset = false;
        }

        // Compute relative translation
        float DAMPING_FACTOR = 40;
        translation -= _referenceTranslation;
        translation /= DAMPING_FACTOR;
        translation.x *= -1;
        
        // Compute relative rotation
        rotation = glm::inverse(_referenceRotation) * rotation;
        
        // copy values
        _headTranslation = translation;
        _headRotation = rotation;
        
        // Set blendshapes
        _blendshapeCoefficients[_leftBlinkIndex] = packet.expressions[1];
        _blendshapeCoefficients[_rightBlinkIndex] = packet.expressions[0];
        
        _blendshapeCoefficients[_browDownLeftIndex] = packet.expressions[14];
        _blendshapeCoefficients[_browDownRightIndex] = packet.expressions[15];
        _blendshapeCoefficients[_browUpCenterIndex] = (packet.expressions[14] + packet.expressions[14]) / 2.0f;
        _blendshapeCoefficients[_browUpLeftIndex] = packet.expressions[14];
        _blendshapeCoefficients[_browUpRightIndex] = packet.expressions[15];
        
        _blendshapeCoefficients[_jawOpenIndex] = packet.expressions[21];
        
    } else {
        qDebug() << "[Error] DDE Face Tracker Decode Error";
    }
    _lastReceiveTimestamp = usecTimestampNow();
}
