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

static const float DDE_MIN_RANGE = -0.2f;
static const float DDE_MAX_RANGE = 1.5f;
float rescaleCoef(float ddeCoef) {
    return (ddeCoef - DDE_MIN_RANGE) / (DDE_MAX_RANGE - DDE_MIN_RANGE);
}

const int MIN = 0;
const int AVG = 1;
const int MAX = 2;
const float LONG_TERM_AVERAGE = 0.999f;


void resetCoefficient(float * coefficient, float currentValue) {
    coefficient[MIN] = coefficient[MAX] = coefficient[AVG] = currentValue;
}

float updateAndGetCoefficient(float * coefficient, float currentValue, bool scaleToRange = false) {
    coefficient[MIN] = (currentValue < coefficient[MIN]) ? currentValue : coefficient[MIN];
    coefficient[MAX] = (currentValue > coefficient[MAX]) ? currentValue : coefficient[MAX];
    coefficient[AVG] = LONG_TERM_AVERAGE * coefficient[AVG] + (1.f - LONG_TERM_AVERAGE) * currentValue;
    if (coefficient[MAX] > coefficient[MIN]) {
        if (scaleToRange) {
            return glm::clamp((currentValue - coefficient[AVG]) / (coefficient[MAX] - coefficient[MIN]), 0.0f, 1.0f);
        } else {
            return glm::clamp(currentValue - coefficient[AVG], 0.0f, 1.0f);
        }
    } else {
        return 0.0f;
    }
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
            
            resetCoefficient(_rightEye, packet.expressions[0]);
            resetCoefficient(_leftEye, packet.expressions[1]);
            _reset = false;
        }

        // Compute relative translation
        float LEAN_DAMPING_FACTOR = 200.0f;
        translation -= _referenceTranslation;
        translation /= LEAN_DAMPING_FACTOR;
        translation.x *= -1;
        
        // Compute relative rotation
        rotation = glm::inverse(_referenceRotation) * rotation;
        
        // copy values
        _headTranslation = translation;
        _headRotation = rotation;
        
        if (_lastReceiveTimestamp == 0) {
            //  On first packet, reset coefficients
        }
        
        // Set blendshapes
        float EYE_MAGNIFIER = 4.0f;
        float rightEye = glm::clamp((updateAndGetCoefficient(_rightEye, packet.expressions[0])) * EYE_MAGNIFIER, 0.0f, 1.0f);
        _blendshapeCoefficients[_rightBlinkIndex] = rightEye;
        float leftEye = glm::clamp((updateAndGetCoefficient(_leftEye, packet.expressions[1])) * EYE_MAGNIFIER, 0.0f, 1.0f);
        _blendshapeCoefficients[_leftBlinkIndex] = leftEye;

        
        float leftBrow = 1.0f - rescaleCoef(packet.expressions[14]);
        if (leftBrow < 0.5f) {
            _blendshapeCoefficients[_browDownLeftIndex] = 1.0f - 2.0f * leftBrow;
            _blendshapeCoefficients[_browUpLeftIndex] = 0.0f;
        } else {
            _blendshapeCoefficients[_browDownLeftIndex] = 0.0f;
            _blendshapeCoefficients[_browUpLeftIndex] = 2.0f * (leftBrow - 0.5f);
        }
        float rightBrow = 1.0f - rescaleCoef(packet.expressions[15]);
        if (rightBrow < 0.5f) {
            _blendshapeCoefficients[_browDownRightIndex] = 1.0f - 2.0f * rightBrow;
            _blendshapeCoefficients[_browUpRightIndex] = 0.0f;
        } else {
            _blendshapeCoefficients[_browDownRightIndex] = 0.0f;
            _blendshapeCoefficients[_browUpRightIndex] = 2.0f * (rightBrow - 0.5f);
        }
        
        float JAW_OPEN_MAGNIFIER = 1.4f;
        _blendshapeCoefficients[_jawOpenIndex] = rescaleCoef(packet.expressions[21]) * JAW_OPEN_MAGNIFIER;
        
        float SMILE_MULTIPLIER = 2.0f;
        _blendshapeCoefficients[_mouthSmileLeftIndex] = glm::clamp(packet.expressions[24] * SMILE_MULTIPLIER, 0.0f, 1.0f);
        _blendshapeCoefficients[_mouthSmileRightIndex] = glm::clamp(packet.expressions[23] * SMILE_MULTIPLIER, 0.0f, 1.0f);
        
        
    } else {
        qDebug() << "[Error] DDE Face Tracker Decode Error";
    }
    _lastReceiveTimestamp = usecTimestampNow();
}
