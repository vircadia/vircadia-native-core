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

#include "DdeFaceTracker.h"

#include <SharedUtil.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>

#include <GLMHelpers.h>
#include <NumericalConstants.h>
#include <FaceshiftConstants.h>

#include "Application.h"
#include "InterfaceLogging.h"
#include "Menu.h"


static const QHostAddress DDE_SERVER_ADDR("127.0.0.1");
static const quint16 DDE_SERVER_PORT = 64204;
static const quint16 DDE_CONTROL_PORT = 64205;
#if defined(Q_OS_WIN)
static const QString DDE_PROGRAM_PATH = "/dde/dde.exe";
#elif defined(Q_OS_MAC)
static const QString DDE_PROGRAM_PATH = "/dde.app/Contents/MacOS/dde";
#endif
static const QStringList DDE_ARGUMENTS = QStringList() 
    << "--udp=" + DDE_SERVER_ADDR.toString() + ":" + QString::number(DDE_SERVER_PORT) 
    << "--receiver=" + QString::number(DDE_CONTROL_PORT)
    << "--facedet_interval=500"  // ms
    << "--headless";

static const int NUM_EXPRESSIONS = 46;
static const int MIN_PACKET_SIZE = (8 + NUM_EXPRESSIONS) * sizeof(float) + sizeof(int);
static const int MAX_NAME_SIZE = 31;

// There's almost but not quite a 1-1 correspondence between DDE's 46 and Faceshift 1.3's 48 packets.
// The best guess at mapping is to:
// - Swap L and R values
// - Skip two Faceshift values: JawChew (22) and LipsLowerDown (37)
static const int DDE_TO_FACESHIFT_MAPPING[] = {
    1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14,
    16,
    18, 17,
    19,
    23,
    21,
    // Skip JawChew
    20,
    25, 24, 27, 26, 29, 28, 31, 30, 33, 32,
    34, 35, 36,
    // Skip LipsLowerDown
    38, 39, 40, 41, 42, 43, 44, 45,
    47, 46
};

// The DDE coefficients, overall, range from -0.2 to 1.5 or so. However, individual coefficients typically vary much 
// less than this.
static const float DDE_COEFFICIENT_SCALES[] = {
    1.0f, // EyeBlink_L
    1.0f, // EyeBlink_R
    1.0f, // EyeSquint_L
    1.0f, // EyeSquint_R
    1.0f, // EyeDown_L
    1.0f, // EyeDown_R
    1.0f, // EyeIn_L
    1.0f, // EyeIn_R
    1.0f, // EyeOpen_L
    1.0f, // EyeOpen_R
    1.0f, // EyeOut_L
    1.0f, // EyeOut_R
    1.0f, // EyeUp_L
    1.0f, // EyeUp_R
    3.0f, // BrowsD_L
    3.0f, // BrowsD_R
    3.0f, // BrowsU_C
    3.0f, // BrowsU_L
    3.0f, // BrowsU_R
    1.0f, // JawFwd
    2.0f, // JawLeft
    1.8f, // JawOpen
    1.0f, // JawChew
    2.0f, // JawRight
    1.5f, // MouthLeft
    1.5f, // MouthRight
    1.5f, // MouthFrown_L
    1.5f, // MouthFrown_R
    2.5f, // MouthSmile_L
    2.5f, // MouthSmile_R
    1.0f, // MouthDimple_L
    1.0f, // MouthDimple_R
    1.0f, // LipsStretch_L
    1.0f, // LipsStretch_R
    1.0f, // LipsUpperClose
    1.0f, // LipsLowerClose
    1.0f, // LipsUpperUp
    1.0f, // LipsLowerDown
    1.0f, // LipsUpperOpen
    1.0f, // LipsLowerOpen
    1.5f, // LipsFunnel
    2.5f, // LipsPucker
    1.5f, // ChinLowerRaise
    1.5f, // ChinUpperRaise
    1.0f, // Sneer
    3.0f, // Puff
    1.0f, // CheekSquint_L
    1.0f  // CheekSquint_R
};

struct DDEPacket {
    //roughly in mm
    float focal_length[1];
    float translation[3];
    
    //quaternion
    float rotation[4];
    
    // The DDE coefficients, overall, range from -0.2 to 1.5 or so. However, individual coefficients typically vary much 
    // less than this.
    float expressions[NUM_EXPRESSIONS];
    
    //avatar id selected on the UI
    int avatar_id;
    
    //client name, arbitrary length
    char name[MAX_NAME_SIZE + 1];
};

static const float STARTING_DDE_MESSAGE_TIME = 0.033f;
static const float DEFAULT_DDE_EYE_CLOSING_THRESHOLD = 0.8f;
static const int CALIBRATION_SAMPLES = 150;

DdeFaceTracker::DdeFaceTracker() :
    DdeFaceTracker(QHostAddress::Any, DDE_SERVER_PORT, DDE_CONTROL_PORT)
{

}

DdeFaceTracker::DdeFaceTracker(const QHostAddress& host, quint16 serverPort, quint16 controlPort) :
    _ddeProcess(NULL),
    _ddeStopping(false),
    _host(host),
    _serverPort(serverPort),
    _controlPort(controlPort),
    _lastReceiveTimestamp(0),
    _reset(false),
    _leftBlinkIndex(0), // see http://support.faceshift.com/support/articles/35129-export-of-blendshapes
    _rightBlinkIndex(1),
    _leftEyeDownIndex(4),
    _rightEyeDownIndex(5),
    _leftEyeInIndex(6),
    _rightEyeInIndex(7),
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
    _lastMessageReceived(0),
    _averageMessageTime(STARTING_DDE_MESSAGE_TIME),
    _lastHeadTranslation(glm::vec3(0.0f)),
    _filteredHeadTranslation(glm::vec3(0.0f)),
    _lastBrowUp(0.0f),
    _filteredBrowUp(0.0f),
    _eyePitch(0.0f),
    _eyeYaw(0.0f),
    _lastEyePitch(0.0f),
    _lastEyeYaw(0.0f),
    _filteredEyePitch(0.0f),
    _filteredEyeYaw(0.0f),
    _longTermAverageEyePitch(0.0f),
    _longTermAverageEyeYaw(0.0f),
    _lastEyeBlinks(),
    _filteredEyeBlinks(),
    _lastEyeCoefficients(),
    _eyeClosingThreshold("ddeEyeClosingThreshold", DEFAULT_DDE_EYE_CLOSING_THRESHOLD),
    _isCalibrating(false),
    _calibrationCount(0),
    _calibrationValues(),
    _calibrationBillboard(NULL),
    _calibrationBillboardID(UNKNOWN_OVERLAY_ID),
    _calibrationMessage(QString()),
    _isCalibrated(false)
{
    _coefficients.resize(NUM_FACESHIFT_BLENDSHAPES);
    _blendshapeCoefficients.resize(NUM_FACESHIFT_BLENDSHAPES);
    _coefficientAverages.resize(NUM_FACESHIFT_BLENDSHAPES);
    _calibrationValues.resize(NUM_FACESHIFT_BLENDSHAPES);

    _eyeStates[0] = EYE_UNCONTROLLED;
    _eyeStates[1] = EYE_UNCONTROLLED;

    connect(&_udpSocket, SIGNAL(readyRead()), SLOT(readPendingDatagrams()));
    connect(&_udpSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(socketErrorOccurred(QAbstractSocket::SocketError)));
    connect(&_udpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), 
        SLOT(socketStateChanged(QAbstractSocket::SocketState)));
}

DdeFaceTracker::~DdeFaceTracker() {
    setEnabled(false);

    if (_isCalibrating) {
        cancelCalibration();
    }
}

void DdeFaceTracker::init() {
    FaceTracker::init();
    setEnabled(Menu::getInstance()->isOptionChecked(MenuOption::UseCamera) && !_isMuted);
    Menu::getInstance()->getActionForOption(MenuOption::CalibrateCamera)->setEnabled(!_isMuted);
}

void DdeFaceTracker::setEnabled(bool enabled) {
    if (!_isInitialized) {
        // Don't enable until have explicitly initialized
        return;
    }
#ifdef HAVE_DDE

    if (_isCalibrating) {
        cancelCalibration();
    }

    // isOpen() does not work as one might expect on QUdpSocket; don't test isOpen() before closing socket.
    _udpSocket.close();

    // Terminate any existing DDE process, perhaps left running after an Interface crash.
    // Do this even if !enabled in case user reset their settings after crash.
    const char* DDE_EXIT_COMMAND = "exit";
    _udpSocket.bind(_host, _serverPort);
    _udpSocket.writeDatagram(DDE_EXIT_COMMAND, DDE_SERVER_ADDR, _controlPort);

    if (enabled && !_ddeProcess) {
        _ddeStopping = false;
        qCDebug(interfaceapp) << "DDE Face Tracker: Starting";
        _ddeProcess = new QProcess(qApp);
        connect(_ddeProcess, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(processFinished(int, QProcess::ExitStatus)));
        _ddeProcess->start(QCoreApplication::applicationDirPath() + DDE_PROGRAM_PATH, DDE_ARGUMENTS);
    }
    
    if (!enabled && _ddeProcess) {
        _ddeStopping = true;
        qCDebug(interfaceapp) << "DDE Face Tracker: Stopping";
    }
#endif
}

void DdeFaceTracker::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (_ddeProcess) {
        if (_ddeStopping) {
            qCDebug(interfaceapp) << "DDE Face Tracker: Stopped";

        } else {
            qCWarning(interfaceapp) << "DDE Face Tracker: Stopped unexpectedly";
            Menu::getInstance()->setIsOptionChecked(MenuOption::NoFaceTracking, true);
        }
        _udpSocket.close();
        delete _ddeProcess;
        _ddeProcess = NULL;
    }
}

void DdeFaceTracker::reset() {
    if (_udpSocket.state() == QAbstractSocket::BoundState) {
        _reset = true;

        qCDebug(interfaceapp) << "DDE Face Tracker: Reset";

        const char* DDE_RESET_COMMAND = "reset";
        _udpSocket.writeDatagram(DDE_RESET_COMMAND, DDE_SERVER_ADDR, _controlPort);

        FaceTracker::reset();

        _reset = true;
    }
}

void DdeFaceTracker::update(float deltaTime) {
    if (!isActive()) {
        return;
    }
    FaceTracker::update(deltaTime);

    glm::vec3 headEulers = glm::degrees(glm::eulerAngles(_headRotation));
    _estimatedEyePitch = _eyePitch - headEulers.x;
    _estimatedEyeYaw = _eyeYaw - headEulers.y;
}

bool DdeFaceTracker::isActive() const {
    return (_ddeProcess != NULL);
}

bool DdeFaceTracker::isTracking() const {
    static const quint64 ACTIVE_TIMEOUT_USECS = 3000000; //3 secs
    return (usecTimestampNow() - _lastReceiveTimestamp < ACTIVE_TIMEOUT_USECS);
}

//private slots and methods
void DdeFaceTracker::socketErrorOccurred(QAbstractSocket::SocketError socketError) {
    qCWarning(interfaceapp) << "DDE Face Tracker: Socket error: " << _udpSocket.errorString();
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
    qCDebug(interfaceapp) << "DDE Face Tracker: Socket: " << state;
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
    _lastReceiveTimestamp = usecTimestampNow();

    if (buffer.size() > MIN_PACKET_SIZE) {
        if (!_isCalibrated) {
            calibrate();
        }

        bool isFiltering = Menu::getInstance()->isOptionChecked(MenuOption::VelocityFilter);

        DDEPacket packet;
        int bytesToCopy = glm::min((int)sizeof(packet), buffer.size());
        memset(&packet.name, '\n', MAX_NAME_SIZE + 1);
        memcpy(&packet, buffer.data(), bytesToCopy);
        
        glm::vec3 translation;
        memcpy(&translation, packet.translation, sizeof(packet.translation));
        glm::quat rotation;
        memcpy(&rotation, &packet.rotation, sizeof(packet.rotation));
        if (_reset || (_lastMessageReceived == 0)) {
            memcpy(&_referenceTranslation, &translation, sizeof(glm::vec3));
            memcpy(&_referenceRotation, &rotation, sizeof(glm::quat));
            _reset = false;
        }

        // Compute relative translation
        float LEAN_DAMPING_FACTOR = 75.0f;
        translation -= _referenceTranslation;
        translation /= LEAN_DAMPING_FACTOR;
        translation.x *= -1;
        if (isFiltering) {
            glm::vec3 linearVelocity = (translation - _lastHeadTranslation) / _averageMessageTime;
            const float LINEAR_VELOCITY_FILTER_STRENGTH = 0.3f;
            float velocityFilter = glm::clamp(1.0f - glm::length(linearVelocity) *
                LINEAR_VELOCITY_FILTER_STRENGTH, 0.0f, 1.0f);
            _filteredHeadTranslation = velocityFilter * _filteredHeadTranslation + (1.0f - velocityFilter) * translation;
            _lastHeadTranslation = translation;
            _headTranslation = _filteredHeadTranslation;
        } else {
            _headTranslation = translation;
        }

        // Compute relative rotation
        rotation = glm::inverse(_referenceRotation) * rotation;
        if (isFiltering) {
            glm::quat r = glm::normalize(rotation * glm::inverse(_headRotation));
            float theta = 2 * acos(r.w);
            glm::vec3 angularVelocity;
            if (theta > EPSILON) {
                float rMag = glm::length(glm::vec3(r.x, r.y, r.z));
                angularVelocity = theta / _averageMessageTime * glm::vec3(r.x, r.y, r.z) / rMag;
            } else {
                angularVelocity = glm::vec3(0, 0, 0);
            }
            const float ANGULAR_VELOCITY_FILTER_STRENGTH = 0.3f;
            _headRotation = safeMix(_headRotation, rotation, glm::clamp(glm::length(angularVelocity) *
                ANGULAR_VELOCITY_FILTER_STRENGTH, 0.0f, 1.0f));
        } else {
            _headRotation = rotation;
        }

        // Translate DDE coefficients to Faceshift compatible coefficients
        for (int i = 0; i < NUM_EXPRESSIONS; i++) {
            _coefficients[DDE_TO_FACESHIFT_MAPPING[i]] = packet.expressions[i];
        }

        // Calibration
        if (_isCalibrating) {
            addCalibrationDatum();
        }
        for (int i = 0; i < NUM_FACESHIFT_BLENDSHAPES; i++) {
            _coefficients[i] -= _coefficientAverages[i];
        }

        // Use BrowsU_C to control both brows' up and down
        float browUp = _coefficients[_browUpCenterIndex];
        if (isFiltering) {
            const float BROW_VELOCITY_FILTER_STRENGTH = 0.5f;
            float velocity = fabsf(browUp - _lastBrowUp) / _averageMessageTime;
            float velocityFilter = glm::clamp(velocity * BROW_VELOCITY_FILTER_STRENGTH, 0.0f, 1.0f);
            _filteredBrowUp = velocityFilter * browUp + (1.0f - velocityFilter) * _filteredBrowUp;
            _lastBrowUp = browUp;
            browUp = _filteredBrowUp;
            _coefficients[_browUpCenterIndex] = browUp;
        }
        _coefficients[_browUpLeftIndex] = browUp;
        _coefficients[_browUpRightIndex] = browUp;
        _coefficients[_browDownLeftIndex] = -browUp;
        _coefficients[_browDownRightIndex] = -browUp;

        // Offset jaw open coefficient
        static const float JAW_OPEN_THRESHOLD = 0.1f;
        _coefficients[_jawOpenIndex] = _coefficients[_jawOpenIndex] - JAW_OPEN_THRESHOLD;

        // Offset smile coefficients
        static const float SMILE_THRESHOLD = 0.5f;
        _coefficients[_mouthSmileLeftIndex] = _coefficients[_mouthSmileLeftIndex] - SMILE_THRESHOLD;
        _coefficients[_mouthSmileRightIndex] = _coefficients[_mouthSmileRightIndex] - SMILE_THRESHOLD;

        // Eye pitch and yaw
        // EyeDown coefficients work better over both +ve and -ve values than EyeUp values.
        // EyeIn coefficients work better over both +ve and -ve values than EyeOut values.
        // Pitch and yaw values are relative to the screen.
        const float EYE_PITCH_SCALE = -1500.0f;  // Sign, scale, and average to be similar to Faceshift values.
        _eyePitch = EYE_PITCH_SCALE * (_coefficients[_leftEyeDownIndex] + _coefficients[_rightEyeDownIndex]);
        const float EYE_YAW_SCALE = 2000.0f;  // Scale and average to be similar to Faceshift values.
        _eyeYaw = EYE_YAW_SCALE * (_coefficients[_leftEyeInIndex] + _coefficients[_rightEyeInIndex]);
        if (isFiltering) {
            const float EYE_VELOCITY_FILTER_STRENGTH = 0.005f;
            float pitchVelocity = fabsf(_eyePitch - _lastEyePitch) / _averageMessageTime;
            float pitchVelocityFilter = glm::clamp(pitchVelocity * EYE_VELOCITY_FILTER_STRENGTH, 0.0f, 1.0f);
            _filteredEyePitch = pitchVelocityFilter * _eyePitch + (1.0f - pitchVelocityFilter) * _filteredEyePitch;
            _lastEyePitch = _eyePitch;
            _eyePitch = _filteredEyePitch;
            float yawVelocity = fabsf(_eyeYaw - _lastEyeYaw) / _averageMessageTime;
            float yawVelocityFilter = glm::clamp(yawVelocity * EYE_VELOCITY_FILTER_STRENGTH, 0.0f, 1.0f);
            _filteredEyeYaw = yawVelocityFilter * _eyeYaw + (1.0f - yawVelocityFilter) * _filteredEyeYaw;
            _lastEyeYaw = _eyeYaw;
            _eyeYaw = _filteredEyeYaw;
        }

        // Velocity filter EyeBlink values
        const float DDE_EYEBLINK_SCALE = 3.0f;
        float eyeBlinks[] = { DDE_EYEBLINK_SCALE * _coefficients[_leftBlinkIndex],
                              DDE_EYEBLINK_SCALE * _coefficients[_rightBlinkIndex] };
        if (isFiltering) {
            const float BLINK_VELOCITY_FILTER_STRENGTH = 0.3f;
            for (int i = 0; i < 2; i++) {
                float velocity = fabsf(eyeBlinks[i] - _lastEyeBlinks[i]) / _averageMessageTime;
                float velocityFilter = glm::clamp(velocity * BLINK_VELOCITY_FILTER_STRENGTH, 0.0f, 1.0f);
                _filteredEyeBlinks[i] = velocityFilter * eyeBlinks[i] + (1.0f - velocityFilter) * _filteredEyeBlinks[i];
                _lastEyeBlinks[i] = eyeBlinks[i];
            }
        }

        // Finesse EyeBlink values
        float eyeCoefficients[2];
        if (Menu::getInstance()->isOptionChecked(MenuOption::BinaryEyelidControl)) {
            if (_eyeStates[0] == EYE_UNCONTROLLED) {
                _eyeStates[0] = EYE_OPEN;
                _eyeStates[1] = EYE_OPEN;
            }

            for (int i = 0; i < 2; i++) {
                // Scale EyeBlink values so that they can be used to control both EyeBlink and EyeOpen
                // -ve values control EyeOpen; +ve values control EyeBlink
                static const float EYE_CONTROL_THRESHOLD = 0.5f;  // Resting eye value
                eyeCoefficients[i] = (_filteredEyeBlinks[i] - EYE_CONTROL_THRESHOLD) / (1.0f - EYE_CONTROL_THRESHOLD);

                // Change to closing or opening states
                const float EYE_CONTROL_HYSTERISIS = 0.25f;
                float eyeClosingThreshold = getEyeClosingThreshold();
                float eyeOpeningThreshold = eyeClosingThreshold - EYE_CONTROL_HYSTERISIS;
                if ((_eyeStates[i] == EYE_OPEN || _eyeStates[i] == EYE_OPENING) && eyeCoefficients[i] > eyeClosingThreshold) {
                    _eyeStates[i] = EYE_CLOSING;
                } else if ((_eyeStates[i] == EYE_CLOSED || _eyeStates[i] == EYE_CLOSING)
                    && eyeCoefficients[i] < eyeOpeningThreshold) {
                    _eyeStates[i] = EYE_OPENING;
                }

                const float EYELID_MOVEMENT_RATE = 10.0f;  // units/second
                const float EYE_OPEN_SCALE = 0.2f;
                if (_eyeStates[i] == EYE_CLOSING) {
                    // Close eyelid until it's fully closed
                    float closingValue = _lastEyeCoefficients[i] + EYELID_MOVEMENT_RATE * _averageMessageTime;
                    if (closingValue >= 1.0f) {
                        _eyeStates[i] = EYE_CLOSED;
                        eyeCoefficients[i] = 1.0f;
                    } else {
                        eyeCoefficients[i] = closingValue;
                    }
                } else if (_eyeStates[i] == EYE_OPENING) {
                    // Open eyelid until it meets the current adjusted value
                    float openingValue = _lastEyeCoefficients[i] - EYELID_MOVEMENT_RATE * _averageMessageTime;
                    if (openingValue < eyeCoefficients[i] * EYE_OPEN_SCALE) {
                        _eyeStates[i] = EYE_OPEN;
                        eyeCoefficients[i] = eyeCoefficients[i] * EYE_OPEN_SCALE;
                    } else {
                        eyeCoefficients[i] = openingValue;
                    }
                } else  if (_eyeStates[i] == EYE_OPEN) {
                    // Reduce eyelid movement
                    eyeCoefficients[i] = eyeCoefficients[i] * EYE_OPEN_SCALE;
                } else if (_eyeStates[i] == EYE_CLOSED) {
                    // Keep eyelid fully closed
                    eyeCoefficients[i] = 1.0;
                }
            }

            if (_eyeStates[0] == EYE_OPEN && _eyeStates[1] == EYE_OPEN) {
                // Couple eyelids
                eyeCoefficients[0] = eyeCoefficients[1] = (eyeCoefficients[0] + eyeCoefficients[0]) / 2.0f;
            }

            _lastEyeCoefficients[0] = eyeCoefficients[0];
            _lastEyeCoefficients[1] = eyeCoefficients[1];
        } else {
            _eyeStates[0] = EYE_UNCONTROLLED;
            _eyeStates[1] = EYE_UNCONTROLLED;

            eyeCoefficients[0] = _filteredEyeBlinks[0];
            eyeCoefficients[1] = _filteredEyeBlinks[1];
        }

        // Couple eyelid values if configured - use the most "open" value for both
        if (Menu::getInstance()->isOptionChecked(MenuOption::CoupleEyelids)) {
            float eyeCoefficient = std::min(eyeCoefficients[0], eyeCoefficients[1]);
            eyeCoefficients[0] = eyeCoefficient;
            eyeCoefficients[1] = eyeCoefficient;
        }

        // Use EyeBlink values to control both EyeBlink and EyeOpen
        if (eyeCoefficients[0] > 0) {
            _coefficients[_leftBlinkIndex] = eyeCoefficients[0];
            _coefficients[_leftEyeOpenIndex] = 0.0f;
        } else {
            _coefficients[_leftBlinkIndex] = 0.0f;
            _coefficients[_leftEyeOpenIndex] = -eyeCoefficients[0];
        }
        if (eyeCoefficients[1] > 0) {
            _coefficients[_rightBlinkIndex] = eyeCoefficients[1];
            _coefficients[_rightEyeOpenIndex] = 0.0f;
        } else {
            _coefficients[_rightBlinkIndex] = 0.0f;
            _coefficients[_rightEyeOpenIndex] = -eyeCoefficients[1];
        }

        // Scale all coefficients
        for (int i = 0; i < NUM_EXPRESSIONS; i++) {
            _blendshapeCoefficients[i]
                = glm::clamp(DDE_COEFFICIENT_SCALES[i] * _coefficients[i], 0.0f, 1.0f);
        }

        // Calculate average frame time
        const float FRAME_AVERAGING_FACTOR = 0.99f;
        quint64 usecsNow = usecTimestampNow();
        if (_lastMessageReceived != 0) {
            _averageMessageTime = FRAME_AVERAGING_FACTOR * _averageMessageTime 
                + (1.0f - FRAME_AVERAGING_FACTOR) * (float)(usecsNow - _lastMessageReceived) / 1000000.0f;
        }
        _lastMessageReceived = usecsNow;

        FaceTracker::countFrame();
        
    } else {
        qCWarning(interfaceapp) << "DDE Face Tracker: Decode error";
    }

    if (_isCalibrating && _calibrationCount > CALIBRATION_SAMPLES) {
        finishCalibration();
    }
}

void DdeFaceTracker::setEyeClosingThreshold(float eyeClosingThreshold) {
    _eyeClosingThreshold.set(eyeClosingThreshold);
}

static const int CALIBRATION_BILLBOARD_WIDTH = 300;
static const int CALIBRATION_BILLBOARD_HEIGHT = 120;
static const int CALIBRATION_BILLBOARD_TOP_MARGIN = 30;
static const int CALIBRATION_BILLBOARD_LEFT_MARGIN = 30;
static const int CALIBRATION_BILLBOARD_FONT_SIZE = 16;
static const float CALIBRATION_BILLBOARD_ALPHA = 0.5f;
static QString CALIBRATION_INSTRUCTION_MESSAGE = "Hold still to calibrate camera";

void DdeFaceTracker::calibrate() {
    if (!Menu::getInstance()->isOptionChecked(MenuOption::UseCamera) || _isMuted) {
        return;
    }

    if (!_isCalibrating) {
        qCDebug(interfaceapp) << "DDE Face Tracker: Calibration started";

        _isCalibrating = true;
        _calibrationCount = 0;
        _calibrationMessage = CALIBRATION_INSTRUCTION_MESSAGE + "\n\n";

        _calibrationBillboard = new TextOverlay();
        _calibrationBillboard->setTopMargin(CALIBRATION_BILLBOARD_TOP_MARGIN);
        _calibrationBillboard->setLeftMargin(CALIBRATION_BILLBOARD_LEFT_MARGIN);
        _calibrationBillboard->setFontSize(CALIBRATION_BILLBOARD_FONT_SIZE);
        _calibrationBillboard->setText(CALIBRATION_INSTRUCTION_MESSAGE);
        _calibrationBillboard->setAlpha(CALIBRATION_BILLBOARD_ALPHA);
        glm::vec2 viewport = qApp->getCanvasSize();
        _calibrationBillboard->setX((viewport.x - CALIBRATION_BILLBOARD_WIDTH) / 2);
        _calibrationBillboard->setY((viewport.y - CALIBRATION_BILLBOARD_HEIGHT) / 2);
        _calibrationBillboard->setWidth(CALIBRATION_BILLBOARD_WIDTH);
        _calibrationBillboard->setHeight(CALIBRATION_BILLBOARD_HEIGHT);
        _calibrationBillboardID = qApp->getOverlays().addOverlay(_calibrationBillboard);

        for (int i = 0; i < NUM_FACESHIFT_BLENDSHAPES; i++) {
            _calibrationValues[i] = 0.0f;
        }
    }
}

void DdeFaceTracker::addCalibrationDatum() {
    const int LARGE_TICK_INTERVAL = 30;
    const int SMALL_TICK_INTERVAL = 6;
    int samplesLeft = CALIBRATION_SAMPLES - _calibrationCount;
    if (samplesLeft % LARGE_TICK_INTERVAL == 0) {
        _calibrationMessage += QString::number(samplesLeft / LARGE_TICK_INTERVAL);
        _calibrationBillboard->setText(_calibrationMessage);
    } else if (samplesLeft % SMALL_TICK_INTERVAL == 0) {
        _calibrationMessage += ".";
        _calibrationBillboard->setText(_calibrationMessage);
    }

    for (int i = 0; i < NUM_FACESHIFT_BLENDSHAPES; i++) {
        _calibrationValues[i] += _coefficients[i];
    }

    _calibrationCount += 1;
}

void DdeFaceTracker::cancelCalibration() {
    qApp->getOverlays().deleteOverlay(_calibrationBillboardID);
    _calibrationBillboard = NULL;
    _isCalibrating = false;
    qCDebug(interfaceapp) << "DDE Face Tracker: Calibration cancelled";
}

void DdeFaceTracker::finishCalibration() {
    qApp->getOverlays().deleteOverlay(_calibrationBillboardID);
    _calibrationBillboard = NULL;
    _isCalibrating = false;
    _isCalibrated = true;

    for (int i = 0; i < NUM_FACESHIFT_BLENDSHAPES; i++) {
        _coefficientAverages[i] = _calibrationValues[i] / (float)CALIBRATION_SAMPLES;
    }

    reset();

    qCDebug(interfaceapp) << "DDE Face Tracker: Calibration finished";
}
