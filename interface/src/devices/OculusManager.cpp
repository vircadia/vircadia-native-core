//
//  OculusManager.cpp
//  interface/src/devices
//
//  Created by Stephen Birarda on 5/9/13.
//  Refactored by Ben Arnold on 6/30/2014
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OculusManager.h"
#include <gpu/GPUConfig.h>

#include <QDesktopWidget>
#include <QGuiApplication>
#include <QScreen>
#include <QOpenGLTimerQuery>
#include <QGLWidget>

#include <CursorManager.h>
#include <glm/glm.hpp>

#include <avatar/AvatarManager.h>
#include <avatar/MyAvatar.h>
#include <GlWindow.h>
#include <gpu/GLBackend.h>
#include <OglplusHelpers.h>
#include <PathUtils.h>
#include <SharedUtil.h>
#include <UserActivityLogger.h>

#include <OVR_CAPI_GL.h>

#include "InterfaceLogging.h"
#include "Application.h"
#include "ui/overlays/Text3DOverlay.h"

template <typename Function>
void for_each_eye(Function function) {
    for (ovrEyeType eye = ovrEyeType::ovrEye_Left;
        eye < ovrEyeType::ovrEye_Count;
        eye = static_cast<ovrEyeType>(eye + 1)) {
        function(eye);
    }
}

template <typename Function>
void for_each_eye(const ovrHmd & hmd, Function function) {
    for (int i = 0; i < ovrEye_Count; ++i) {
        ovrEyeType eye = hmd->EyeRenderOrder[i];
        function(eye);
    }
}
enum CalibrationState {
    UNCALIBRATED,
    WAITING_FOR_DELTA,
    WAITING_FOR_ZERO,
    WAITING_FOR_ZERO_HELD,
    CALIBRATED
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

inline glm::ivec2 toGlm(const ovrVector2i & ov) {
    return glm::ivec2(ov.x, ov.y);
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



#ifdef Q_OS_WIN

// A base class for FBO wrappers that need to use the Oculus C
// API to manage textures via ovrHmd_CreateSwapTextureSetGL,
// ovrHmd_CreateMirrorTextureGL, etc
template <typename C>
struct RiftFramebufferWrapper : public FramebufferWrapper<C, char> {
    ovrHmd hmd;
    RiftFramebufferWrapper(const ovrHmd & hmd) : hmd(hmd) {
        color = 0;
        depth = 0;
    };

    void Resize(const uvec2 & size) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oglplus::GetName(fbo));
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        this->size = size;
        initColor();
        initDone();
    }

protected:
    virtual void initDepth() override final {
    }
};

// A wrapper for constructing and using a swap texture set,
// where each frame you draw to a texture via the FBO,
// then submit it and increment to the next texture.
// The Oculus SDK manages the creation and destruction of
// the textures
struct SwapFramebufferWrapper : public RiftFramebufferWrapper<ovrSwapTextureSet*> {
    SwapFramebufferWrapper(const ovrHmd & hmd)
        : RiftFramebufferWrapper(hmd) {
    }

    ~SwapFramebufferWrapper() {
        if (color) {
            ovrHmd_DestroySwapTextureSet(hmd, color);
            color = nullptr;
        }
    }

    void Increment() {
        ++color->CurrentIndex;
        color->CurrentIndex %= color->TextureCount;
    }

protected:
    virtual void initColor() override {
        if (color) {
            ovrHmd_DestroySwapTextureSet(hmd, color);
            color = nullptr;
        }

        ovrResult result = ovrHmd_CreateSwapTextureSetGL(hmd, GL_RGBA, size.x, size.y, &color);
        Q_ASSERT(OVR_SUCCESS(result));

        for (int i = 0; i < color->TextureCount; ++i) {
            ovrGLTexture& ovrTex = (ovrGLTexture&)color->Textures[i];
            glBindTexture(GL_TEXTURE_2D, ovrTex.OGL.TexId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    virtual void initDone() override {
    }

    virtual void onBind(oglplus::Framebuffer::Target target) override {
        ovrGLTexture& tex = (ovrGLTexture&)(color->Textures[color->CurrentIndex]);
        glFramebufferTexture2D(toEnum(target), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.OGL.TexId, 0);
    }

    virtual void onUnbind(oglplus::Framebuffer::Target target) override {
        glFramebufferTexture2D(toEnum(target), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    }
};


// We use a FBO to wrap the mirror texture because it makes it easier to
// render to the screen via glBlitFramebuffer
struct MirrorFramebufferWrapper : public RiftFramebufferWrapper<ovrGLTexture*> {
    MirrorFramebufferWrapper(const ovrHmd & hmd)
        : RiftFramebufferWrapper(hmd) {
    }

    virtual ~MirrorFramebufferWrapper() {
        if (color) {
            ovrHmd_DestroyMirrorTexture(hmd, (ovrTexture*)color);
            color = nullptr;
        }
    }

private:
    void initColor() override {
        if (color) {
            ovrHmd_DestroyMirrorTexture(hmd, (ovrTexture*)color);
            color = nullptr;
        }
        ovrResult result = ovrHmd_CreateMirrorTextureGL(hmd, GL_RGBA, size.x, size.y, (ovrTexture**)&color);
        Q_ASSERT(OVR_SUCCESS(result));
    }

    void initDone() override {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oglplus::GetName(fbo));
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color->OGL.TexId, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
};

static SwapFramebufferWrapper* _swapFbo{ nullptr };
static MirrorFramebufferWrapper* _mirrorFbo{ nullptr };
static ovrLayerEyeFov _sceneLayer;

#else

static ovrTexture _eyeTextures[ovrEye_Count];
static GlWindow* _outputWindow{ nullptr };

#endif

static bool _isConnected = false;
static ovrHmd _ovrHmd;
static ovrFovPort _eyeFov[ovrEye_Count];
static ovrEyeRenderDesc _eyeRenderDesc[ovrEye_Count];
static ovrSizei _renderTargetSize;
static glm::mat4 _eyeProjection[ovrEye_Count];
static unsigned int _frameIndex = 0;
static bool _frameTimingActive = false;
static Camera* _camera = NULL;
static ovrEyeType _activeEye = ovrEye_Count;
static bool _hswDismissed = false;

static const float CALIBRATION_DELTA_MINIMUM_LENGTH = 0.02f;
static const float CALIBRATION_DELTA_MINIMUM_ANGLE = 5.0f * RADIANS_PER_DEGREE;
static const float CALIBRATION_ZERO_MAXIMUM_LENGTH = 0.01f;
static const float CALIBRATION_ZERO_MAXIMUM_ANGLE = 2.0f * RADIANS_PER_DEGREE;
static const quint64 CALIBRATION_ZERO_HOLD_TIME = 3000000; // usec
static const float CALIBRATION_MESSAGE_DISTANCE = 2.5f;
static CalibrationState _calibrationState;
static glm::vec3 _calibrationPosition;
static glm::quat _calibrationOrientation;
static quint64 _calibrationStartTime;
static int _calibrationMessage = 0;
static glm::vec3 _eyePositions[ovrEye_Count];
// TODO expose this as a developer toggle
static bool _eyePerFrameMode = false;
static ovrEyeType _lastEyeRendered = ovrEye_Count;
static ovrSizei _recommendedTexSize = { 0, 0 };
static float _offscreenRenderScale = 1.0;
static glm::mat4 _combinedProjection;
static ovrPosef _eyeRenderPoses[ovrEye_Count];
static ovrRecti _eyeViewports[ovrEye_Count];
static ovrVector3f _eyeOffsets[ovrEye_Count];


glm::vec3 OculusManager::getLeftEyePosition() { return _eyePositions[ovrEye_Left]; }
glm::vec3 OculusManager::getRightEyePosition() { return _eyePositions[ovrEye_Right]; }
glm::vec3 OculusManager::getMidEyePosition() { return (_eyePositions[ovrEye_Left] + _eyePositions[ovrEye_Right]) / 2.0f; }

void OculusManager::connect(QOpenGLContext* shareContext) {
    qCDebug(interfaceapp) << "Oculus SDK" << OVR_VERSION_STRING;

    ovrInitParams initParams; memset(&initParams, 0, sizeof(initParams));

#ifdef DEBUG
    initParams.Flags |= ovrInit_Debug;
#endif

    ovr_Initialize(&initParams);

#ifdef Q_OS_WIN

    ovrResult res = ovrHmd_Create(0, &_ovrHmd);
#ifdef DEBUG
    if (!OVR_SUCCESS(res)) {
        res = ovrHmd_CreateDebug(ovrHmd_DK2, &_ovrHmd);
        Q_ASSERT(OVR_SUCCESS(res));
    }
#endif

#else

    _ovrHmd = ovrHmd_Create(0);
#ifdef DEBUG
    if (!_ovrHmd) {
        _ovrHmd = ovrHmd_CreateDebug(ovrHmd_DK2);
    }
#endif
    
#endif

    if (!_ovrHmd) {
        _isConnected = false;

        // we're definitely not in "VR mode" so tell the menu that
        Menu::getInstance()->getActionForOption(MenuOption::EnableVRMode)->setChecked(false);
        ovr_Shutdown();
        return;
    }

    _calibrationState = UNCALIBRATED;
    if (!_isConnected) {
        UserActivityLogger::getInstance().connectedDevice("hmd", "oculus");
    }
    _isConnected = true;

    for_each_eye([&](ovrEyeType eye) {
        _eyeFov[eye] = _ovrHmd->DefaultEyeFov[eye];
        _eyeProjection[eye] = toGlm(ovrMatrix4f_Projection(_eyeFov[eye], 
            DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_RightHanded));
        ovrEyeRenderDesc erd = ovrHmd_GetRenderDesc(_ovrHmd, eye, _eyeFov[eye]);
        _eyeOffsets[eye] = erd.HmdToEyeViewOffset;
    });
    ovrFovPort combinedFov = _ovrHmd->MaxEyeFov[0];
    combinedFov.RightTan = _ovrHmd->MaxEyeFov[1].RightTan;
    _combinedProjection = toGlm(ovrMatrix4f_Projection(combinedFov, 
        DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_RightHanded));

    _recommendedTexSize = ovrHmd_GetFovTextureSize(_ovrHmd, ovrEye_Left, _eyeFov[ovrEye_Left], 1.0f);
    _renderTargetSize = { _recommendedTexSize.w * 2, _recommendedTexSize.h };

#ifdef Q_OS_WIN

    _mirrorFbo = new MirrorFramebufferWrapper(_ovrHmd);
    _swapFbo = new SwapFramebufferWrapper(_ovrHmd);
    _swapFbo->Init(toGlm(_renderTargetSize));
    _sceneLayer.ColorTexture[0] = _swapFbo->color;
    _sceneLayer.ColorTexture[1] = nullptr;
    _sceneLayer.Viewport[0].Pos = { 0, 0 };
    _sceneLayer.Viewport[0].Size = _recommendedTexSize;
    _sceneLayer.Viewport[1].Pos = { _recommendedTexSize.w, 0 };
    _sceneLayer.Viewport[1].Size = _recommendedTexSize;
    _sceneLayer.Header.Type = ovrLayerType_EyeFov;
    _sceneLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
    for_each_eye([&](ovrEyeType eye) {
        _eyeViewports[eye] = _sceneLayer.Viewport[eye];
        _sceneLayer.Fov[eye] = _eyeFov[eye];
    });



#else
    _outputWindow = new GlWindow(shareContext);
    _outputWindow->show();
//    _outputWindow->setFlags(Qt::FramelessWindowHint );
//    _outputWindow->resize(_ovrHmd->Resolution.w, _ovrHmd->Resolution.h);
//    _outputWindow->setPosition(_ovrHmd->WindowsPos.x, _ovrHmd->WindowsPos.y);
    ivec2 desiredPosition = toGlm(_ovrHmd->WindowsPos);
    foreach(QScreen* screen, qGuiApp->screens()) {
        ivec2 screenPosition = toGlm(screen->geometry().topLeft());
        if (screenPosition == desiredPosition) {
            _outputWindow->setScreen(screen);
            break;
        }
    }
    _outputWindow->showFullScreen();
    _outputWindow->makeCurrent();
    
    ovrGLConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.BackBufferSize = _ovrHmd->Resolution;
    cfg.OGL.Header.Multisample = 0;

    int distortionCaps = 0
        | ovrDistortionCap_Vignette
        | ovrDistortionCap_Overdrive
        | ovrDistortionCap_TimeWarp;

    int configResult = ovrHmd_ConfigureRendering(_ovrHmd, &cfg.Config,
        distortionCaps, _eyeFov, _eyeRenderDesc);
    assert(configResult);
    Q_UNUSED(configResult);
    
    _outputWindow->doneCurrent();

    for_each_eye([&](ovrEyeType eye) {
        //Get texture size
        _eyeTextures[eye].Header.API = ovrRenderAPI_OpenGL;
        _eyeTextures[eye].Header.TextureSize = _renderTargetSize;
        _eyeTextures[eye].Header.RenderViewport.Pos = { 0, 0 };
        _eyeTextures[eye].Header.RenderViewport.Size = _renderTargetSize;
        _eyeTextures[eye].Header.RenderViewport.Size.w /= 2;
    });
    _eyeTextures[ovrEye_Right].Header.RenderViewport.Pos.x = _recommendedTexSize.w;
    for_each_eye([&](ovrEyeType eye) {
        _eyeViewports[eye] = _eyeTextures[eye].Header.RenderViewport;
    });
#endif

    ovrHmd_SetEnabledCaps(_ovrHmd,
            ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);

    ovrHmd_ConfigureTracking(_ovrHmd,
            ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection,
            ovrTrackingCap_Orientation);

    if (!_camera) {
        _camera = new Camera;
        configureCamera(*_camera); // no need to use screen dimensions; they're ignored
    }
}

//Disconnects and deallocates the OR
void OculusManager::disconnect() {
    if (_isConnected) {

#ifdef Q_OS_WIN
        if (_swapFbo) {
            delete _swapFbo;
            _swapFbo = nullptr;
        }

        if (_mirrorFbo) {
            delete _mirrorFbo;
            _mirrorFbo = nullptr;
        }
#else
        _outputWindow->showNormal();
        _outputWindow->deleteLater();
        _outputWindow = nullptr;
#endif

        if (_ovrHmd) {
            ovrHmd_Destroy(_ovrHmd);
            _ovrHmd = nullptr;
        }
        ovr_Shutdown();
    
        _isConnected = false;
        // Prepare to potentially have to dismiss the HSW again
        // if the user re-enables VR
        _hswDismissed = false;
    }
}

void positionCalibrationBillboard(Text3DOverlay* billboard) {
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    glm::quat headOrientation = myAvatar->getHeadOrientation();
    headOrientation.x = 0;
    headOrientation.z = 0;
    glm::normalize(headOrientation);
    billboard->setPosition(myAvatar->getHeadPosition()
        + headOrientation * glm::vec3(0.0f, 0.0f, -CALIBRATION_MESSAGE_DISTANCE));
    billboard->setRotation(headOrientation);
}

void calibrate(const glm::vec3& position, const glm::quat& orientation) {
    static QString instructionMessage = "Hold still to calibrate";
    static QString progressMessage;
    static Text3DOverlay* billboard;

    switch (_calibrationState) {

        case UNCALIBRATED:
            if (position != glm::vec3() && orientation != glm::quat()) {  // Handle zero values at start-up.
                _calibrationPosition = position;
                _calibrationOrientation = orientation;
                _calibrationState = WAITING_FOR_DELTA;
            }
            break;

        case WAITING_FOR_DELTA:
            if (glm::length(position - _calibrationPosition) > CALIBRATION_DELTA_MINIMUM_LENGTH
                || glm::angle(orientation * glm::inverse(_calibrationOrientation)) > CALIBRATION_DELTA_MINIMUM_ANGLE) {
                _calibrationPosition = position;
                _calibrationOrientation = orientation;
                _calibrationState = WAITING_FOR_ZERO;
            }
            break;

        case WAITING_FOR_ZERO:
            if (glm::length(position - _calibrationPosition) < CALIBRATION_ZERO_MAXIMUM_LENGTH
                && glm::angle(orientation * glm::inverse(_calibrationOrientation)) < CALIBRATION_ZERO_MAXIMUM_ANGLE) {
                _calibrationStartTime = usecTimestampNow();
                _calibrationState = WAITING_FOR_ZERO_HELD;

                if (!_calibrationMessage) {
                    qCDebug(interfaceapp) << "Hold still to calibrate HMD";

                    billboard = new Text3DOverlay();
                    billboard->setDimensions(glm::vec2(2.0f, 1.25f));
                    billboard->setTopMargin(0.35f);
                    billboard->setLeftMargin(0.28f);
                    billboard->setText(instructionMessage);
                    billboard->setAlpha(0.5f);
                    billboard->setLineHeight(0.1f);
                    billboard->setIsFacingAvatar(false);
                    positionCalibrationBillboard(billboard);

                    _calibrationMessage = Application::getInstance()->getOverlays().addOverlay(billboard);
                }

                progressMessage = "";
            } else {
                _calibrationPosition = position;
                _calibrationOrientation = orientation;
            }
            break;

        case WAITING_FOR_ZERO_HELD:
            if (glm::length(position - _calibrationPosition) < CALIBRATION_ZERO_MAXIMUM_LENGTH
                && glm::angle(orientation * glm::inverse(_calibrationOrientation)) < CALIBRATION_ZERO_MAXIMUM_ANGLE) {
                if ((usecTimestampNow() - _calibrationStartTime) > CALIBRATION_ZERO_HOLD_TIME) {
                    _calibrationState = CALIBRATED;
                    qCDebug(interfaceapp) << "HMD calibrated";
                    Application::getInstance()->getOverlays().deleteOverlay(_calibrationMessage);
                    _calibrationMessage = 0;
                    Application::getInstance()->resetSensors();
                } else {
                    quint64 quarterSeconds = (usecTimestampNow() - _calibrationStartTime) / 250000;
                    if (quarterSeconds + 1 > (quint64)progressMessage.length()) {
                        // 3...2...1...
                        if (quarterSeconds == 4 * (quarterSeconds / 4)) {
                            quint64 wholeSeconds = CALIBRATION_ZERO_HOLD_TIME / 1000000 - quarterSeconds / 4;

                            if (wholeSeconds == 3) {
                                positionCalibrationBillboard(billboard);
                            }

                            progressMessage += QString::number(wholeSeconds);
                        } else {
                            progressMessage += ".";
                        }
                        billboard->setText(instructionMessage + "\n\n" + progressMessage);
                    }
                }
            } else {
                _calibrationPosition = position;
                _calibrationOrientation = orientation;
                _calibrationState = WAITING_FOR_ZERO;
            }
            break;
        default:
            break;

    }
}

void OculusManager::recalibrate() {
    _calibrationState = UNCALIBRATED;
}

void OculusManager::abandonCalibration() {
    _calibrationState = CALIBRATED;
    if (_calibrationMessage) {
        qCDebug(interfaceapp) << "Abandoned HMD calibration";
        Application::getInstance()->getOverlays().deleteOverlay(_calibrationMessage);
        _calibrationMessage = 0;
    }
}


bool OculusManager::isConnected() {
    return _isConnected;
}

//Begins the frame timing for oculus prediction purposes
void OculusManager::beginFrameTiming() {
    if (_frameTimingActive) {
        printf("WARNING: Called OculusManager::beginFrameTiming() twice in a row, need to call OculusManager::endFrameTiming().");
    }
   _frameTimingActive = true;
}

bool OculusManager::allowSwap() {
    return false;
}

//Ends frame timing
void OculusManager::endFrameTiming() {
    _frameIndex++;
    _frameTimingActive = false;
}

//Sets the camera FoV and aspect ratio
void OculusManager::configureCamera(Camera& camera) {
    if (_activeEye == ovrEye_Count) {
        // When not rendering, provide a FOV encompasing both eyes
        camera.setProjection(_combinedProjection);
        return;
    } 
    camera.setProjection(_eyeProjection[_activeEye]);
}

//Displays everything for the oculus, frame timing must be active
void OculusManager::display(QGLWidget * glCanvas, RenderArgs* renderArgs, const glm::quat &bodyOrientation, const glm::vec3 &position, Camera& whichCamera) {

#ifdef DEBUG
    // Ensure the frame counter always increments by exactly 1
    static int oldFrameIndex = -1;
    assert(oldFrameIndex == -1 || (unsigned int)oldFrameIndex == _frameIndex - 1);
    oldFrameIndex = _frameIndex;
#endif
    
#ifndef Q_OS_WIN

    // FIXME:  we need a better way of responding to the HSW.  In particular
    // we need to ensure that it's only displayed once per session, rather than
    // every time the user toggles VR mode, and we need to hook it up to actual
    // keyboard input.  OVR claim they are refactoring HSW
    // https://forums.oculus.com/viewtopic.php?f=20&t=21720#p258599
    static ovrHSWDisplayState hasWarningState;
    if (!_hswDismissed) {
        ovrHmd_GetHSWDisplayState(_ovrHmd, &hasWarningState);
        if (hasWarningState.Displayed) {
            ovrHmd_DismissHSWDisplay(_ovrHmd);
        } else {
            _hswDismissed = true;
        }
    }
#endif


    //beginFrameTiming must be called before display
    if (!_frameTimingActive) {
        printf("WARNING: Called OculusManager::display() without calling OculusManager::beginFrameTiming() first.");
        return;
    }

    auto primaryFBO = DependencyManager::get<TextureCache>()->getPrimaryFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(primaryFBO));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::quat orientation;
    glm::vec3 trackerPosition;
    auto deviceSize = qApp->getDeviceSize();

    ovrTrackingState ts = ovrHmd_GetTrackingState(_ovrHmd, ovr_GetTimeInSeconds());
    ovrVector3f ovrHeadPosition = ts.HeadPose.ThePose.Position;

    trackerPosition = glm::vec3(ovrHeadPosition.x, ovrHeadPosition.y, ovrHeadPosition.z);

    if (_calibrationState != CALIBRATED) {
        ovrQuatf ovrHeadOrientation = ts.HeadPose.ThePose.Orientation;
        orientation = glm::quat(ovrHeadOrientation.w, ovrHeadOrientation.x, ovrHeadOrientation.y, ovrHeadOrientation.z);
        calibrate(trackerPosition, orientation);
    }

    trackerPosition = bodyOrientation * trackerPosition;
    ovrPosef eyePoses[ovrEye_Count];
    ovrHmd_GetEyePoses(_ovrHmd, _frameIndex, _eyeOffsets, eyePoses, nullptr);
#ifndef Q_OS_WIN
    ovrHmd_BeginFrame(_ovrHmd, _frameIndex);
#endif
    //Render each eye into an fbo
    for_each_eye(_ovrHmd, [&](ovrEyeType eye){
        // If we're in eye-per-frame mode, only render one eye
        // per call to display, and allow timewarp to correct for
        // the other eye.  Poor man's perf improvement
        if (_eyePerFrameMode && eye == _lastEyeRendered) {
            return;
        }
        _lastEyeRendered = _activeEye = eye;
        _eyeRenderPoses[eye] = eyePoses[eye];
        // Set the camera rotation for this eye

        _eyePositions[eye] = toGlm(_eyeRenderPoses[eye].Position);
        _eyePositions[eye] = whichCamera.getRotation() * _eyePositions[eye];
        quat eyeRotation = toGlm(_eyeRenderPoses[eye].Orientation);
        
        // Update our camera to what the application camera is doing
        _camera->setRotation(whichCamera.getRotation() * eyeRotation);
        _camera->setPosition(whichCamera.getPosition() + _eyePositions[eye]);
        configureCamera(*_camera);
        _camera->update(1.0f / Application::getInstance()->getFps());

        ovrRecti & vp = _eyeViewports[eye];
        vp.Size.h = _recommendedTexSize.h * _offscreenRenderScale;
        vp.Size.w = _recommendedTexSize.w * _offscreenRenderScale;
        glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);

        renderArgs->_viewport = glm::ivec4(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
        renderArgs->_renderSide = RenderArgs::MONO;
        qApp->displaySide(renderArgs, *_camera);
        qApp->getApplicationCompositor().displayOverlayTextureHmd(renderArgs, eye);
    });
    _activeEye = ovrEye_Count;

    gpu::FramebufferPointer finalFbo;
    finalFbo = DependencyManager::get<TextureCache>()->getPrimaryFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // restore our normal viewport
    glViewport(0, 0, deviceSize.width(), deviceSize.height());

#ifdef Q_OS_WIN
    auto srcFboSize = finalFbo->getSize();


    // Blit to the oculus provided texture
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(finalFbo));
    _swapFbo->Bound(oglplus::Framebuffer::Target::Draw, [&] {
        glBlitFramebuffer(
            0, 0, srcFboSize.x, srcFboSize.y,
            0, 0, _swapFbo->size.x, _swapFbo->size.y,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
    });
    
    // Blit to the onscreen window
    auto destWindowSize = qApp->getDeviceSize();
    glBlitFramebuffer(
        0, 0, srcFboSize.x, srcFboSize.y,
        0, 0, destWindowSize.width(), destWindowSize.height(),
        GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // Submit the frame to the Oculus SDK for timewarp and distortion
    for_each_eye([&](ovrEyeType eye) {
        _sceneLayer.RenderPose[eye] = _eyeRenderPoses[eye];
    });
    auto header = &_sceneLayer.Header;
    ovrResult res = ovrHmd_SubmitFrame(_ovrHmd, _frameIndex, nullptr, &header, 1);
    Q_ASSERT(OVR_SUCCESS(res));
    _swapFbo->Increment();
#else 
    GLsync syncObject = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glFlush();


    _outputWindow->makeCurrent();
    // force the compositing context to wait for the texture 
    // rendering to complete before it starts the distortion rendering,
    // but without triggering a CPU/GPU synchronization
    glWaitSync(syncObject, 0, GL_TIMEOUT_IGNORED);
    glDeleteSync(syncObject);

    GLuint textureId = gpu::GLBackend::getTextureID(finalFbo->getRenderBuffer(0));
    for_each_eye([&](ovrEyeType eye) {
        ovrGLTexture & glEyeTexture = reinterpret_cast<ovrGLTexture&>(_eyeTextures[eye]);
        glEyeTexture.OGL.TexId = textureId;

    });

    // restore our normal viewport
    ovrHmd_EndFrame(_ovrHmd, _eyeRenderPoses, _eyeTextures);
    glCanvas->makeCurrent();
#endif


    // in order to account account for changes in the pick ray caused by head movement
    // we need to force a mouse move event on every frame (perhaps we could change this 
    // to based on the head moving a minimum distance from the last position in which we 
    // sent?)
    {
        QMouseEvent mouseEvent(QEvent::MouseMove, glCanvas->mapFromGlobal(QCursor::pos()),
            Qt::NoButton, Qt::NoButton, 0);
        qApp->mouseMoveEvent(&mouseEvent, 0);
    }



}

//Tries to reconnect to the sensors
void OculusManager::reset() {
    if (_isConnected) {
        ovrHmd_RecenterPose(_ovrHmd);
    }
}

glm::vec3 OculusManager::getRelativePosition() {
    ovrTrackingState trackingState = ovrHmd_GetTrackingState(_ovrHmd, ovr_GetTimeInSeconds());
    return toGlm(trackingState.HeadPose.ThePose.Position);
}

glm::quat OculusManager::getOrientation() {
    ovrTrackingState trackingState = ovrHmd_GetTrackingState(_ovrHmd, ovr_GetTimeInSeconds());
    return toGlm(trackingState.HeadPose.ThePose.Orientation);
}

QSize OculusManager::getRenderTargetSize() {
    QSize rv;
    rv.setWidth(_renderTargetSize.w);
    rv.setHeight(_renderTargetSize.h);
    return rv;
}

void OculusManager::overrideOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearVal,
        float& farVal, glm::vec4& nearClipPlane, glm::vec4& farClipPlane) {
    if (_activeEye != ovrEye_Count) {
        const ovrFovPort& port = _eyeFov[_activeEye];
        right = nearVal * port.RightTan;
        left = -nearVal * port.LeftTan;
        top = nearVal * port.UpTan;
        bottom = -nearVal * port.DownTan;
    }
}

int OculusManager::getHMDScreen() {
#ifdef Q_OS_WIN
    return -1;
#else
    int hmdScreenIndex = -1; // unknown
    // TODO: it might be smarter to handle multiple HMDs connected in this case. but for now,
    // we will simply assume the initialization code that set up _ovrHmd picked the best hmd

    if (_ovrHmd) {
        QString productNameFromOVR = _ovrHmd->ProductName;

        int hmdWidth = _ovrHmd->Resolution.w;
        int hmdHeight = _ovrHmd->Resolution.h;
        int hmdAtX = _ovrHmd->WindowsPos.x;
        int hmdAtY = _ovrHmd->WindowsPos.y;

        // we will score the likelihood that each screen is a match based on the following
        // rubrik of potential matching features
        const int EXACT_NAME_MATCH = 100;
        const int SIMILAR_NAMES = 10;
        const int EXACT_LOCATION_MATCH = 50;
        const int EXACT_RESOLUTION_MATCH = 25;

        int bestMatchScore = 0;

        // look at the display list and see if we can find the best match
        QDesktopWidget* desktop = QApplication::desktop();
        int screenNumber = 0;
        foreach (QScreen* screen, QGuiApplication::screens()) {
            QString screenName = screen->name();
            QRect screenRect = desktop->screenGeometry(screenNumber);

            int screenScore = 0;
            if (screenName == productNameFromOVR) {
                screenScore += EXACT_NAME_MATCH;
            }
            if (similarStrings(screenName, productNameFromOVR)) {
                screenScore += SIMILAR_NAMES;
            }
            if (hmdWidth == screenRect.width() && hmdHeight == screenRect.height()) {
                screenScore += EXACT_RESOLUTION_MATCH;
            }
            if (hmdAtX == screenRect.x() && hmdAtY == screenRect.y()) {
                screenScore += EXACT_LOCATION_MATCH;
            }
            if (screenScore > bestMatchScore) {
                bestMatchScore = screenScore;
                hmdScreenIndex = screenNumber;
            }

            screenNumber++;
        }
    }
    return hmdScreenIndex;
#endif
}

mat4 OculusManager::getEyeProjection(int eye) {
    return _eyeProjection[eye];
}

mat4 OculusManager::getEyePose(int eye) {
    return toGlm(_eyeRenderPoses[eye]);
}

mat4 OculusManager::getHeadPose() {
    ovrTrackingState ts = ovrHmd_GetTrackingState(_ovrHmd, ovr_GetTimeInSeconds());
    return toGlm(ts.HeadPose.ThePose);
}
