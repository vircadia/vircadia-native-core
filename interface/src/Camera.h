//
//  Camera.h
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__camera__
#define __interface__camera__

#include "Orientation.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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

    struct CameraFollowingAttributes
    {
        float upShift;
        float distance;
        float tightness;
    };

    void initialize(); // instantly put the camera at the ideal position and rotation. 

    void update( float deltaTime );
    
    void setYaw           ( float       y ) { _yaw            = y; }
    void setPitch         ( float       p ) { _pitch          = p; }
    void setRoll          ( float       r ) { _roll           = r; }
    void setUpShift       ( float       u ) { _upShift        = u; }
    void setDistance      ( float       d ) { _distance       = d; }
    void setTargetPosition( glm::vec3   t ) { _targetPosition = t; }
    void setTargetYaw     ( float       y ) { _idealYaw       = y; }
    void setPosition      ( glm::vec3   p ) { _position       = p; }
    void setTightness     ( float       t ) { _tightness      = t; }
    void setTargetRotation( float yaw, float pitch, float roll );
    
    void setMode          ( CameraMode  m );
    void setMode          ( CameraMode  m, CameraFollowingAttributes attributes );
    void setFieldOfView   ( float       f );
    void setAspectRatio   ( float       a );
    void setNearClip      ( float       n );
    void setFarClip       ( float       f );
    void setEyeOffsetPosition     ( const glm::vec3& p);
    void setEyeOffsetOrientation  ( const glm::quat& o);

    float       getYaw        () { return _yaw;         }
    float       getPitch      () { return _pitch;       }
    float       getRoll       () { return _roll;        }
    glm::vec3   getPosition   () { return _position;    }
    Orientation getOrientation() { return _orientation; }
    CameraMode  getMode       () { return _mode;        }
    float       getFieldOfView() { return _fieldOfView; }
    float       getAspectRatio() { return _aspectRatio; }
    float       getNearClip   () { return _nearClip;    }
    float       getFarClip    () { return _farClip;     }
    glm::vec3   getEyeOffsetPosition  () { return _eyeOffsetPosition;   }
    glm::quat   getEyeOffsetOrientation () { return _eyeOffsetOrientation; }
    bool        getFrustumNeedsReshape(); // call to find out if the view frustum needs to be reshaped
    void        setFrustumWasReshaped();  // call this after reshaping the view frustum.

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
    float       _yaw;
    float       _pitch;
    float       _roll;
    float       _upShift;
    float       _idealYaw;
    float       _idealPitch;
    float       _idealRoll;
    float       _distance;
    float       _tightness;
    Orientation _orientation;
    float       _modeShift;

    CameraFollowingAttributes _attributes[NUM_CAMERA_MODES];
    CameraFollowingAttributes _previousAttributes[NUM_CAMERA_MODES];
    
    void generateOrientation();
    void updateFollowMode( float deltaTime );
};

#endif
