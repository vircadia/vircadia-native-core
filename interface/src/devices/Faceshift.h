//
//  Faceshift.h
//  interface
//
//  Created by Andrzej Kapolka on 9/3/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Faceshift__
#define __interface__Faceshift__

#include <vector>

#include <QTcpSocket>
#include <QUdpSocket>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <fsbinarystream.h>

/// Handles interaction with the Faceshift software, which provides head position/orientation and facial features.
class Faceshift : public QObject {
    Q_OBJECT

public:

    Faceshift();

    bool isActive() const;

    const glm::quat& getHeadRotation() const { return _headRotation; }
    const glm::vec3& getHeadAngularVelocity() const { return _headAngularVelocity; }
    const glm::vec3& getHeadTranslation() const { return _headTranslation; }

    float getEyeGazeLeftPitch() const { return _eyeGazeLeftPitch; }
    float getEyeGazeLeftYaw() const { return _eyeGazeLeftYaw; }
    
    float getEyeGazeRightPitch() const { return _eyeGazeRightPitch; }
    float getEyeGazeRightYaw() const { return _eyeGazeRightYaw; }

    float getEstimatedEyePitch() const { return _estimatedEyePitch; }
    float getEstimatedEyeYaw() const { return _estimatedEyeYaw; }

    const std::vector<float>& getBlendshapeCoefficients() const { return _blendshapeCoefficients; }

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
    uint64_t _lastTrackingStateReceived;
    
    glm::quat _headRotation;
    glm::vec3 _headAngularVelocity;
    glm::vec3 _headTranslation;
    
    float _eyeGazeLeftPitch;
    float _eyeGazeLeftYaw;
    
    float _eyeGazeRightPitch;
    float _eyeGazeRightYaw;
    
    std::vector<float> _blendshapeCoefficients;
    
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
    
    float _longTermAverageEyePitch;
    float _longTermAverageEyeYaw;
    bool _longTermAverageInitialized;
    
    float _estimatedEyePitch;
    float _estimatedEyeYaw;
};

#endif /* defined(__interface__Faceshift__) */
