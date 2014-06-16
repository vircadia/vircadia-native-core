//
//  OculusManager.h
//  interface/src/devices
//
//  Created by Stephen Birarda on 5/9/13.
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OculusManager_h
#define hifi_OculusManager_h

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
    
    static bool isConnected();
    
    static void configureCamera(Camera& camera, int screenWidth, int screenHeight);
    
    static void display(Camera& whichCamera);
    
    static void reset();
    
    /// param \yaw[out] yaw in radians
    /// param \pitch[out] pitch in radians
    /// param \roll[out] roll in radians
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
    
#ifdef HAVE_LIBOVR
    static OVR::Ptr<OVR::DeviceManager> _deviceManager;
    static OVR::Ptr<OVR::HMDDevice> _hmdDevice;
    static OVR::Ptr<OVR::SensorDevice> _sensorDevice;
    static OVR::SensorFusion* _sensorFusion;
    static OVR::Util::Render::StereoConfig _stereoConfig;
#endif
};

#endif // hifi_OculusManager_h
