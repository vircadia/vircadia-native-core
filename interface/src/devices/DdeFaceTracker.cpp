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
    DdeFaceTracker(QHostAddress::Any, DDE_FEATURE_POINT_SERVER_PORT)
{

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
    _jawOpenIndex(21),
    _host(host),
    _port(port)
{
    _blendshapeCoefficients.resize(NUM_EXPRESSION);
    
    connect(&_udpSocket, SIGNAL(readyRead()), SLOT(readPendingDatagrams()));
    connect(&_udpSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(socketErrorOccurred(QAbstractSocket::SocketError)));
    connect(&_udpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), SLOT(socketStateChanged(QAbstractSocket::SocketState)));
}

DdeFaceTracker::~DdeFaceTracker() {
    if (_udpSocket.isOpen()) {
        _udpSocket.close();
    }
}

void DdeFaceTracker::init() {
    
}

void DdeFaceTracker::reset() {
    _reset  = true;
}

void DdeFaceTracker::update() {
    
}

void DdeFaceTracker::setEnabled(bool enabled) {
    // isOpen() does not work as one might expect on QUdpSocket; don't test isOpen() before closing socket.
    _udpSocket.close();
    if (enabled) {
        _udpSocket.bind(_host, _port);
    }
}

bool DdeFaceTracker::isActive() const {
    static const quint64 ACTIVE_TIMEOUT_USECS = 3000000; //3 secs
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
            state = "Bound";
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
    qDebug() << "[Info] DDE Face Tracker Socket: " << state;
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
        if (_reset || (_lastReceiveTimestamp == 0)) {
            memcpy(&_referenceTranslation, &translation, sizeof(glm::vec3));
            memcpy(&_referenceRotation, &rotation, sizeof(glm::quat));
            _reset = false;
        }

        // Compute relative translation
        float LEAN_DAMPING_FACTOR = 200.0f;
        translation -= _referenceTranslation;
        translation /= LEAN_DAMPING_FACTOR;
        translation.x *= -1;
        _headTranslation = translation;

        // Compute relative rotation
        rotation = glm::inverse(_referenceRotation) * rotation;
        _headRotation = rotation;

        // The DDE coefficients, overall, range from -0.2 to 1.5 or so. However, individual coefficients typically vary much 
        // less than this.

        // Eye blendshapes
        static const float RELAXED_EYE_VALUE = 0.1f;
        float leftEye = packet.expressions[1] - RELAXED_EYE_VALUE;
        float rightEye = packet.expressions[0] - RELAXED_EYE_VALUE;
        if (leftEye > 0.0f) {
            _blendshapeCoefficients[_leftBlinkIndex] = leftEye;
            _blendshapeCoefficients[_leftEyeOpenIndex] = 0.0f;
        } else {
            _blendshapeCoefficients[_leftBlinkIndex] = 0.0f;
            _blendshapeCoefficients[_leftEyeOpenIndex] = -leftEye;
        }
        if (rightEye > 0.0f) {
            _blendshapeCoefficients[_rightBlinkIndex] = rightEye;
            _blendshapeCoefficients[_rightEyeOpenIndex] = 0.0f;
        } else {
            _blendshapeCoefficients[_rightBlinkIndex] = 0.0f;
            _blendshapeCoefficients[_rightEyeOpenIndex] = -rightEye;
        }

        // Eyebrow blendshapes
        float browCenter = packet.expressions[17];
        float browDelta = packet.expressions[24] - packet.expressions[23];
        float browLeft = browCenter - browDelta;
        float browRight = browCenter + browDelta;
        if (browLeft > 0) {
            _blendshapeCoefficients[_browUpLeftIndex] = browLeft;
            _blendshapeCoefficients[_browDownLeftIndex] = 0.0f;
        } else {
            _blendshapeCoefficients[_browUpLeftIndex] = 0.0f;
            _blendshapeCoefficients[_browDownLeftIndex] = -browLeft;
        }
        if (browRight > 0) {
            _blendshapeCoefficients[_browUpRightIndex] = browRight;
            _blendshapeCoefficients[_browDownRightIndex] = 0.0f;
        } else {
            _blendshapeCoefficients[_browUpRightIndex] = 0.0f;
            _blendshapeCoefficients[_browDownRightIndex] = -browRight;
        }
        if (browLeft + browRight > 0) {
            _blendshapeCoefficients[_browUpCenterIndex] = (browLeft + browRight) / 2.0f;
        } else {
            _blendshapeCoefficients[_browUpCenterIndex] = 0.0f;
        }

        // Mouth blendshapes
        _blendshapeCoefficients[_jawOpenIndex] = packet.expressions[21];
        _blendshapeCoefficients[_mouthSmileLeftIndex] = packet.expressions[24];
        _blendshapeCoefficients[_mouthSmileRightIndex] = packet.expressions[23];

    } else {
        qDebug() << "[Error] DDE Face Tracker Decode Error";
    }
    _lastReceiveTimestamp = usecTimestampNow();
}
