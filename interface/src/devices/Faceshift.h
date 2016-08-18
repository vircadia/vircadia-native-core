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

#include <DependencyManager.h>
#include <SettingHandle.h>

#include "FaceTracker.h"

const float STARTING_FACESHIFT_FRAME_TIME = 0.033f;

/// Handles interaction with the Faceshift software, which provides head position/orientation and facial features.
class Faceshift : public FaceTracker, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
#ifdef HAVE_FACESHIFT
    // If we don't have faceshift, use the base class' methods
    virtual void init();
    virtual void update(float deltaTime);
    virtual void reset();

    virtual bool isActive() const;
    virtual bool isTracking() const;
#endif

    bool isConnectedOrConnecting() const;

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

    QString getHostname() { return _hostname.get(); }
    void setHostname(const QString& hostname);

    void updateFakeCoefficients(float leftBlink,
                                float rightBlink,
                                float browUp,
                                float jawOpen,
                                float mouth2,
                                float mouth3,
                                float mouth4,
                                QVector<float>& coefficients) const;

signals:
    void connectionStateChanged();

public slots:
    void setEnabled(bool enabled) override;

private slots:
    void connectSocket();
    void noteConnected();
    void noteError(QAbstractSocket::SocketError error);
    void readPendingDatagrams();
    void readFromSocket();
    void noteDisconnected();

private:
    Faceshift();
    virtual ~Faceshift() {}

    void send(const std::string& message);
    void receive(const QByteArray& buffer);

    QTcpSocket _tcpSocket;
    QUdpSocket _udpSocket;

#ifdef HAVE_FACESHIFT
    fs::fsBinaryStream _stream;
#endif

    bool _tcpEnabled = true;
    int _tcpRetryCount = 0;
    bool _tracking = false;
    quint64 _lastReceiveTimestamp = 0;
    quint64 _lastMessageReceived = 0;
    float _averageFrameTime = STARTING_FACESHIFT_FRAME_TIME;

    glm::vec3 _headAngularVelocity = glm::vec3(0.0f);
    glm::vec3 _headLinearVelocity = glm::vec3(0.0f);
    glm::vec3 _lastHeadTranslation = glm::vec3(0.0f);
    glm::vec3 _filteredHeadTranslation = glm::vec3(0.0f);

    // degrees
    float _eyeGazeLeftPitch = 0.0f;
    float _eyeGazeLeftYaw = 0.0f;
    float _eyeGazeRightPitch = 0.0f;
    float _eyeGazeRightYaw = 0.0f;

    // degrees
    float _longTermAverageEyePitch = 0.0f;
    float _longTermAverageEyeYaw = 0.0f;
    bool _longTermAverageInitialized = false;

    Setting::Handle<QString> _hostname;

    // see http://support.faceshift.com/support/articles/35129-export-of-blendshapes
    int _leftBlinkIndex = 0;
    int _rightBlinkIndex = 1;
    int _leftEyeOpenIndex = 8;
    int _rightEyeOpenIndex = 9;

    // Brows
    int _browDownLeftIndex = 14;
    int _browDownRightIndex = 15;
    int _browUpCenterIndex = 16;
    int _browUpLeftIndex = 17;
    int _browUpRightIndex = 18;

    int _mouthSmileLeftIndex = 28;
    int _mouthSmileRightIndex = 29;

    int _jawOpenIndex = 21;
};

#endif // hifi_Faceshift_h
