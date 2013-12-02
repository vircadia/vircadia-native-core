//
//  OculusManager.h
//  hifi
//
//  Created by Stephen Birarda on 5/9/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__OculusManager__
#define __hifi__OculusManager__

#include <iostream>

#ifdef HAVE_LIBOVR
#include <OVR.h>
#endif

#include "renderer/ProgramObject.h"

class Camera;

/// Handles interaction with the Oculus Rift.
class OculusManager {
public:
    static void connect();
    
    static bool isConnected() { return _isConnected; }
    
    static void configureCamera(Camera& camera, int screenWidth, int screenHeight);
    
    static void display(Camera& whichCamera);
    
    static void reset();
    
    static void getEulerAngles(float& yaw, float& pitch, float& roll);
    
    static void updateYawOffset();
    
private:
    static ProgramObject _program;
    static int _textureLocation;
    static int _lensCenterLocation;
    static int _screenCenterLocation;
    static int _scaleLocation;
    static int _scaleInLocation;
    static int _hmdWarpParamLocation;    
    static bool _isConnected;
    static float _yawOffset;
    
#ifdef HAVE_LIBOVR
    static OVR::Ptr<OVR::DeviceManager> _deviceManager;
    static OVR::Ptr<OVR::HMDDevice> _hmdDevice;
    static OVR::Ptr<OVR::SensorDevice> _sensorDevice;
    static OVR::SensorFusion* _sensorFusion;
    static OVR::Util::Render::StereoConfig _stereoConfig;
#endif
};

#endif /* defined(__hifi__OculusManager__) */
