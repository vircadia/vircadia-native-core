//
//  Faceshift.h
//  interface
//
//  Created by Andrzej Kapolka on 9/3/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Faceshift__
#define __interface__Faceshift__

#include <QTcpSocket>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <fsbinarystream.h>

/// Handles interaction with the Faceshift software, which provides head position/orientation and facial features.
class Faceshift : public QObject {
    Q_OBJECT

public:

    Faceshift();

    bool isActive() const { return _socket.state() == QAbstractSocket::ConnectedState && _tracking; }

    const glm::quat& getHeadRotation() const { return _headRotation; }
    const glm::vec3& getHeadTranslation() const { return _headTranslation; }

    float getEyeGazeLeftPitch() const { return _eyeGazeLeftPitch; }
    float getEyeGazeLeftYaw() const { return _eyeGazeLeftYaw; }
    
    float getEyeGazeRightPitch() const { return _eyeGazeRightPitch; }
    float getEyeGazeRightYaw() const { return _eyeGazeRightYaw; }

    float getEstimatedEyePitch() const { return _estimatedEyePitch; }
    float getEstimatedEyeYaw() const { return _estimatedEyeYaw; }

    float getLeftBlink() const { return _leftBlink; }
    float getRightBlink() const { return _rightBlink; }
    float getLeftEyeOpen() const { return _leftEyeOpen; }
    float getRightEyeOpen() const { return _rightEyeOpen; }

    float getBrowDownLeft() const { return _browDownLeft; }
    float getBrowDownRight() const { return _browDownRight; }
    float getBrowUpCenter() const { return _browUpCenter; }
    float getBrowUpLeft() const { return _browUpLeft; }
    float getBrowUpRight() const { return _browUpRight; }

    float getMouthSize() const { return _mouthSize; }
    float getMouthSmileLeft() const { return _mouthSmileLeft; }
    float getMouthSmileRight() const { return _mouthSmileRight; }

    void update();
    void reset();

public slots:
    
    void setEnabled(bool enabled);

private slots:

    void connectSocket();
    void noteConnected();
    void noteError(QAbstractSocket::SocketError error);
    void readFromSocket();        
    
private:
    
    void send(const std::string& message);
    
    QTcpSocket _socket;
    fs::fsBinaryStream _stream;
    bool _enabled;
    bool _tracking;
    
    glm::quat _headRotation;
    glm::vec3 _headTranslation;
    
    float _eyeGazeLeftPitch;
    float _eyeGazeLeftYaw;
    
    float _eyeGazeRightPitch;
    float _eyeGazeRightYaw;
    
    float _leftBlink;
    float _rightBlink;
    float _leftEyeOpen;
    float _rightEyeOpen;

    int _leftBlinkIndex;
    int _rightBlinkIndex;
    int _leftEyeOpenIndex;
    int _rightEyeOpenIndex;
    

    // Brows
    float _browDownLeft;
    float _browDownRight;
    float _browUpCenter;
    float _browUpLeft;
    float _browUpRight;

    int _browDownLeftIndex;
    int _browDownRightIndex;
    int _browUpCenterIndex;
    int _browUpLeftIndex;
    int _browUpRightIndex;
    
    float _mouthSize;
    
    float _mouthSmileLeft;
    float _mouthSmileRight;
    
    int _mouthSmileLeftIndex;
    int _mouthSmileRightIndex;
    
    int _jawOpenIndex;
    
    float _longTermAverageEyePitch;
    float _longTermAverageEyeYaw;
    
    float _estimatedEyePitch;
    float _estimatedEyeYaw;
};

#endif /* defined(__interface__Faceshift__) */
