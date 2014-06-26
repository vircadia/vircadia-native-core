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
    
    void setUpShift(float u) { _upShift = u; }
    void setDistance(float d) { _distance = d; }
    void setPosition(const glm::vec3& p) { _position = p; }
    void setTargetPosition(const glm::vec3& t);
    void setTightness(float t) { _tightness = t; }
    void setTargetRotation(const glm::quat& rotation);
    void setModeShiftPeriod(float r);
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
    float getModeShiftPeriod() const { return _modeShiftPeriod; }
    float getDistance() const { return _distance; }
    const glm::vec3& getTargetPosition() const { return _targetPosition; }
    const glm::quat& getTargetRotation() const { return _targetRotation; }
    float getFieldOfView() const { return _fieldOfView; }
    float getAspectRatio() const { return _aspectRatio; }
    float getNearClip() const { return _scale * _nearClip; }
    float getFarClip() const;
    const glm::vec3& getEyeOffsetPosition() const { return _eyeOffsetPosition;   }
    const glm::quat& getEyeOffsetOrientation() const { return _eyeOffsetOrientation; }
    float getScale() const { return _scale; }
    
    CameraMode getInterpolatedMode() const;

    bool getFrustumNeedsReshape() const; // call to find out if the view frustum needs to be reshaped
    void setFrustumWasReshaped();  // call this after reshaping the view frustum.

    // These only work on independent cameras
    /// one time change to what the camera is looking at
    void lookAt(const glm::vec3& value);

    /// fix what the camera is looking at, and keep the camera looking at this even if position changes
    void keepLookingAt(const glm::vec3& value);

    /// stops the keep looking at feature, doesn't change what's being looked at, but will stop camera from
    /// continuing to update it's orientation to keep looking at the item
    void stopLooking() { _isKeepLookingAt = false; }
    
private:

    bool _needsToInitialize;
    CameraMode _mode;
    CameraMode _prevMode;
    bool _frustumNeedsReshape;
    glm::vec3 _position;
    glm::vec3 _idealPosition;
    glm::vec3 _targetPosition;
    float _fieldOfView; // degrees
    float _aspectRatio;
    float _nearClip;
    float _farClip;
    glm::vec3 _eyeOffsetPosition;
    glm::quat _eyeOffsetOrientation;
    glm::quat _rotation;
    glm::quat _targetRotation;
    float _upShift;
    float _distance;
    float _tightness;
    float _previousUpShift;
    float _previousDistance;
    float _previousTightness;
    float _newUpShift;
    float _newDistance;
    float _newTightness;
    float _modeShift;
    float _linearModeShift;
    float _modeShiftPeriod;
    float _scale;

    glm::vec3 _lookingAt;
    bool _isKeepLookingAt;
    
    void updateFollowMode(float deltaTime);
};


class CameraScriptableObject  : public QObject {
    Q_OBJECT
public:
    CameraScriptableObject(Camera* camera, ViewFrustum* viewFrustum);

public slots:
    QString getMode() const;
    void setMode(const QString& mode);
    void setModeShiftPeriod(float r) {_camera->setModeShiftPeriod(r); }
    void setPosition(const glm::vec3& value) { _camera->setTargetPosition(value);}

    glm::vec3 getPosition() const { return _camera->getPosition(); }

    void setOrientation(const glm::quat& value) { _camera->setTargetRotation(value); }
    glm::quat getOrientation() const { return _camera->getRotation(); }

    // These only work on independent cameras
    /// one time change to what the camera is looking at
    void lookAt(const glm::vec3& value) { _camera->lookAt(value);}

    /// fix what the camera is looking at, and keep the camera looking at this even if position changes
    void keepLookingAt(const glm::vec3& value) { _camera->keepLookingAt(value);}

    /// stops the keep looking at feature, doesn't change what's being looked at, but will stop camera from
    /// continuing to update it's orientation to keep looking at the item
    void stopLooking() { _camera->stopLooking();}

    PickRay computePickRay(float x, float y);

private:
    Camera* _camera;
    ViewFrustum* _viewFrustum;
};
#endif // hifi_Camera_h
