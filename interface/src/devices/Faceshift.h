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

    bool isActive() const { return _socket.state() == QAbstractSocket::ConnectedState; }

    const glm::quat& getHeadRotation() const { return _headRotation; }
    const glm::vec3& getHeadTranslation() const { return _headTranslation; }

    float getEyeGazeLeftPitch() const { return _eyeGazeLeftPitch; }
    float getEyeGazeLeftYaw() const { return _eyeGazeLeftYaw; }
    
    float getEyeGazeRightPitch() const { return _eyeGazeRightPitch; }
    float getEyeGazeRightYaw() const { return _eyeGazeRightYaw; }

    float getLeftBlink() const { return _leftBlink; }
    float getRightBlink() const { return _rightBlink; }

    float getBrowHeight() const { return _browHeight; }
    
    float getMouthSize() const { return _mouthSize; }

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
    
    glm::quat _headRotation;
    glm::vec3 _headTranslation;
    
    float _eyeGazeLeftPitch;
    float _eyeGazeLeftYaw;
    
    float _eyeGazeRightPitch;
    float _eyeGazeRightYaw;
    
    float _leftBlink;
    float _rightBlink;
    
    int _leftBlinkIndex;
    int _rightBlinkIndex;
    
    float _browHeight;
    
    int _browUpCenterIndex;
    
    float _mouthSize;
    
    int _jawOpenIndex;
    
    int _leftEyeUpIndex;
    int _leftEyeDownIndex;
    int _leftEyeInIndex;
    int _leftEyeOutIndex;
    
    int _rightEyeUpIndex;
    int _rightEyeDownIndex;
    int _rightEyeInIndex;
    int _rightEyeOutIndex;
};

#endif /* defined(__interface__Faceshift__) */
