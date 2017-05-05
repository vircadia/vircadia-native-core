//
//  DdeFaceTracker.h
//
//
//  Created by Clement on 8/2/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DdeFaceTracker_h
#define hifi_DdeFaceTracker_h

#include <QtCore/QtGlobal>

#if defined(Q_OS_WIN) || defined(Q_OS_OSX)
    #define HAVE_DDE
#endif

#include <QProcess>
#include <QUdpSocket>

#include <DependencyManager.h>
#include <ui/overlays/TextOverlay.h>

#include <trackers/FaceTracker.h>

class DdeFaceTracker : public FaceTracker, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    virtual void init() override;
    virtual void reset() override;
    virtual void update(float deltaTime) override;

    virtual bool isActive() const override;
    virtual bool isTracking() const override;

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

    float getEyeClosingThreshold() { return _eyeClosingThreshold.get(); }
    void setEyeClosingThreshold(float eyeClosingThreshold);

public slots:
    void setEnabled(bool enabled) override;
    void calibrate();

private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

    //sockets
    void socketErrorOccurred(QAbstractSocket::SocketError socketError);
    void readPendingDatagrams();
    void socketStateChanged(QAbstractSocket::SocketState socketState);

private:
    DdeFaceTracker();
    DdeFaceTracker(const QHostAddress& host, quint16 serverPort, quint16 controlPort);
    virtual ~DdeFaceTracker();

    QProcess* _ddeProcess;
    bool _ddeStopping;

    QHostAddress _host;
    quint16 _serverPort;
    quint16 _controlPort;

    float getBlendshapeCoefficient(int index) const;
    void decodePacket(const QByteArray& buffer);

    // sockets
    QUdpSocket _udpSocket;
    quint64 _lastReceiveTimestamp;

    bool _reset;
    glm::vec3 _referenceTranslation;
    glm::quat _referenceRotation;

    int _leftBlinkIndex;
    int _rightBlinkIndex;
    int _leftEyeDownIndex;
    int _rightEyeDownIndex;
    int _leftEyeInIndex;
    int _rightEyeInIndex;
    int _leftEyeOpenIndex;
    int _rightEyeOpenIndex;

    int _browDownLeftIndex;
    int _browDownRightIndex;
    int _browUpCenterIndex;
    int _browUpLeftIndex;
    int _browUpRightIndex;

    int _mouthSmileLeftIndex;
    int _mouthSmileRightIndex;

    int _jawOpenIndex;

    QVector<float> _coefficients;

    quint64 _lastMessageReceived;
    float _averageMessageTime;

    glm::vec3 _lastHeadTranslation;
    glm::vec3 _filteredHeadTranslation;

    float _lastBrowUp;
    float _filteredBrowUp;

    float _eyePitch;  // Degrees, relative to screen
    float _eyeYaw;
    float _lastEyePitch;
    float _lastEyeYaw;
    float _filteredEyePitch;
    float _filteredEyeYaw;
    float _longTermAverageEyePitch = 0.0f;
    float _longTermAverageEyeYaw = 0.0f;
    bool _longTermAverageInitialized = false;

    enum EyeState {
        EYE_UNCONTROLLED,
        EYE_OPEN,
        EYE_CLOSING,
        EYE_CLOSED,
        EYE_OPENING
    };
    EyeState _eyeStates[2];
    float _lastEyeBlinks[2];
    float _filteredEyeBlinks[2];
    float _lastEyeCoefficients[2];
    Setting::Handle<float> _eyeClosingThreshold;

    QVector<float> _coefficientAverages;

    bool _isCalibrating;
    int _calibrationCount;
    QVector<float> _calibrationValues;
    TextOverlay* _calibrationBillboard;
    OverlayID _calibrationBillboardID;
    QString _calibrationMessage;
    bool _isCalibrated;
    void addCalibrationDatum();
    void cancelCalibration();
    void finishCalibration();
};

#endif // hifi_DdeFaceTracker_h
