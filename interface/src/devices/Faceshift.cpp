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
    _browHeight(0.0f),
    _browUpCenterIndex(-1),
    _mouthSize(0.0f),
    _jawOpenIndex(-1),
    _leftEyeUpIndex(-1),
    _leftEyeInIndex(-1),
    _rightEyeUpIndex(-1),
    _rightEyeInIndex(-1)
{
    connect(&_socket, SIGNAL(connected()), SLOT(noteConnected()));
    connect(&_socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(noteError(QAbstractSocket::SocketError)));
    connect(&_socket, SIGNAL(readyRead()), SLOT(readFromSocket()));
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
                if (data.m_trackingSuccessful) {
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
                    if (_browUpCenterIndex != -1) {
                        _browHeight = data.m_coeffs[_browUpCenterIndex];
                    }
                    if (_jawOpenIndex != -1) {
                        _mouthSize = data.m_coeffs[_jawOpenIndex];
                    }
                    const float PITCH_SCALE = 45.0f;
                    if (_leftEyeUpIndex != -1) {
                        _eyeGazeLeftPitch = PITCH_SCALE * (data.m_coeffs[_leftEyeUpIndex] - data.m_coeffs[_leftEyeDownIndex]);
                    }
                    const float YAW_SCALE = 45.0f;
                    if (_leftEyeInIndex != -1) {
                        _eyeGazeLeftYaw = YAW_SCALE * (data.m_coeffs[_leftEyeOutIndex] - data.m_coeffs[_leftEyeInIndex]);
                    }
                    if (_rightEyeUpIndex != -1) {
                        _eyeGazeRightPitch = PITCH_SCALE * (data.m_coeffs[_rightEyeUpIndex] -
                            data.m_coeffs[_rightEyeDownIndex]);
                    }
                    if (_rightEyeInIndex != -1) {
                        _eyeGazeRightYaw = YAW_SCALE * (data.m_coeffs[_rightEyeInIndex] - data.m_coeffs[_rightEyeOutIndex]);
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
                        
                    } else if (names[i] == "BrowsU_C") {
                        _browUpCenterIndex = i;    
                        
                    } else if (names[i] == "JawOpen") {
                        _jawOpenIndex = i;
                        
                    } else if (names[i] == "EyeUp_L") {
                        _leftEyeUpIndex = i;
                        
                    } else if (names[i] == "EyeDown_L") {
                        _leftEyeDownIndex = i;
                        
                    } else if (names[i] == "EyeIn_L") {
                        _leftEyeInIndex = i;
                        
                    } else if (names[i] == "EyeOut_L") {
                        _leftEyeOutIndex = i;
                    
                    } else if (names[i] == "EyeUp_R") {
                        _rightEyeUpIndex = i;
                        
                    } else if (names[i] == "EyeDown_R") {
                        _rightEyeDownIndex = i;
                        
                    } else if (names[i] == "EyeIn_R") {
                        _rightEyeInIndex = i;
                        
                    } else if (names[i] == "EyeOut_R") {
                        _rightEyeOutIndex = i;
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
