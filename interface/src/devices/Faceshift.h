//
//  Faceshift.h
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 9/3/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Faceshift_h
#define hifi_Faceshift_h

#include <QTcpSocket>
#include <QUdpSocket>

#ifdef HAVE_FACESHIFT

#include <fsbinarystream.h>

#endif

#include "FaceTracker.h"

/// Handles interaction with the Faceshift software, which provides head position/orientation and facial features.
class Faceshift : public FaceTracker {
    Q_OBJECT

public:
    Faceshift();

    void init();

    bool isConnectedOrConnecting() const; 

    bool isActive() const;
    
#ifdef HAVE_FACESHIFT

    const glm::vec3& getHeadAngularVelocity() const { return _headAngularVelocity; }

    // these pitch/yaw angles are in degrees
    float getEyeGazeLeftPitch() const { return _eyeGazeLeftPitch; }
    float getEyeGazeLeftYaw() const { return _eyeGazeLeftYaw; }
    
    float getEyeGazeRightPitch() const { return _eyeGazeRightPitch; }
    float getEyeGazeRightYaw() const { return _eyeGazeRightYaw; }

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

    void update();
    void reset();
    
    void updateFakeCoefficients(float leftBlink, float rightBlink, float browUp,
        float jawOpen, QVector<float>& coefficients) const;
    
signals:

    void connectionStateChanged();

public slots:
    
    void setTCPEnabled(bool enabled);
    
private slots:

    void connectSocket();
    void noteConnected();
    void noteError(QAbstractSocket::SocketError error);
    void readPendingDatagrams();
    void readFromSocket();        
    
private:
    
    float getBlendshapeCoefficient(int index) const;
    
    void send(const std::string& message);
    void receive(const QByteArray& buffer);
    
    QTcpSocket _tcpSocket;
    QUdpSocket _udpSocket;
    fs::fsBinaryStream _stream;
    bool _tcpEnabled;
    int _tcpRetryCount;
    bool _tracking;
    quint64 _lastTrackingStateReceived;
    
    glm::vec3 _headAngularVelocity;
    
    // degrees
    float _eyeGazeLeftPitch;
    float _eyeGazeLeftYaw;
    float _eyeGazeRightPitch;
    float _eyeGazeRightYaw;
    
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
    
    // degrees
    float _longTermAverageEyePitch;
    float _longTermAverageEyeYaw;
    bool _longTermAverageInitialized;
    
#endif // HAVE_FACESHIFT
};

#endif // hifi_Faceshift_h
