//-----------------------------------------------------------
//
// Created by Jeffrey Ventrella and added as a utility 
// class for High Fidelity Code base, April 2013
//
//-----------------------------------------------------------

#ifndef __interface__camera__
#define __interface__camera__

#include "Orientation.h"
#include <glm/glm.hpp>

enum CameraMode
{
    CAMERA_MODE_NULL = -1,
    CAMERA_MODE_THIRD_PERSON,
    CAMERA_MODE_FIRST_PERSON,
    CAMERA_MODE_MY_OWN_FACE,
    NUM_CAMERA_MODES
};

const float MODE_SHIFT_RATE = 2.0f;

class Camera
{
public:
    Camera();

	void update( float deltaTime );
	
    void setYaw           ( float       y ) { _yaw            = y; }
    void setPitch         ( float       p ) { _pitch          = p; }
    void setRoll          ( float       r ) { _roll           = r; }
    void setUpShift       ( float       u ) { _upShift        = u; }
    void setRightShift    ( float       r ) { _rightShift     = r; }
    void setDistance      ( float       d ) { _distance       = d; }
    void setTargetPosition( glm::vec3   t ) { _targetPosition = t; }
    void setTargetYaw     ( float       y ) { _idealYaw       = y; }
    void setPosition      ( glm::vec3   p ) { _position       = p; }
    void setOrientation   ( Orientation o ) { _orientation.set(o); }
    void setTightness     ( float       t ) { _tightness      = t; }
    
    void setMode          ( CameraMode  m );
    void setFieldOfView   ( float       f );
    void setAspectRatio   ( float       a );
    void setNearClip      ( float       n );
    void setFarClip       ( float       f );

    float       getYaw        () { return _yaw;         }
    float       getPitch      () { return _pitch;       }
    float       getRoll       () { return _roll;        }
    glm::vec3   getPosition   () { return _position;    }
    Orientation getOrientation() { return _orientation; }
    CameraMode  getMode       () { return _mode;        }
    float       getModeShift  () { return _modeShift;   }
    float       getFieldOfView() { return _fieldOfView; }
    float       getAspectRatio() { return _aspectRatio; }
    float       getNearClip   () { return _nearClip;    }
    float       getFarClip    () { return _farClip;     }
    bool        getFrustumNeedsReshape(); // call to find out if the view frustum needs to be reshaped
    void        setFrustumWasReshaped();  // call this after reshaping the view frustum.

private:

	CameraMode  _mode;
    float       _modeShift; // 0.0 to 1.0
    bool        _frustumNeedsReshape;
	glm::vec3	_position;
	glm::vec3	_idealPosition;
	glm::vec3	_targetPosition;
	float		_fieldOfView;
    float		_aspectRatio;
    float		_nearClip;
    float		_farClip;
	float		_yaw;
	float		_pitch;
	float		_roll;
	float		_upShift;
	float		_rightShift;
	float		_idealYaw;
	float		_distance;
	float		_tightness;
	Orientation	_orientation;
    
    void updateFollowMode( float deltaTime );
};

#endif
