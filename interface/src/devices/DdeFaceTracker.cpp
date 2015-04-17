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

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QElapsedTimer>

#include <GLMHelpers.h>

#include "DdeFaceTracker.h"
#include "FaceshiftConstants.h"
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
    4.0f, // EyeBlink_L
    4.0f, // EyeBlink_R
    1.0f, // EyeSquint_L
    1.0f, // EyeSquint_R
    1.0f, // EyeDown_L
    1.0f, // EyeDown_R
    1.0f, // EyeIn_L
    1.0f, // EyeIn_R
    4.0f, // EyeOpen_L
    4.0f, // EyeOpen_R
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
    1.5f, // JawLeft
    1.8f, // JawOpen
    1.0f, // JawChew
    1.5f, // JawRight
    1.5f, // MouthLeft
    1.5f, // MouthRight
    1.5f, // MouthFrown_L
    1.5f, // MouthFrown_R
    1.5f, // MouthSmile_L
    1.5f, // MouthSmile_R
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
    2.5f, // LipsFunnel
    2.0f, // LipsPucker
    1.5f, // ChinLowerRaise
    1.5f, // ChinUpperRaise
    1.0f, // Sneer
    3.0f, // Puff
    1.0f, // CheekSquint_L
    1.0f  // CheekSquint_R
};

struct Packet {
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

const float STARTING_DDE_MESSAGE_TIME = 0.033f;

DdeFaceTracker::DdeFaceTracker() :
    DdeFaceTracker(QHostAddress::Any, DDE_SERVER_PORT, DDE_CONTROL_PORT)
{

}

DdeFaceTracker::DdeFaceTracker(const QHostAddress& host, quint16 serverPort, quint16 controlPort) :
    _ddeProcess(NULL),
    _host(host),
    _serverPort(serverPort),
    _controlPort(controlPort),
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
    _lastMessageReceived(0),
    _averageMessageTime(STARTING_DDE_MESSAGE_TIME),
    _lastHeadTranslation(glm::vec3(0.0f)),
    _filteredHeadTranslation(glm::vec3(0.0f)),
    _lastLeftEyeBlink(0.0f),
    _filteredLeftEyeBlink(0.0f),
    _lastRightEyeBlink(0.0f),
    _filteredRightEyeBlink(0.0f)
{
    _coefficients.resize(NUM_FACESHIFT_BLENDSHAPES);

    _blendshapeCoefficients.resize(NUM_FACESHIFT_BLENDSHAPES);
    
    connect(&_udpSocket, SIGNAL(readyRead()), SLOT(readPendingDatagrams()));
    connect(&_udpSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(socketErrorOccurred(QAbstractSocket::SocketError)));
    connect(&_udpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), SLOT(socketStateChanged(QAbstractSocket::SocketState)));
}

DdeFaceTracker::~DdeFaceTracker() {
    setEnabled(false);
}

void DdeFaceTracker::setEnabled(bool enabled) {
#ifdef HAVE_DDE
    // isOpen() does not work as one might expect on QUdpSocket; don't test isOpen() before closing socket.
    _udpSocket.close();
    if (enabled) {
        _udpSocket.bind(_host, _serverPort);
    }

    const char* DDE_EXIT_COMMAND = "exit";

    if (enabled && !_ddeProcess) {
        // Terminate any existing DDE process, perhaps left running after an Interface crash
        _udpSocket.writeDatagram(DDE_EXIT_COMMAND, DDE_SERVER_ADDR, _controlPort);

        qDebug() << "[Info] DDE Face Tracker Starting";
        _ddeProcess = new QProcess(qApp);
        connect(_ddeProcess, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(processFinished(int, QProcess::ExitStatus)));
        _ddeProcess->start(QCoreApplication::applicationDirPath() + DDE_PROGRAM_PATH, DDE_ARGUMENTS);
    }

    if (!enabled && _ddeProcess) {
        _udpSocket.writeDatagram(DDE_EXIT_COMMAND, DDE_SERVER_ADDR, _controlPort);
        delete _ddeProcess;
        _ddeProcess = NULL;
        qDebug() << "[Info] DDE Face Tracker Stopped";
    }
#endif
}

void DdeFaceTracker::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (_ddeProcess) {
        // DDE crashed or was manually terminated
        qDebug() << "[Info] DDE Face Tracker Stopped Unexpectedly";
        _udpSocket.close();
        _ddeProcess = NULL;
        Menu::getInstance()->setIsOptionChecked(MenuOption::NoFaceTracking, true);
    }
}

void DdeFaceTracker::resetTracking() {
    qDebug() << "[Info] Reset DDE Tracking";
    const char* DDE_RESET_COMMAND = "reset";
    _udpSocket.writeDatagram(DDE_RESET_COMMAND, DDE_SERVER_ADDR, _controlPort);
}

bool DdeFaceTracker::isActive() const {
    static const quint64 ACTIVE_TIMEOUT_USECS = 3000000; //3 secs
    return (usecTimestampNow() - _lastReceiveTimestamp < ACTIVE_TIMEOUT_USECS);
}

//private slots and methods
void DdeFaceTracker::socketErrorOccurred(QAbstractSocket::SocketError socketError) {
    qCDebug(interfaceapp) << "[Error] DDE Face Tracker Socket Error: " << _udpSocket.errorString();
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
    qCDebug(interfaceapp) << "[Info] DDE Face Tracker Socket: " << state;
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
        bool isFiltering = Menu::getInstance()->isOptionChecked(MenuOption::DDEFiltering);

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
            glm::quat r = rotation * glm::inverse(_headRotation);
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
        for (int i = 0; i < NUM_EXPRESSIONS; i += 1) {
            _coefficients[DDE_TO_FACESHIFT_MAPPING[i]] = packet.expressions[i];
        }

        // Use EyeBlink values to control both EyeBlink and EyeOpen
        static const float RELAXED_EYE_VALUE = 0.1f;
        float leftEye = _coefficients[_leftBlinkIndex];
        float rightEye = _coefficients[_rightBlinkIndex];
        if (isFiltering) {
            const float BLINK_VELOCITY_FILTER_STRENGTH = 0.3f;

            float velocity = fabs(leftEye - _lastLeftEyeBlink) / _averageMessageTime;
            float velocityFilter = glm::clamp(velocity * BLINK_VELOCITY_FILTER_STRENGTH, 0.0f, 1.0f);
            _filteredLeftEyeBlink = velocityFilter * leftEye + (1.0f - velocityFilter) * _filteredLeftEyeBlink;
            _lastLeftEyeBlink = leftEye;
            leftEye = _filteredLeftEyeBlink;

            velocity = fabs(rightEye - _lastRightEyeBlink) / _averageMessageTime;
            velocityFilter = glm::clamp(velocity * BLINK_VELOCITY_FILTER_STRENGTH, 0.0f, 1.0f);
            _filteredRightEyeBlink = velocityFilter * rightEye + (1.0f - velocityFilter) * _filteredRightEyeBlink;
            _lastRightEyeBlink = rightEye;
            rightEye = _filteredRightEyeBlink;
        }
        if (leftEye > RELAXED_EYE_VALUE) {
            _coefficients[_leftBlinkIndex] = leftEye - RELAXED_EYE_VALUE;
            _coefficients[_leftEyeOpenIndex] = 0.0f;
        } else {
            _coefficients[_leftBlinkIndex] = 0.0f;
            _coefficients[_leftEyeOpenIndex] = RELAXED_EYE_VALUE - leftEye;
        }
        if (rightEye > RELAXED_EYE_VALUE) {
            _coefficients[_rightBlinkIndex] = rightEye - RELAXED_EYE_VALUE;
            _coefficients[_rightEyeOpenIndex] = 0.0f;
        } else {
            _coefficients[_rightBlinkIndex] = 0.0f;
            _coefficients[_rightEyeOpenIndex] = RELAXED_EYE_VALUE - rightEye;
        }

        // Use BrowsU_C to control both brows' up and down
        _coefficients[_browDownLeftIndex] = -_coefficients[_browUpCenterIndex];
        _coefficients[_browDownRightIndex] = -_coefficients[_browUpCenterIndex];
        _coefficients[_browUpLeftIndex] = _coefficients[_browUpCenterIndex];
        _coefficients[_browUpRightIndex] = _coefficients[_browUpCenterIndex];

        // Offset jaw open coefficient
        static const float JAW_OPEN_THRESHOLD = 0.16f;
        _coefficients[_jawOpenIndex] = _coefficients[_jawOpenIndex] - JAW_OPEN_THRESHOLD;

        // Offset smile coefficients
        static const float SMILE_THRESHOLD = 0.18f;
        _coefficients[_mouthSmileLeftIndex] = _coefficients[_mouthSmileLeftIndex] - SMILE_THRESHOLD;
        _coefficients[_mouthSmileRightIndex] = _coefficients[_mouthSmileRightIndex] - SMILE_THRESHOLD;


        // Scale all coefficients
        for (int i = 0; i < NUM_EXPRESSIONS; i += 1) {
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
    
    } else {
        qCDebug(interfaceapp) << "[Error] DDE Face Tracker Decode Error";
    }
    _lastReceiveTimestamp = usecTimestampNow();
}
