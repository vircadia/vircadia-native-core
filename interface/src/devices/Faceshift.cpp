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

#include "Faceshift.h"
#include "Menu.h"
#include "Util.h"

#ifdef HAVE_FACESHIFT
using namespace fs;
#endif

using namespace std;

const quint16 FACESHIFT_PORT = 33433;
float STARTING_FACESHIFT_FRAME_TIME = 0.033f;

Faceshift::Faceshift() :
    _tcpEnabled(true),
    _tcpRetryCount(0),
    _lastTrackingStateReceived(0),
    _averageFrameTime(STARTING_FACESHIFT_FRAME_TIME),
    _headAngularVelocity(0),
    _headLinearVelocity(0),
    _lastHeadTranslation(0),
    _filteredHeadTranslation(0),
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
#ifdef HAVE_FACESHIFT
    connect(&_tcpSocket, SIGNAL(connected()), SLOT(noteConnected()));
    connect(&_tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(noteError(QAbstractSocket::SocketError)));
    connect(&_tcpSocket, SIGNAL(readyRead()), SLOT(readFromSocket()));
    connect(&_tcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), SIGNAL(connectionStateChanged()));

    connect(&_udpSocket, SIGNAL(readyRead()), SLOT(readPendingDatagrams()));

    _udpSocket.bind(FACESHIFT_PORT);
#endif
}

void Faceshift::init() {
#ifdef HAVE_FACESHIFT
    setTCPEnabled(Menu::getInstance()->isOptionChecked(MenuOption::Faceshift));
#endif
}

bool Faceshift::isConnectedOrConnecting() const {
    return _tcpSocket.state() == QAbstractSocket::ConnectedState ||
    (_tcpRetryCount == 0 && _tcpSocket.state() != QAbstractSocket::UnconnectedState);
}

bool Faceshift::isActive() const {
#ifdef HAVE_FACESHIFT
    const quint64 ACTIVE_TIMEOUT_USECS = 1000000;
    return (usecTimestampNow() - _lastTrackingStateReceived) < ACTIVE_TIMEOUT_USECS;
#else
    return false;
#endif
}

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
#ifdef HAVE_FACESHIFT
    if (_tcpSocket.state() == QAbstractSocket::ConnectedState) {
        string message;
        fsBinaryStream::encode_message(message, fsMsgCalibrateNeutral());
        send(message);
    }
    _longTermAverageInitialized = false;
#endif
}

void Faceshift::updateFakeCoefficients(float leftBlink, float rightBlink, float browUp,
        float jawOpen, float mouth2, float mouth3, float mouth4, QVector<float>& coefficients) const {
    const int MMMM_BLENDSHAPE = 34;
    const int FUNNEL_BLENDSHAPE = 40;
    const int SMILE_LEFT_BLENDSHAPE = 28;
    const int SMILE_RIGHT_BLENDSHAPE = 29;
    const int MAX_FAKE_BLENDSHAPE = 40;  //  Largest modified blendshape from above and below

    coefficients.resize(max((int)coefficients.size(), MAX_FAKE_BLENDSHAPE + 1));
    qFill(coefficients.begin(), coefficients.end(), 0.0f);
    coefficients[_leftBlinkIndex] = leftBlink;
    coefficients[_rightBlinkIndex] = rightBlink;
    coefficients[_browUpCenterIndex] = browUp;
    coefficients[_browUpLeftIndex] = browUp;
    coefficients[_browUpRightIndex] = browUp;
    coefficients[_jawOpenIndex] = jawOpen;
    coefficients[SMILE_LEFT_BLENDSHAPE] = coefficients[SMILE_RIGHT_BLENDSHAPE] = mouth4;
    coefficients[MMMM_BLENDSHAPE] = mouth2;
    coefficients[FUNNEL_BLENDSHAPE] = mouth3;
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

        _tcpSocket.connectToHost(Menu::getInstance()->getFaceshiftHostname(), FACESHIFT_PORT);
        _tracking = false;
    }
}

void Faceshift::noteConnected() {
#ifdef HAVE_FACESHIFT
    qDebug("Faceshift: Connected.");
    // request the list of blendshape names
    string message;
    fsBinaryStream::encode_message(message, fsMsgSendBlendshapeNames());
    send(message);
#endif
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
#ifdef HAVE_FACESHIFT
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
                        _headAngularVelocity = theta / _averageFrameTime * glm::vec3(r.x, r.y, r.z) / rMag;
                    } else {
                        _headAngularVelocity = glm::vec3(0,0,0);
                    }
                    const float ANGULAR_VELOCITY_FILTER_STRENGTH = 0.3f;
                    _headRotation = safeMix(_headRotation, newRotation, glm::clamp(glm::length(_headAngularVelocity) *
                                                                                   ANGULAR_VELOCITY_FILTER_STRENGTH, 0.0f, 1.0f));

                    const float TRANSLATION_SCALE = 0.02f;
                    glm::vec3 newHeadTranslation = glm::vec3(data.m_headTranslation.x, data.m_headTranslation.y,
                                                            -data.m_headTranslation.z) * TRANSLATION_SCALE;
                    
                    _headLinearVelocity = (newHeadTranslation - _lastHeadTranslation) / _averageFrameTime;
                    
                    const float LINEAR_VELOCITY_FILTER_STRENGTH = 0.3f;
                    float velocityFilter = glm::clamp(1.0f - glm::length(_headLinearVelocity) *
                                                      LINEAR_VELOCITY_FILTER_STRENGTH, 0.0f, 1.0f);
                    _filteredHeadTranslation = velocityFilter * _filteredHeadTranslation + (1.0f - velocityFilter) * newHeadTranslation;
                    
                    _lastHeadTranslation = newHeadTranslation;
                    _headTranslation = _filteredHeadTranslation;
                    
                    _eyeGazeLeftPitch = -data.m_eyeGazeLeftPitch;
                    _eyeGazeLeftYaw = data.m_eyeGazeLeftYaw;
                    _eyeGazeRightPitch = -data.m_eyeGazeRightPitch;
                    _eyeGazeRightYaw = data.m_eyeGazeRightYaw;
                    _blendshapeCoefficients = QVector<float>::fromStdVector(data.m_coeffs);

                    const float FRAME_AVERAGING_FACTOR = 0.99f;
                    quint64 usecsNow = usecTimestampNow();
                    if (_lastTrackingStateReceived != 0) {
                        _averageFrameTime = FRAME_AVERAGING_FACTOR * _averageFrameTime +
                        (1.0f - FRAME_AVERAGING_FACTOR) * (float)(usecsNow - _lastTrackingStateReceived) / 1000000.0f;
                    }
                    _lastTrackingStateReceived = usecsNow;
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
#endif
}
