//
//  Faceshift.cpp
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 9/3/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QTimer>

#include <PerfStat.h>
#include <SharedUtil.h>

#include "Application.h"
#include "Faceshift.h"
#include "Menu.h"
#include "Util.h"

#ifdef HAVE_FACESHIFT
using namespace fs;
#endif

using namespace std;

const quint16 FACESHIFT_PORT = 33433;

#ifdef HAVE_FACESHIFT

Faceshift::Faceshift() :
    _tcpEnabled(true),
    _tcpRetryCount(0),
    _lastTrackingStateReceived(0),
    _eyeGazeLeftPitch(0.0f),
    _eyeGazeLeftYaw(0.0f),
    _eyeGazeRightPitch(0.0f),
    _eyeGazeRightYaw(0.0f),
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
    _longTermAverageEyePitch(0.0f),
    _longTermAverageEyeYaw(0.0f),
    _longTermAverageInitialized(false)
{
    connect(&_tcpSocket, SIGNAL(connected()), SLOT(noteConnected()));
    connect(&_tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(noteError(QAbstractSocket::SocketError)));
    connect(&_tcpSocket, SIGNAL(readyRead()), SLOT(readFromSocket()));
    connect(&_tcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), SIGNAL(connectionStateChanged()));

    connect(&_udpSocket, SIGNAL(readyRead()), SLOT(readPendingDatagrams()));

    _udpSocket.bind(FACESHIFT_PORT);
}

void Faceshift::init() {
    setTCPEnabled(Menu::getInstance()->isOptionChecked(MenuOption::Faceshift));
}

bool Faceshift::isConnectedOrConnecting() const {
    return _tcpSocket.state() == QAbstractSocket::ConnectedState ||
        (_tcpRetryCount == 0 && _tcpSocket.state() != QAbstractSocket::UnconnectedState);
}

bool Faceshift::isActive() const {
    const quint64 ACTIVE_TIMEOUT_USECS = 1000000;
    return (usecTimestampNow() - _lastTrackingStateReceived) < ACTIVE_TIMEOUT_USECS;
}

#else

Faceshift::Faceshift() {
    
}

bool Faceshift::isConnectedOrConnecting() const {
    return false;
}

bool Faceshift::isActive() const {
    return false;
}

#endif

#ifdef HAVE_FACESHIFT

void Faceshift::update() {
    if (!isActive()) {
        return;
    }
    PerformanceTimer perfTimer("faceshift");
    // get the euler angles relative to the window
    glm::vec3 eulers = glm::degrees(safeEulerAngles(_headRotation * glm::quat(glm::radians(glm::vec3(
        (_eyeGazeLeftPitch + _eyeGazeRightPitch) / 2.0f, (_eyeGazeLeftYaw + _eyeGazeRightYaw) / 2.0f, 0.0f)))));

    // compute and subtract the long term average
    const float LONG_TERM_AVERAGE_SMOOTHING = 0.999f;
    if (!_longTermAverageInitialized) {
        _longTermAverageEyePitch = eulers.x;
        _longTermAverageEyeYaw = eulers.y;
        _longTermAverageInitialized = true;

    } else {
        _longTermAverageEyePitch = glm::mix(eulers.x, _longTermAverageEyePitch, LONG_TERM_AVERAGE_SMOOTHING);
        _longTermAverageEyeYaw = glm::mix(eulers.y, _longTermAverageEyeYaw, LONG_TERM_AVERAGE_SMOOTHING);
    }
    _estimatedEyePitch = eulers.x - _longTermAverageEyePitch;
    _estimatedEyeYaw = eulers.y - _longTermAverageEyeYaw;
}

void Faceshift::reset() {
    if (_tcpSocket.state() == QAbstractSocket::ConnectedState) {
        string message;
        fsBinaryStream::encode_message(message, fsMsgCalibrateNeutral());
        send(message);
    }
    _longTermAverageInitialized = false;
}

void Faceshift::updateFakeCoefficients(float leftBlink, float rightBlink, float browUp,
        float jawOpen, QVector<float>& coefficients) const {
    coefficients.resize(max((int)coefficients.size(), _jawOpenIndex + 1));
    qFill(coefficients.begin(), coefficients.end(), 0.0f);
    coefficients[_leftBlinkIndex] = leftBlink;
    coefficients[_rightBlinkIndex] = rightBlink;
    coefficients[_browUpCenterIndex] = browUp;
    coefficients[_browUpLeftIndex] = browUp;
    coefficients[_browUpRightIndex] = browUp;
    coefficients[_jawOpenIndex] = jawOpen;
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
        if (!_tcpRetryCount) {
            qDebug("Faceshift: Connecting...");
        }

        _tcpSocket.connectToHost("localhost", FACESHIFT_PORT);
        _tracking = false;
    }
}

void Faceshift::noteConnected() {
    qDebug("Faceshift: Connected.");
    // request the list of blendshape names
    string message;
    fsBinaryStream::encode_message(message, fsMsgSendBlendshapeNames());
    send(message);
}

void Faceshift::noteError(QAbstractSocket::SocketError error) {
    if (!_tcpRetryCount) {
       // Only spam log with fail to connect the first time, so that we can keep waiting for server
       qDebug() << "Faceshift: " << _tcpSocket.errorString();
    }
    // retry connection after a 2 second delay
    if (_tcpEnabled) {
        _tcpRetryCount++;
        QTimer::singleShot(2000, this, SLOT(connectSocket()));
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

float Faceshift::getBlendshapeCoefficient(int index) const {
    return (index >= 0 && index < (int)_blendshapeCoefficients.size()) ? _blendshapeCoefficients[index] : 0.0f;
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
                    glm::quat newRotation = glm::quat(data.m_headRotation.w, -data.m_headRotation.x,
                                                      data.m_headRotation.y, -data.m_headRotation.z);
                    // Compute angular velocity of the head
                    glm::quat r = newRotation * glm::inverse(_headRotation);
                    float theta = 2 * acos(r.w);
                    if (theta > EPSILON) {
                        float rMag = glm::length(glm::vec3(r.x, r.y, r.z));
                        float AVERAGE_FACESHIFT_FRAME_TIME = 0.033f;
                        _headAngularVelocity = theta / AVERAGE_FACESHIFT_FRAME_TIME * glm::vec3(r.x, r.y, r.z) / rMag;
                    } else {
                        _headAngularVelocity = glm::vec3(0,0,0);
                    }
                    _headRotation = newRotation;

                    const float TRANSLATION_SCALE = 0.02f;
                    _headTranslation = glm::vec3(data.m_headTranslation.x, data.m_headTranslation.y,
                        -data.m_headTranslation.z) * TRANSLATION_SCALE;
                    _eyeGazeLeftPitch = -data.m_eyeGazeLeftPitch;
                    _eyeGazeLeftYaw = data.m_eyeGazeLeftYaw;
                    _eyeGazeRightPitch = -data.m_eyeGazeRightPitch;
                    _eyeGazeRightYaw = data.m_eyeGazeRightYaw;
                    _blendshapeCoefficients = QVector<float>::fromStdVector(data.m_coeffs);

                    _lastTrackingStateReceived = usecTimestampNow();
                }
                break;
            }
            case fsMsg::MSG_OUT_BLENDSHAPE_NAMES: {
                const vector<string>& names = static_cast<fsMsgBlendshapeNames*>(msg.get())->blendshape_names();
                for (size_t i = 0; i < names.size(); i++) {
                    if (names[i] == "EyeBlink_L") {
                        _leftBlinkIndex = i;

                    } else if (names[i] == "EyeBlink_R") {
                        _rightBlinkIndex = i;

                    } else if (names[i] == "EyeOpen_L") {
                        _leftEyeOpenIndex = i;

                    } else if (names[i] == "EyeOpen_R") {
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
}

#endif
