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

#ifdef HAVE_LIBOVR
#include <OVR.h>
//#include <../src/OVR_CAPI.h>
#endif

#include "../src/Util/Util_Render_Stereo.h"
using namespace OVR::Util::Render;

#include <../src/Kernel/OVR_SysFile.h>
#include <../src/Kernel/OVR_Log.h>
#include <../src/Kernel/OVR_Timer.h>

#include "renderer/ProgramObject.h"


const float DEFAULT_OCULUS_UI_ANGULAR_SIZE = 72.0f;

class Camera;

/// Handles interaction with the Oculus Rift.
class OculusManager {
public:
    static void connect();
    
    static void generateDistortionMesh();

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

    struct DistortionVertex {
        glm::vec2 pos;
        glm::vec2 texR;
        glm::vec2 texG;
        glm::vec2 texB;
        struct {
            GLubyte r;
            GLubyte g;
            GLubyte b;
            GLubyte a;
        } color;
    };

    static ProgramObject _program;
    //Uniforms
    static int _textureLocation;
    static int _eyeToSourceUVScaleLocation;
    static int _eyeToSourceUVOffsetLocation;
    static int _eyeRotationStartLocation;
    static int _eyeRotationEndLocation;
    //Attributes
    static int _positionAttributeLocation;
    static int _colorAttributeLocation;
    static int _texCoord0AttributeLocation;
    static int _texCoord1AttributeLocation;
    static int _texCoord2AttributeLocation;

    static bool _isConnected;
    


#ifdef HAVE_LIBOVR
    static ovrHmd _ovrHmd;
    static ovrHmdDesc _ovrHmdDesc;
    static ovrFovPort _eyeFov[ovrEye_Count];
    static ovrSizei _renderTargetSize;
    static ovrVector2f _UVScaleOffset[ovrEye_Count][2];
    static GLuint _vbo[ovrEye_Count];
    static GLuint _indicesVbo[ovrEye_Count];
    static GLsizei _meshSize[ovrEye_Count];
    static ovrFrameTiming _hmdFrameTiming;
#endif
};

#endif // hifi_OculusManager_h
