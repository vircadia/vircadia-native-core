//
//  OculusManager.h
//  interface/src/devices
//
//  Created by Stephen Birarda on 5/9/13.
//  Refactored by Ben Arnold on 6/30/2014
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OculusManager_h
#define hifi_OculusManager_h

#ifdef HAVE_LIBOVR
#include <OVR.h>
#include "../src/Util/Util_Render_Stereo.h"
#endif

#include "renderer/ProgramObject.h"

const float DEFAULT_OCULUS_UI_ANGULAR_SIZE = 72.0f;

class Camera;
class PalmData;

/// Handles interaction with the Oculus Rift.
class OculusManager {
public:
    static void connect();
    static void disconnect();
    static bool isConnected();
    static void beginFrameTiming();
    static void endFrameTiming();
    static void configureCamera(Camera& camera, int screenWidth, int screenHeight);
    static void display(const glm::quat &bodyOrientation, const glm::vec3 &position, Camera& whichCamera);
    static void reset();
    
    /// param \yaw[out] yaw in radians
    /// param \pitch[out] pitch in radians
    /// param \roll[out] roll in radians
    static void getEulerAngles(float& yaw, float& pitch, float& roll);
    static QSize getRenderTargetSize();
    
private:
#ifdef HAVE_LIBOVR
    static void generateDistortionMesh();
    static void renderDistortionMesh(ovrPosef eyeRenderPose[ovrEye_Count]);

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
    
    static ovrHmd _ovrHmd;
    static ovrHmdDesc _ovrHmdDesc;
    static ovrFovPort _eyeFov[ovrEye_Count];
    static ovrEyeRenderDesc _eyeRenderDesc[ovrEye_Count];
    static ovrSizei _renderTargetSize;
    static ovrVector2f _UVScaleOffset[ovrEye_Count][2];
    static GLuint _vertices[ovrEye_Count];
    static GLuint _indices[ovrEye_Count];
    static GLsizei _meshSize[ovrEye_Count];
    static ovrFrameTiming _hmdFrameTiming;
    static ovrRecti _eyeRenderViewport[ovrEye_Count];
    static unsigned int _frameIndex;
    static bool _frameTimingActive;
    static bool _programInitialized;
    static Camera* _camera;
#endif
};

#endif // hifi_OculusManager_h
