//
//  Camera.h
//  interface/src
//
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Camera_h
#define hifi_Camera_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <RegisteredMetaTypes.h>
#include <ViewFrustum.h>

enum CameraMode
{
    CAMERA_MODE_NULL = -1,
    CAMERA_MODE_THIRD_PERSON,
    CAMERA_MODE_FIRST_PERSON,
    CAMERA_MODE_MIRROR,
    CAMERA_MODE_INDEPENDENT,
    NUM_CAMERA_MODES
};

Q_DECLARE_METATYPE(CameraMode);
static int cameraModeId = qRegisterMetaType<CameraMode>();

class Camera : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(glm::vec3 position READ getPosition WRITE setPosition)
    Q_PROPERTY(glm::quat orientation READ getOrientation WRITE setOrientation)
    Q_PROPERTY(QString mode READ getModeString WRITE setModeString)
public:
    Camera();

    void initialize(); // instantly put the camera at the ideal position and rotation. 

    void update( float deltaTime );

    void setRotation(const glm::quat& rotation) { _rotation = rotation; };
    void setHmdPosition(const glm::vec3& hmdPosition) { _hmdPosition = hmdPosition; }
    void setHmdRotation(const glm::quat& hmdRotation) { _hmdRotation = hmdRotation; };
    
    void setMode(CameraMode m);
    void setFieldOfView(float f);
    void setAspectRatio(float a);
    void setNearClip(float n);
    void setFarClip(float f);
    void setEyeOffsetPosition(const glm::vec3& p) { _eyeOffsetPosition = p; }
    void setEyeOffsetOrientation(const glm::quat& o) { _eyeOffsetOrientation = o; }
    void setScale(const float s) { _scale = s; }
    
    glm::quat getRotation() const { return _rotation * _hmdRotation; }
    const glm::vec3& getHmdPosition() const { return _hmdPosition; }
    const glm::quat& getHmdRotation() const { return _hmdRotation; }
    
    CameraMode getMode() const { return _mode; }
    float getFieldOfView() const { return _fieldOfView; }
    float getAspectRatio() const { return _aspectRatio; }
    float getNearClip() const { return _scale * _nearClip; }
    float getFarClip() const;
    const glm::vec3& getEyeOffsetPosition() const { return _eyeOffsetPosition;   }
    const glm::quat& getEyeOffsetOrientation() const { return _eyeOffsetOrientation; }
    float getScale() const { return _scale; }
public slots:
    QString getModeString() const;
    void setModeString(const QString& mode);

    glm::vec3 getPosition() const { return _position + _hmdPosition; }
    void setPosition(const glm::vec3& position) { _position = position; }
    
    void setOrientation(const glm::quat& orientation) { setRotation(orientation); }
    glm::quat getOrientation() const { return getRotation(); }
    
    PickRay computePickRay(float x, float y);
    PickRay computeViewPickRay(float xRatio, float yRatio);
signals:
    void modeUpdated(const QString& newMode);

private:
    CameraMode _mode;
    glm::vec3 _position;
    float _fieldOfView; // degrees
    float _aspectRatio;
    float _nearClip;
    float _farClip;
    glm::vec3 _eyeOffsetPosition;
    glm::quat _eyeOffsetOrientation;
    glm::quat _rotation;
    glm::vec3 _hmdPosition;
    glm::quat _hmdRotation;
    glm::vec3 _targetPosition;
    glm::quat _targetRotation;
    float _scale;
};

#endif // hifi_Camera_h
