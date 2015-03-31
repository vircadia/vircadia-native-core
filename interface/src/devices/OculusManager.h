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

#include <OVR_CAPI.h>

#include <ProgramObject.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera;
class PalmData;
class Text3DOverlay;
// #define CLIENT_DISTORTION 1


/// Handles interaction with the Oculus Rift.
class OculusManager {
public:
    static void init();
    static void connect();
    static void disconnect();
    static bool isConnected();
    static void recalibrate();
    static void abandonCalibration();
    static void beginFrameTiming();
    static void endFrameTiming();
    static bool allowSwap();
    static void configureCamera(Camera& camera, int screenWidth, int screenHeight);
    static void display(const glm::quat &bodyOrientation, const glm::vec3 &position, Camera& whichCamera);
    static void reset();
    
    /// param \yaw[out] yaw in radians
    /// param \pitch[out] pitch in radians
    /// param \roll[out] roll in radians
    static void getEulerAngles(float& yaw, float& pitch, float& roll);
    static glm::vec3 getRelativePosition();
    static QSize getRenderTargetSize();
    
    static void overrideOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearVal,
        float& farVal, glm::vec4& nearClipPlane, glm::vec4& farClipPlane);
    
    static glm::vec3 getLeftEyePosition() { return _eyePositions[ovrEye_Left]; }
    static glm::vec3 getRightEyePosition() { return _eyePositions[ovrEye_Right]; }
    
    static int getHMDScreen();
    
private:
#ifdef CLIENT_DISTORTION
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
    static ovrVector2f _UVScaleOffset[ovrEye_Count][2];
    static GLuint _vertices[ovrEye_Count];
    static GLuint _indices[ovrEye_Count];
    static GLsizei _meshSize[ovrEye_Count];
    static ovrFrameTiming _hmdFrameTiming;
    static bool _programInitialized;
#endif

    static ovrTexture _eyeTextures[ovrEye_Count];
    static bool _isConnected;
    static glm::vec3 _eyePositions[ovrEye_Count];
    static ovrHmd _ovrHmd;
    static ovrFovPort _eyeFov[ovrEye_Count];
    static ovrVector3f _eyeOffset[ovrEye_Count];
    static glm::mat4 _eyeProjection[ovrEye_Count];
    static ovrEyeRenderDesc _eyeRenderDesc[ovrEye_Count];
    static ovrSizei _renderTargetSize;
    static unsigned int _frameIndex;
    static bool _frameTimingActive;
    static Camera* _camera;
    static ovrEyeType _activeEye;

    static void calibrate(const glm::vec3 position, const glm::quat orientation);
    enum CalibrationState {
        UNCALIBRATED,
        WAITING_FOR_DELTA,
        WAITING_FOR_ZERO,
        WAITING_FOR_ZERO_HELD,
        CALIBRATED
    };
    static void positionCalibrationBillboard(Text3DOverlay* message);
    static float CALIBRATION_DELTA_MINIMUM_LENGTH;
    static float CALIBRATION_DELTA_MINIMUM_ANGLE;
    static float CALIBRATION_ZERO_MAXIMUM_LENGTH;
    static float CALIBRATION_ZERO_MAXIMUM_ANGLE;
    static quint64 CALIBRATION_ZERO_HOLD_TIME;
    static float CALIBRATION_MESSAGE_DISTANCE;
    static CalibrationState _calibrationState;
    static glm::vec3 _calibrationPosition;
    static glm::quat _calibrationOrientation;
    static quint64 _calibrationStartTime;
    static int _calibrationMessage;

};


inline glm::mat4 toGlm(const ovrMatrix4f & om) {
    return glm::transpose(glm::make_mat4(&om.M[0][0]));
}

inline glm::mat4 toGlm(const ovrFovPort & fovport, float nearPlane = 0.01f, float farPlane = 10000.0f) {
    return toGlm(ovrMatrix4f_Projection(fovport, nearPlane, farPlane, true));
}

inline glm::vec3 toGlm(const ovrVector3f & ov) {
    return glm::make_vec3(&ov.x);
}

inline glm::vec2 toGlm(const ovrVector2f & ov) {
    return glm::make_vec2(&ov.x);
}

inline glm::uvec2 toGlm(const ovrSizei & ov) {
    return glm::uvec2(ov.w, ov.h);
}

inline glm::quat toGlm(const ovrQuatf & oq) {
    return glm::make_quat(&oq.x);
}

inline glm::mat4 toGlm(const ovrPosef & op) {
    glm::mat4 orientation = glm::mat4_cast(toGlm(op.Orientation));
    glm::mat4 translation = glm::translate(glm::mat4(), toGlm(op.Position));
    return translation * orientation;
}

inline ovrMatrix4f ovrFromGlm(const glm::mat4 & m) {
    ovrMatrix4f result;
    glm::mat4 transposed(glm::transpose(m));
    memcpy(result.M, &(transposed[0][0]), sizeof(float) * 16);
    return result;
}

inline ovrVector3f ovrFromGlm(const glm::vec3 & v) {
    return{ v.x, v.y, v.z };
}

inline ovrVector2f ovrFromGlm(const glm::vec2 & v) {
    return{ v.x, v.y };
}

inline ovrSizei ovrFromGlm(const glm::uvec2 & v) {
    return{ (int)v.x, (int)v.y };
}

inline ovrQuatf ovrFromGlm(const glm::quat & q) {
    return{ q.x, q.y, q.z, q.w };
}

#endif // hifi_OculusManager_h
