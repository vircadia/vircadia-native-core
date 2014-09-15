//
//  CaraFaceTracker.cpp
//  interface/src/devices
//
//  Created by Li Zuwei on 7/22/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CaraFaceTracker.h"
#include <GLMHelpers.h>

//qt
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QElapsedTimer>

#define PI M_PI
#define RADTODEG(x) ( (x) * 180.0 / PI )
#define DEGTORAD(x) ( (x) * PI / 180.0 )

static const QHostAddress CARA_FEATURE_POINT_SERVER_ADDR("127.0.0.1");
static const quint16 CARA_FEATURE_POINT_SERVER_PORT = 36555;
static QString sampleJson = "[{\"id\":1, \
    \"face\":{\"x\":248,\"y\":64,\"width\":278,\"height\":341}, \
    \"pose\":{\"roll\":2.62934,\"pitch\":-12.2318,\"yaw\":0.936743}, \
    \"feature_points\":[314,194,326,187,340,187,354,189,367,193,409,190,421,187,435,184,448,183,459,188, \
    388,207,389,223,390,240,391,257,377,266,384,267,392,268,399,266,407,264,331,209, \
    341,204,354,204,364,209,353,214,341,214,410,208,420,201,433,200,443,205,434,211, \
    421,211,362,294,372,290,383,287,393,289,404,286,415,289,426,291,418,300,407,306, \
    394,308,382,307,371,302,383,295,394,295,404,294,404,295,393,297,383,296], \
    \"classifiers\":{\"emotion\":{\"smi\":-0.368829,\"sur\":-1.33334,\"neg\":0.00235828,\"att\":1},\"blink\":1}}]";

static const glm::vec3 DEFAULT_HEAD_ORIGIN(0.0f, 0.0f, 0.0f);
static const float TRANSLATION_SCALE = 1.0f;
static const int NUM_BLENDSHAPE_COEFF = 30;
static const int NUM_SMOOTHING_SAMPLES = 3;

struct CaraPerson {
    struct CaraPose {
        float roll, pitch, yaw;
        CaraPose() :
            roll(0.0f),
            pitch(0.0f),
            yaw(0.0f) 
        {
        }
    };

    struct CaraEmotion {
        float smile, surprise, negative, attention;
        CaraEmotion(): 
            smile(0.0f),
            surprise(0.0f),
            negative(0.0f),
            attention(0.0f)
        {
        }
    };

    enum CaraBlink {
        BLINK_NOT_AVAILABLE,
        NO_BLINK,
        BLINK
    };

    CaraPerson() :
        id(-1),
        blink(BLINK_NOT_AVAILABLE) 
    {    
    }

    int id;
    CaraPose pose;
    CaraEmotion emotion;
    CaraBlink blink;

    QString toString() {
        QString s = QString("id: %1, roll: %2, pitch: %3, yaw: %4, smi: %5, sur: %6, neg: %7, att: %8, blink: %9").
            arg(id).
            arg(pose.roll).
            arg(pose.pitch).
            arg(pose.yaw).
            arg(emotion.smile).
            arg(emotion.surprise).
            arg(emotion.negative).
            arg(emotion.attention).
            arg(blink);
        return s;
    }
};

class CaraPacketDecoder {
public:
    static CaraPerson extractOne(const QByteArray& buffer, QJsonParseError* jsonError) {
        CaraPerson person;
        QJsonDocument dom = QJsonDocument::fromJson(buffer, jsonError);

        //check for errors
        if(jsonError->error == QJsonParseError::NoError) {
            //read the dom structure and populate the blend shapes and head poses
            //qDebug() << "[Info] Cara Face Tracker Packet Parsing Successful!";

            //begin extracting the packet
            if(dom.isArray()) {
                QJsonArray people = dom.array();                
                //extract the first person in the array
                if(people.size() > 0) { 
                    QJsonValue val = people.at(0);
                    if(val.isObject()) {
                        QJsonObject personDOM = val.toObject();
                        person.id = extractId(personDOM);
                        person.pose = extractPose(personDOM);

                        //extract the classifier outputs
                        QJsonObject::const_iterator it = personDOM.constFind("classifiers");
                        if(it != personDOM.constEnd()) {
                            QJsonObject classifierDOM = (*it).toObject();
                            person.emotion = extractEmotion(classifierDOM);
                            person.blink = extractBlink(classifierDOM);
                        }                    
                    }
                }
            }
        }

        return person;
    }

private:
    static int extractId(const QJsonObject& person) {
        int id = -1;
        QJsonObject::const_iterator it = person.constFind("id");
        if(it != person.constEnd()) {
            id = (*it).toInt(-1);
        }
        return id;
    }

    static CaraPerson::CaraPose extractPose(const QJsonObject& person) {
        CaraPerson::CaraPose pose;
        QJsonObject::const_iterator it = person.constFind("pose");
        if(it != person.constEnd()) {
            QJsonObject poseDOM = (*it).toObject();

            //look for the roll, pitch, yaw;
            QJsonObject::const_iterator poseIt = poseDOM.constFind("roll");
            QJsonObject::const_iterator poseEnd = poseDOM.constEnd();
            if(poseIt != poseEnd) {
                pose.roll = (float)(*poseIt).toDouble(0.0);
            }
            poseIt = poseDOM.constFind("pitch");
            if(poseIt != poseEnd) {
                pose.pitch = (float)(*poseIt).toDouble(0.0);
            }
            poseIt = poseDOM.constFind("yaw");
            if(poseIt != poseEnd) {
                pose.yaw = (float)(*poseIt).toDouble(0.0);
            }
        }
        return pose;
    }

    static CaraPerson::CaraEmotion extractEmotion(const QJsonObject& classifiers) {
        CaraPerson::CaraEmotion emotion;
        QJsonObject::const_iterator it = classifiers.constFind("emotion");
        if(it != classifiers.constEnd()) {
            QJsonObject emotionDOM = (*it).toObject();

            //look for smile, surprise, negative, attention responses
            QJsonObject::const_iterator emoEnd = emotionDOM.constEnd();
            QJsonObject::const_iterator emoIt = emotionDOM.constFind("smi");
            if(emoIt != emoEnd) {
                emotion.smile = (float)(*emoIt).toDouble(0.0);
            }
            emoIt = emotionDOM.constFind("sur");
            if(emoIt != emoEnd) {
                emotion.surprise = (float)(*emoIt).toDouble(0.0);
            }
            emoIt = emotionDOM.constFind("neg");
            if(emoIt != emoEnd) {
                emotion.negative = (float)(*emoIt).toDouble(0.0);
            }
            emoIt = emotionDOM.constFind("att");
            if(emoIt != emoEnd) {
                emotion.attention = (float)(*emoIt).toDouble(0.0);
            }
        }
        return emotion;
    }

    static CaraPerson::CaraBlink extractBlink(const QJsonObject& classifiers) {
        CaraPerson::CaraBlink blink = CaraPerson::BLINK_NOT_AVAILABLE;
        QJsonObject::const_iterator it = classifiers.constFind("blink");
        if(it != classifiers.constEnd()) {
            int b = (*it).toInt(CaraPerson::BLINK_NOT_AVAILABLE);
            switch(b) {
                case CaraPerson::BLINK_NOT_AVAILABLE: 
                    blink = CaraPerson::BLINK_NOT_AVAILABLE;
                    break;
                case CaraPerson::NO_BLINK:
                    blink = CaraPerson::NO_BLINK;
                    break;
                case CaraPerson::BLINK:
                    blink = CaraPerson::BLINK;
                    break;
                default:
                    blink = CaraPerson::BLINK_NOT_AVAILABLE;
                    break;
            }
        }
        return blink;
    }
};

CaraFaceTracker::CaraFaceTracker() :
    _lastReceiveTimestamp(0),
    _pitchAverage(NUM_SMOOTHING_SAMPLES),
    _yawAverage(NUM_SMOOTHING_SAMPLES),
    _rollAverage(NUM_SMOOTHING_SAMPLES),
    _eyeGazeLeftPitch(0.0f),
    _eyeGazeLeftYaw(0.0f),
    _eyeGazeRightPitch(0.0f),
    _eyeGazeRightYaw(0),
    _leftBlinkIndex(0),
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
    connect(&_udpSocket, SIGNAL(readyRead()), SLOT(readPendingDatagrams()));
    connect(&_udpSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(socketErrorOccurred(QAbstractSocket::SocketError)));
    connect(&_udpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), SLOT(socketStateChanged(QAbstractSocket::SocketState)));

    bindTo(CARA_FEATURE_POINT_SERVER_PORT);

    _headTranslation = DEFAULT_HEAD_ORIGIN;
    _blendshapeCoefficients.resize(NUM_BLENDSHAPE_COEFF);
    _blendshapeCoefficients.fill(0.0f);

    //qDebug() << sampleJson;
}

CaraFaceTracker::CaraFaceTracker(const QHostAddress& host, quint16 port) :
    _lastReceiveTimestamp(0),
    _pitchAverage(NUM_SMOOTHING_SAMPLES),
    _yawAverage(NUM_SMOOTHING_SAMPLES),
    _rollAverage(NUM_SMOOTHING_SAMPLES),
    _eyeGazeLeftPitch(0.0f),
    _eyeGazeLeftYaw(0.0f),
    _eyeGazeRightPitch(0.0f),
    _eyeGazeRightYaw(0.0f),
    _leftBlinkIndex(0),
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
    connect(&_udpSocket, SIGNAL(readyRead()), SLOT(readPendingDatagrams()));
    connect(&_udpSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(socketErrorOccurred(QAbstractSocket::SocketError)));
    connect(&_udpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), SIGNAL(socketStateChanged(QAbstractSocket::SocketState)));

    bindTo(host, port);

    _headTranslation = DEFAULT_HEAD_ORIGIN * TRANSLATION_SCALE;
    _blendshapeCoefficients.resize(NUM_BLENDSHAPE_COEFF); //set the size of the blendshape coefficients
    _blendshapeCoefficients.fill(0.0f);
}

CaraFaceTracker::~CaraFaceTracker() {
    if(_udpSocket.isOpen())
        _udpSocket.close();
}

void CaraFaceTracker::init() {

}

void CaraFaceTracker::reset() {

}

void CaraFaceTracker::bindTo(quint16 port) {
    bindTo(QHostAddress::Any, port);
}

void CaraFaceTracker::bindTo(const QHostAddress& host, quint16 port) {
    if(_udpSocket.isOpen()) {
        _udpSocket.close();
    }
    _udpSocket.bind(host, port);
}

bool CaraFaceTracker::isActive() const {
    static const quint64 ACTIVE_TIMEOUT_USECS = 3000000; //3 secs
    return (usecTimestampNow() - _lastReceiveTimestamp < ACTIVE_TIMEOUT_USECS);
}

void CaraFaceTracker::update() {
    // get the euler angles relative to the window
    glm::vec3 eulers = glm::degrees(safeEulerAngles(_headRotation * glm::quat(glm::radians(glm::vec3(
        (_eyeGazeLeftPitch + _eyeGazeRightPitch) / 2.0f, (_eyeGazeLeftYaw + _eyeGazeRightYaw) / 2.0f, 0.0f)))));

    //TODO: integrate when cara has eye gaze estimation

    _estimatedEyePitch = eulers.x;
    _estimatedEyeYaw = eulers.y;
}

//private slots and methods
void CaraFaceTracker::socketErrorOccurred(QAbstractSocket::SocketError socketError) {
    qDebug() << "[Error] Cara Face Tracker Socket Error: " << _udpSocket.errorString();
}

void CaraFaceTracker::socketStateChanged(QAbstractSocket::SocketState socketState) {
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
    qDebug() << "[Info] Cara Face Tracker Socket: " << socketState;
}

void CaraFaceTracker::readPendingDatagrams() {
    QByteArray buffer;
    while (_udpSocket.hasPendingDatagrams()) {
        buffer.resize(_udpSocket.pendingDatagramSize());
        _udpSocket.readDatagram(buffer.data(), buffer.size());
        decodePacket(buffer);
    }
}

void CaraFaceTracker::decodePacket(const QByteArray& buffer) {
    //decode the incoming udp packet
    QJsonParseError jsonError;
    CaraPerson person = CaraPacketDecoder::extractOne(buffer, &jsonError);

    if(jsonError.error == QJsonParseError::NoError) {

        //do some noise filtering to the head poses
        //reduce the noise first by truncating to 1 dp
        person.pose.roll = glm::floor(person.pose.roll * 10) / 10;
        person.pose.pitch = glm::floor(person.pose.pitch * 10) / 10;
        person.pose.yaw = glm::floor(person.pose.yaw * 10) / 10;

        //qDebug() << person.toString();

        glm::quat newRotation(glm::vec3(DEGTORAD(person.pose.pitch), DEGTORAD(person.pose.yaw), DEGTORAD(person.pose.roll)));

        // Compute angular velocity of the head
        glm::quat r = newRotation * glm::inverse(_headRotation);
        float theta = 2 * acos(r.w);
        if (theta > EPSILON) {
            float rMag = glm::length(glm::vec3(r.x, r.y, r.z));
            const float AVERAGE_CARA_FRAME_TIME = 0.04f;
            const float YAW_STANDARD_DEV_DEG = 2.5f;

            _headAngularVelocity = theta / AVERAGE_CARA_FRAME_TIME * glm::vec3(r.x, r.y, r.z) / rMag;
            _pitchAverage.updateAverage(person.pose.pitch);
            _rollAverage.updateAverage(person.pose.roll);

            //could use the angular velocity to detemine whether to update pitch and roll to further remove the noise.
            //use the angular velocity for roll and pitch, update if > THRESHOLD
            //if(glm::abs(_headAngularVelocity.x) > ANGULAR_VELOCITY_MIN) {           
            //   _pitchAverage.updateAverage(person.pose.pitch);
            //}

            //if(glm::abs(_headAngularVelocity.z) > ANGULAR_VELOCITY_MIN) {
            //   _rollAverage.updateAverage(person.pose.roll);;
            //}

            //for yaw, the jitter is great, you can't use angular velocity because it swings too much
            //use the previous and current yaw, calculate the 
            //abs difference and move it the difference is above the standard deviation which is around 2.5
            // > the standard deviation 2.5 deg, update the yaw smoothing average
            if(glm::abs(person.pose.yaw - _yawAverage.getAverage()) > YAW_STANDARD_DEV_DEG) {
                //qDebug() << "Yaw Diff: " << glm::abs(person.pose.yaw - _previousYaw);
                _yawAverage.updateAverage(person.pose.yaw);
            }

            //set the new rotation
            newRotation = glm::quat(glm::vec3(DEGTORAD(_pitchAverage.getAverage()), DEGTORAD(_yawAverage.getAverage()), DEGTORAD(-_rollAverage.getAverage())));
        } 
        else {
            //no change in position, use previous averages
            newRotation = glm::quat(glm::vec3(DEGTORAD(_pitchAverage.getAverage()), DEGTORAD(_yawAverage.getAverage()), DEGTORAD(-_rollAverage.getAverage())));
            _headAngularVelocity = glm::vec3(0,0,0);
        }

        //update to new rotation angles
        _headRotation = newRotation;       

        //TODO: head translation, right now is 0

        //Do Blendshapes, clip between 0.0f to 1.0f, neg should be ignored       
        _blendshapeCoefficients[_leftBlinkIndex] = person.blink == CaraPerson::BLINK ? 1.0f : 0.0f;
        _blendshapeCoefficients[_rightBlinkIndex] = person.blink == CaraPerson::BLINK ? 1.0f : 0.0f;

        //anger and surprised are mutually exclusive so we could try use this fact to determine
        //whether to down the brows or up the brows                      
        _blendshapeCoefficients[_browDownLeftIndex] = person.emotion.negative < 0.0f ? 0.0f : person.emotion.negative;
        _blendshapeCoefficients[_browDownRightIndex] = person.emotion.negative < 0.0f ? 0.0f : person.emotion.negative;
        _blendshapeCoefficients[_browUpCenterIndex] = person.emotion.surprise < 0.0f ? 0.0f : person.emotion.surprise;
        _blendshapeCoefficients[_browUpLeftIndex] = person.emotion.surprise < 0.0f ? 0.0f : person.emotion.surprise;
        _blendshapeCoefficients[_browUpRightIndex] = person.emotion.surprise < 0.0f ? 0.0f : person.emotion.surprise;
        _blendshapeCoefficients[_jawOpenIndex] = person.emotion.surprise < 0.0f ? 0.0f : person.emotion.surprise;
        _blendshapeCoefficients[_mouthSmileLeftIndex] = person.emotion.smile < 0.0f ? 0.0f : person.emotion.smile;
        _blendshapeCoefficients[_mouthSmileRightIndex] = person.emotion.smile < 0.0f ? 0.0f : person.emotion.smile;
    }
    else {
        qDebug() << "[Error] Cara Face Tracker Decode Error: " << jsonError.errorString();
    }

    _lastReceiveTimestamp = usecTimestampNow();    
}

float CaraFaceTracker::getBlendshapeCoefficient(int index) const {
    return (index >= 0 && index < (int)_blendshapeCoefficients.size()) ? _blendshapeCoefficients[index] : 0.0f;
}
