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

#include <QUdpSocket>

#include <DependencyManager.h>

#include "FaceTracker.h"

class DdeFaceTracker : public FaceTracker, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    
public:
    virtual void reset() { _reset = true; }

    virtual bool isActive() const;
    virtual bool isTracking() const { return isActive(); }
    
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

public slots:
    void setEnabled(bool enabled);

private slots:
    
    //sockets
    void socketErrorOccurred(QAbstractSocket::SocketError socketError);
    void readPendingDatagrams();
    void socketStateChanged(QAbstractSocket::SocketState socketState);
    
private:
    DdeFaceTracker();
    DdeFaceTracker(const QHostAddress& host, quint16 port);
    ~DdeFaceTracker();

    QHostAddress _host;
    quint16 _port;
    
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

    // Previous values for simple smoothing
    glm::vec3 _previousTranslation;
    glm::quat _previousRotation;
    QVector<float> _previousExpressions;
};

#endif // hifi_DdeFaceTracker_h