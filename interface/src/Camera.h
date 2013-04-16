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
    CAMERA_MODE_FIRST_PERSON,
    CAMERA_MODE_THIRD_PERSON,
    CAMERA_MODE_MY_OWN_FACE,
    NUM_CAMERA_MODES
};

static const float DEFAULT_CAMERA_TIGHTNESS = 10.0f;

class Camera
{
public:
    Camera();

	void update( float deltaTime );

    void setMode            ( CameraMode    m ) { _mode             = m; }
    void setYaw             ( float         y ) { _yaw              = y; }
    void setPitch           ( float         p ) { _pitch            = p; }
    void setRoll            ( float         r ) { _roll             = r; }
    void setUp              ( float         u ) { _up               = u; }
    void setDistance        ( float         d ) { _distance         = d; }
    void setTargetPosition  ( glm::vec3     t ) { _targetPosition   = t; };
    void setPosition        ( glm::vec3     p ) { _position         = p; };
    void setOrientation     ( Orientation   o ) { _orientation.set(o);   }
    void setTightness       ( float         t ) { _tightness        = t; }
    void setFieldOfView     ( float         f ) { _fieldOfView      = f; }
    void setAspectRatio     ( float         a ) { _aspectRatio      = a; }
    void setNearClip        ( float         n ) { _nearClip         = n; }
    void setFarClip         ( float         f ) { _farClip          = f; }

    float       getYaw              () { return _yaw;               }
    float       getPitch            () { return _pitch;             }
    float       getRoll             () { return _roll;              }
    glm::vec3   getPosition         () { return _position;          }
    Orientation getOrientation      () { return _orientation;       }
    CameraMode  getMode             () { return _mode;              }
    float       getFieldOfView      () { return _fieldOfView;       }
    float       getAspectRatio      () { return _aspectRatio;       }
    float       getNearClip         () { return _nearClip;          }
    float       getFarClip          () { return _farClip;           }

private:

    CameraMode	_mode;
    glm::vec3   _position;
	glm::vec3	_idealPosition;
    glm::vec3   _targetPosition;
    float       _yaw;
    float       _pitch;
    float       _roll;
    float       _up;
    float       _distance;
	float		_tightness;
    Orientation _orientation;

    // Lens attributes
    float       _fieldOfView;       // in degrees
    float       _aspectRatio;       // width/height
    float       _nearClip;          // in world units? - XXXBHG - we need to think about this!
    float       _farClip;           // in world units?
};

#endif
