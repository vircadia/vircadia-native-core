//
//  Camera.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__camera__
#define __interface__camera__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

const float DEFAULT_FIELD_OF_VIEW_DEGREES = 90.0f;

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
    void setTargetPosition(const glm::vec3& t) { _targetPosition = t; }
    void setTightness(float t) { _tightness = t; }
    void setTargetRotation(const glm::quat& rotation);
    
    void setMode(CameraMode m);
    void setModeShiftRate(float r);
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
    
    bool getWantsAutoFollow() const { return _wantsAutoFollow; }
    void setWantsAutoFollow(bool value) { _wantsAutoFollow = value; }

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
    float _modeShiftRate;
    float _scale;
    
    bool _wantsAutoFollow;

    void updateFollowMode(float deltaTime);
};


class CameraScriptableObject  : public QObject {
    Q_OBJECT
public:
    CameraScriptableObject(Camera* camera) { _camera = camera; }

public slots:
    QString getMode() const;
    void setMode(const QString& mode);
    void setPosition(const glm::vec3& p);
    glm::vec3 getPosition() const { return _camera->getPosition(); }

    //bool getWantsAutoFollow() const { return _camera->getWantsAutoFollow(); }
    //void setWantsAutoFollow(bool value) { _camera->setWantsAutoFollow(value); }

private:
    Camera* _camera;
};
#endif
