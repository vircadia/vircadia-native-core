//
//  Faceshift.cpp
//  interface
//
//  Created by Andrzej Kapolka on 9/3/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QTimer>

#include <SharedUtil.h>

#include "Faceshift.h"

using namespace fs;
using namespace std;

const quint16 FACESHIFT_PORT = 33433;

Faceshift::Faceshift() :
    _tcpEnabled(false),
    _lastMessageReceived(0),
    _eyeGazeLeftPitch(0.0f),
    _eyeGazeLeftYaw(0.0f),
    _eyeGazeRightPitch(0.0f),
    _eyeGazeRightYaw(0.0f),
    _leftBlink(0.0f),
    _rightBlink(0.0f),
    _leftEyeOpen(0.0f),
    _rightEyeOpen(0.0f),
    _browDownLeft(0.0f),
    _browDownRight(0.0f),
    _browUpCenter(0.0f),
    _browUpLeft(0.0f),
    _browUpRight(0.0f),
    _browDownLeftIndex(-1),
    _browDownRightIndex(-1),
    _browUpLeftIndex(-1),
    _browUpRightIndex(-1),
    _mouthSize(0.0f),
    _mouthSmileLeft(0),
    _mouthSmileRight(0),
    _mouthSmileLeftIndex(-1),
    _mouthSmileRightIndex(0),
    _leftBlinkIndex(0), // see http://support.faceshift.com/support/articles/35129-export-of-blendshapes
    _rightBlinkIndex(1),
    _leftEyeOpenIndex(8),
    _rightEyeOpenIndex(9),
    _browUpCenterIndex(16),
    _jawOpenIndex(21),
    _longTermAverageEyePitch(0.0f),
    _longTermAverageEyeYaw(0.0f),
    _estimatedEyePitch(0.0f),
    _estimatedEyeYaw(0.0f)
{
    connect(&_tcpSocket, SIGNAL(connected()), SLOT(noteConnected()));
    connect(&_tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(noteError(QAbstractSocket::SocketError)));
    connect(&_tcpSocket, SIGNAL(readyRead()), SLOT(readFromSocket()));
    
    connect(&_udpSocket, SIGNAL(readyRead()), SLOT(readPendingDatagrams()));
    
    _udpSocket.bind(FACESHIFT_PORT);
}

bool Faceshift::isActive() const {
    const uint64_t ACTIVE_TIMEOUT_USECS = 1000000;
    return (_tcpSocket.state() == QAbstractSocket::ConnectedState ||
        (usecTimestampNow() - _lastMessageReceived) < ACTIVE_TIMEOUT_USECS) && _tracking;
}

void Faceshift::update() {
    if (!isActive()) {
        return;
    }
    float averageEyePitch = (_eyeGazeLeftPitch + _eyeGazeRightPitch) / 2.0f;
    float averageEyeYaw = (_eyeGazeLeftYaw + _eyeGazeRightYaw) / 2.0f;
    const float LONG_TERM_AVERAGE_SMOOTHING = 0.999f;
    _longTermAverageEyePitch = glm::mix(averageEyePitch, _longTermAverageEyePitch, LONG_TERM_AVERAGE_SMOOTHING);
    _longTermAverageEyeYaw = glm::mix(averageEyeYaw, _longTermAverageEyeYaw, LONG_TERM_AVERAGE_SMOOTHING);
    _estimatedEyePitch = averageEyePitch - _longTermAverageEyePitch;
    _estimatedEyeYaw = averageEyeYaw - _longTermAverageEyeYaw;
}

void Faceshift::reset() {
    if (_tcpSocket.state() == QAbstractSocket::ConnectedState) {
        string message;
        fsBinaryStream::encode_message(message, fsMsgCalibrateNeutral());
        send(message);
    }
}

void Faceshift::setTCPEnabled(bool enabled) {
    if ((_tcpEnabled = enabled)) {
        connectSocket();
    
    } else {
        _tcpSocket.disconnectFromHost();
    }
}

void Faceshift::connectSocket() {
    if (_tcpEnabled) {
        qDebug("Faceshift: Connecting...\n");
    
        _tcpSocket.connectToHost("localhost", FACESHIFT_PORT);
        _tracking = false;
    }
}

void Faceshift::noteConnected() {
    qDebug("Faceshift: Connected.\n");
    
    // request the list of blendshape names
    string message;
    fsBinaryStream::encode_message(message, fsMsgSendBlendshapeNames());
    send(message);
}

void Faceshift::noteError(QAbstractSocket::SocketError error) {
    qDebug() << "Faceshift: " << _tcpSocket.errorString() << "\n";
    
    // reconnect after a delay
    if (_tcpEnabled) {
        QTimer::singleShot(1000, this, SLOT(connectSocket()));
    }
}

void Faceshift::readPendingDatagrams() {
    QByteArray buffer;
    while (_udpSocket.hasPendingDatagrams()) {
        buffer.resize(_udpSocket.pendingDatagramSize());
        _udpSocket.readDatagram(buffer.data(), buffer.size());
        receive(buffer);
    }
}

void Faceshift::readFromSocket() {
    receive(_tcpSocket.readAll());
}

void Faceshift::send(const std::string& message) {
    _tcpSocket.write(message.data(), message.size());
}

void Faceshift::receive(const QByteArray& buffer) {
    _stream.received(buffer.size(), buffer.constData());
    fsMsgPtr msg;
    for (fsMsgPtr msg; (msg = _stream.get_message()); ) {
        switch (msg->id()) {
            case fsMsg::MSG_OUT_TRACKING_STATE: {
                const fsTrackingData& data = static_cast<fsMsgTrackingState*>(msg.get())->tracking_data();
                if ((_tracking = data.m_trackingSuccessful)) {
                    _headRotation = glm::quat(data.m_headRotation.w, -data.m_headRotation.x,
                        data.m_headRotation.y, -data.m_headRotation.z);
                    const float TRANSLATION_SCALE = 0.02f;
                    _headTranslation = glm::vec3(data.m_headTranslation.x, data.m_headTranslation.y,
                        -data.m_headTranslation.z) * TRANSLATION_SCALE;
                    _eyeGazeLeftPitch = -data.m_eyeGazeLeftPitch;
                    _eyeGazeLeftYaw = data.m_eyeGazeLeftYaw;
                    _eyeGazeRightPitch = -data.m_eyeGazeRightPitch;
                    _eyeGazeRightYaw = data.m_eyeGazeRightYaw;

                    if (_leftBlinkIndex != -1) {
                        _leftBlink = data.m_coeffs[_leftBlinkIndex];
                    }
                    if (_rightBlinkIndex != -1) {
                        _rightBlink = data.m_coeffs[_rightBlinkIndex];
                    }
                    if (_leftEyeOpenIndex != -1) {
                        _leftEyeOpen = data.m_coeffs[_leftEyeOpenIndex];
                    }
                    if (_rightEyeOpenIndex != -1) {
                        _rightEyeOpen = data.m_coeffs[_rightEyeOpenIndex];
                    }
                    if (_browDownLeftIndex != -1) {
                        _browDownLeft = data.m_coeffs[_browDownLeftIndex];
                    }
                    if (_browDownRightIndex != -1) {
                        _browDownRight = data.m_coeffs[_browDownRightIndex];
                    }
                    if (_browUpCenterIndex != -1) {
                        _browUpCenter = data.m_coeffs[_browUpCenterIndex];
                    }
                    if (_browUpLeftIndex != -1) {
                        _browUpLeft = data.m_coeffs[_browUpLeftIndex];
                    }
                    if (_browUpRightIndex != -1) {
                        _browUpRight = data.m_coeffs[_browUpRightIndex];
                    }
                    if (_jawOpenIndex != -1) {
                        _mouthSize = data.m_coeffs[_jawOpenIndex];
                    }
                    if (_mouthSmileLeftIndex != -1) {
                        _mouthSmileLeft = data.m_coeffs[_mouthSmileLeftIndex];
                    }
                    if (_mouthSmileRightIndex != -1) {
                        _mouthSmileRight = data.m_coeffs[_mouthSmileRightIndex];
                    }
                }
                break;
            }
            case fsMsg::MSG_OUT_BLENDSHAPE_NAMES: {
                const vector<string>& names = static_cast<fsMsgBlendshapeNames*>(msg.get())->blendshape_names();
                for (int i = 0; i < names.size(); i++) {
                    if (names[i] == "EyeBlink_L") {
                        _leftBlinkIndex = i;
                    
                    } else if (names[i] == "EyeBlink_R") {
                        _rightBlinkIndex = i;

                    }else if (names[i] == "EyeOpen_L") {
                        _leftEyeOpenIndex = i;

                    }else if (names[i] == "EyeOpen_R") {
                        _rightEyeOpenIndex = i;

                    } else if (names[i] == "BrowsD_L") {
                        _browDownLeftIndex = i;

                    } else if (names[i] == "BrowsD_R") {
                        _browDownRightIndex = i;

                    } else if (names[i] == "BrowsU_C") {
                        _browUpCenterIndex = i;

                    } else if (names[i] == "BrowsU_L") {
                        _browUpLeftIndex = i;

                    } else if (names[i] == "BrowsU_R") {
                        _browUpRightIndex = i;

                    } else if (names[i] == "JawOpen") {
                        _jawOpenIndex = i;
                        
                    } else if (names[i] == "MouthSmile_L") {
                        _mouthSmileLeftIndex = i;
                        
                    } else if (names[i] == "MouthSmile_R") {
                        _mouthSmileRightIndex = i;
                        
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    _lastMessageReceived = usecTimestampNow();
}
