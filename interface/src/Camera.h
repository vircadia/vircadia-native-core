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

const float HORIZONTAL_FIELD_OF_VIEW_DEGREES   = 90.0f;

enum CameraMode
{
    CAMERA_MODE_NULL = -1,
    CAMERA_MODE_THIRD_PERSON,
    CAMERA_MODE_FIRST_PERSON,
    CAMERA_MODE_MIRROR,
    NUM_CAMERA_MODES
};

class Camera
{
public:
    Camera();

    void initialize(); // instantly put the camera at the ideal position and rotation. 

    void update( float deltaTime );
    
    void setUpShift       ( float            u ) { _upShift        = u; }
    void setDistance      ( float            d ) { _distance       = d; }
    void setTargetPosition( const glm::vec3& t ) { _targetPosition = t; }
    void setPosition      ( const glm::vec3& p ) { _position       = p; }
    void setTightness     ( float            t ) { _tightness      = t; }
    void setTargetRotation( const glm::quat& rotation );
    
    void setMode                ( CameraMode  m      );
    void setModeShiftRate       ( float       r      );
    void setFieldOfView         ( float       f      );
    void setAspectRatio         ( float       a      );
    void setNearClip            ( float       n      );
    void setFarClip             ( float       f      );
    void setEyeOffsetPosition   ( const glm::vec3& p );
    void setEyeOffsetOrientation( const glm::quat& o );
    
    const glm::vec3& getTargetPosition       () { return _targetPosition; }
    const glm::vec3& getPosition             () { return _position;    }
    const glm::quat& getTargetRotation       () { return _targetRotation; }
    const glm::quat& getRotation             () { return _rotation;    }
    CameraMode       getMode                 () { return _mode;        }
    float            getFieldOfView          () { return _fieldOfView; }
    float            getAspectRatio          () { return _aspectRatio; }
    float            getNearClip             () { return _nearClip;    }
    float            getFarClip              () { return _farClip;     }
    const glm::vec3& getEyeOffsetPosition    () { return _eyeOffsetPosition;   }
    const glm::quat& getEyeOffsetOrientation () { return _eyeOffsetOrientation; }
    
    bool getFrustumNeedsReshape(); // call to find out if the view frustum needs to be reshaped
    void setFrustumWasReshaped();  // call this after reshaping the view frustum.

private:

    bool        _needsToInitialize;
    CameraMode  _mode;
    bool        _frustumNeedsReshape;
    glm::vec3   _position;
    glm::vec3   _idealPosition;
    glm::vec3   _targetPosition;
    float       _fieldOfView;
    float       _aspectRatio;
    float       _nearClip;
    float       _farClip;
    glm::vec3   _eyeOffsetPosition;
    glm::quat   _eyeOffsetOrientation;
    glm::quat   _rotation;
    glm::quat   _targetRotation;
    float       _upShift;
    float       _distance;
    float       _tightness;
    float       _previousUpShift;
    float       _previousDistance;
    float       _previousTightness;
    float       _newUpShift;
    float       _newDistance;
    float       _newTightness;
    float       _modeShift;
    float       _linearModeShift;
    float       _modeShiftRate;

    void updateFollowMode( float deltaTime );
};

#endif
