//
//  Faceshift.cpp
//  interface
//
//  Created by Andrzej Kapolka on 9/3/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QTimer>

#include "Faceshift.h"

using namespace fs;
using namespace std;

Faceshift::Faceshift() :
    _enabled(false),
    _eyeGazeLeftPitch(0.0f),
    _eyeGazeLeftYaw(0.0f),
    _eyeGazeRightPitch(0.0f),
    _eyeGazeRightYaw(0.0f),
    _leftBlink(0.0f),
    _rightBlink(0.0f),
    _leftBlinkIndex(-1),
    _rightBlinkIndex(-1),
    _browDownLeft(0.0f),
    _browDownRight(0.0f),
    _browHeight(0.0f),
    _browUpLeft(0.0f),
    _browUpRight(0.0f),
    _browDownLeftIndex(-1),
    _browDownRightIndex(-1),
    _browUpCenterIndex(-1),
    _browUpLeftIndex(-1),
    _browUpRightIndex(-1),
    _mouthSize(0.0f),
    _jawOpenIndex(-1),
    _longTermAverageEyePitch(0.0f),
    _longTermAverageEyeYaw(0.0f),
    _estimatedEyePitch(0.0f),
    _estimatedEyeYaw(0.0f)
{
    connect(&_socket, SIGNAL(connected()), SLOT(noteConnected()));
    connect(&_socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(noteError(QAbstractSocket::SocketError)));
    connect(&_socket, SIGNAL(readyRead()), SLOT(readFromSocket()));
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
    if (isActive()) {
        string message;
        fsBinaryStream::encode_message(message, fsMsgCalibrateNeutral());
        send(message);
    }
}

void Faceshift::setEnabled(bool enabled) {
    if ((_enabled = enabled)) {
        connectSocket();
    
    } else {
        _socket.disconnectFromHost();
    }
}

void Faceshift::connectSocket() {
    if (_enabled) {
        qDebug("Faceshift: Connecting...\n");
    
        const quint16 FACESHIFT_PORT = 33433;
        _socket.connectToHost("localhost", FACESHIFT_PORT);
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
    qDebug() << "Faceshift: " << _socket.errorString() << "\n";
    
    // reconnect after a delay
    if (_enabled) {
        QTimer::singleShot(1000, this, SLOT(connectSocket()));
    }
}

void Faceshift::readFromSocket() {
    QByteArray buffer = _socket.readAll();
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
                    if (_browDownLeftIndex != -1) {
                        _browDownLeft = data.m_coeffs[_browDownLeftIndex];
                    }
                    if (_browDownRightIndex != -1) {
                        _browDownRight = data.m_coeffs[_browDownRightIndex];
                    }
                    if (_browUpCenterIndex != -1) {
                        _browHeight = data.m_coeffs[_browUpCenterIndex];
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
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

void Faceshift::send(const std::string& message) {
    _socket.write(message.data(), message.size());
}
