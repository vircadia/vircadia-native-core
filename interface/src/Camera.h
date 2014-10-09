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

class Camera {

public:
    Camera();

    void initialize(); // instantly put the camera at the ideal position and rotation. 

    void update( float deltaTime );

    void setPosition(const glm::vec3& p) { _position = p; }
    void setRotation(const glm::quat& rotation) { _rotation = rotation; };
    void setMode(CameraMode m);
    void setFieldOfView(float f);
    void setAspectRatio(float a);
    void setNearClip(float n);
    void setFarClip(float f);
    void setEyeOffsetPosition(const glm::vec3& p);
    void setEyeOffsetOrientation(const glm::quat& o);
    void setScale(const float s);
    
    const glm::vec3& getPosition() const { return _position; }
    const glm::quat& getRotation() const { return _rotation; }
    CameraMode getMode() const { return _mode; }
    float getFieldOfView() const { return _fieldOfView; }
    float getAspectRatio() const { return _aspectRatio; }
    float getNearClip() const { return _scale * _nearClip; }
    float getFarClip() const;
    const glm::vec3& getEyeOffsetPosition() const { return _eyeOffsetPosition;   }
    const glm::quat& getEyeOffsetOrientation() const { return _eyeOffsetOrientation; }
    float getScale() const { return _scale; }
    
private:

    bool _needsToInitialize;
    CameraMode _mode;
    glm::vec3 _position;
    float _fieldOfView; // degrees
    float _aspectRatio;
    float _nearClip;
    float _farClip;
    glm::vec3 _eyeOffsetPosition;
    glm::quat _eyeOffsetOrientation;
    glm::quat _rotation;

    float _scale;
};


class CameraScriptableObject  : public QObject {
    Q_OBJECT
public:
    CameraScriptableObject(Camera* camera, ViewFrustum* viewFrustum);

public slots:
    QString getMode() const;
    void setMode(const QString& mode);
    void setPosition(const glm::vec3& value) { _camera->setPosition(value);}

    glm::vec3 getPosition() const { return _camera->getPosition(); }

    void setOrientation(const glm::quat& value) { _camera->setRotation(value); }
    glm::quat getOrientation() const { return _camera->getRotation(); }

    PickRay computePickRay(float x, float y);

private:
    Camera* _camera;
    ViewFrustum* _viewFrustum;
};
#endif // hifi_Camera_h
