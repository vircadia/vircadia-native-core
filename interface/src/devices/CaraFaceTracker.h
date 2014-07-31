//
//  CaraFaceTracker.h
//  interface/src/devices
//
//  Created by Li Zuwei on 7/22/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hi_fi_CaraFaceTracker_h
#define hi_fi_CaraFaceTracker_h

#include <QUdpSocket>

#include "FaceTracker.h"

/*!
 * \class   CaraFaceTracker
 *
 * \brief   Handles interaction with the Cara software, 
 *          which provides head position/orientation and facial features.
 * \details By default, opens a udp socket with IPV4_ANY_ADDR with port 36555.
 *          User needs to run the Cara Face Detection UDP Client with the destination
 *          host address (eg: 127.0.0.1 for localhost) and destination port 36555.
**/

class CaraFaceTracker : public FaceTracker {
    Q_OBJECT

public:
    CaraFaceTracker();
    CaraFaceTracker(const QHostAddress& host, quint16 port);
    ~CaraFaceTracker();

    //initialization
    void init();
    void reset();

    //sockets
    void bindTo(quint16 port);
    void bindTo(const QHostAddress& host, quint16 port);
    bool isActive() const;

    //tracking
    void update();

    //head angular velocity
    const glm::vec3& getHeadAngularVelocity() const { return _headAngularVelocity; }

    //eye gaze
    float getEyeGazeLeftPitch() const { return _eyeGazeLeftPitch; }
    float getEyeGazeLeftYaw() const { return _eyeGazeLeftYaw; }

    float getEyeGazeRightPitch() const { return _eyeGazeRightPitch; }
    float getEyeGazeRightYaw() const { return _eyeGazeRightYaw; }

    //blend shapes
    float getLeftBlink() const { return getBlendshapeCoefficient(_leftBlinkIndex); }
    float getRightBlink() const { return getBlendshapeCoefficient(_rightBlinkIndex); }
    float getLeftEyeOpen() const { return getBlendshapeCoefficient(_leftEyeOpenIndex); }
    float getRightEyeOpen() const { return getBlendshapeCoefficient(_rightEyeOpenIndex); }

    float getBrowDownLeft() const { return getBlendshapeCoefficient(_browDownLeftIndex); }
    float getBrowDownRight() const { return getBlendshapeCoefficient(_browDownRightIndex); }
    float getBrowUpCenter() const { return getBlendshapeCoefficient(_browUpCenterIndex); }
    float getBrowUpLeft() const { return getBlendshapeCoefficient(_browUpLeftIndex); }
    float getBrowUpRight() const { return getBlendshapeCoefficient(_browUpRightIndex); }

    float getMouthSize() const { return getBlendshapeCoefficient(_jawOpenIndex); }
    float getMouthSmileLeft() const { return getBlendshapeCoefficient(_mouthSmileLeftIndex); }
    float getMouthSmileRight() const { return getBlendshapeCoefficient(_mouthSmileRightIndex); }

private slots:

    //sockets
    void socketErrorOccurred(QAbstractSocket::SocketError socketError);
    void readPendingDatagrams();
    void socketStateChanged(QAbstractSocket::SocketState socketState);
    
private:
    void decodePacket(const QByteArray& buffer);
    float getBlendshapeCoefficient(int index) const;

    // sockets
    QUdpSocket _udpSocket;
    quint64 _lastReceiveTimestamp;

    //head tracking
    glm::vec3 _headAngularVelocity;

    //pose history
    float _previousPitch;
    float _previousYaw;
    float _previousRoll;

    // eye gaze degrees
    float _eyeGazeLeftPitch;
    float _eyeGazeLeftYaw;
    float _eyeGazeRightPitch;
    float _eyeGazeRightYaw;

    //blend shapes
    int _leftBlinkIndex;
    int _rightBlinkIndex;
    int _leftEyeOpenIndex;
    int _rightEyeOpenIndex;

    // Brows
    int _browDownLeftIndex;
    int _browDownRightIndex;
    int _browUpCenterIndex;
    int _browUpLeftIndex;
    int _browUpRightIndex;

    int _mouthSmileLeftIndex;
    int _mouthSmileRightIndex;

    int _jawOpenIndex;
};

#endif //endif hi_fi_CaraFaceTracker_h