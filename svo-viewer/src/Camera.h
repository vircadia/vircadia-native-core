//
//  Camera.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__camera__
#define __interface__camera__

#include <glm/glm.hpp>
#include <ViewFrustum.h>

//const float DEFAULT_FIELD_OF_VIEW_DEGREES = 90.0f;

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
    
    void setMode(CameraMode m);
    void setModeShiftPeriod(float r);
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
    float _fieldOfView;
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


#endif
