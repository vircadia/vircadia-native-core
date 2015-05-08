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

#include "InterfaceConfig.h"

#include "OculusManager.h"
#include "ui/overlays/Text3DOverlay.h"

#include <QDesktopWidget>
#include <QGuiApplication>
#include <QScreen>	
#include <QOpenGLTimerQuery>

#include <glm/glm.hpp>

#include <avatar/AvatarManager.h>
#include <avatar/MyAvatar.h>
#include <GlowEffect.h>
#include <PathUtils.h>
#include <SharedUtil.h>
#include <UserActivityLogger.h>

#include <OVR_CAPI_GL.h>

#include "InterfaceLogging.h"
#include "Application.h"

#include <gpu/GLBackend.h>

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

#ifdef OVR_CLIENT_DISTORTION
ProgramObject OculusManager::_program;
int OculusManager::_textureLocation;
int OculusManager::_eyeToSourceUVScaleLocation;
int OculusManager::_eyeToSourceUVOffsetLocation;
int OculusManager::_eyeRotationStartLocation;
int OculusManager::_eyeRotationEndLocation;
int OculusManager::_positionAttributeLocation;
int OculusManager::_colorAttributeLocation;
int OculusManager::_texCoord0AttributeLocation;
int OculusManager::_texCoord1AttributeLocation;
int OculusManager::_texCoord2AttributeLocation;
ovrVector2f OculusManager::_UVScaleOffset[ovrEye_Count][2];
GLuint OculusManager::_vertices[ovrEye_Count] = { 0, 0 };
GLuint OculusManager::_indices[ovrEye_Count] = { 0, 0 };
GLsizei OculusManager::_meshSize[ovrEye_Count] = { 0, 0 };
ovrFrameTiming OculusManager::_hmdFrameTiming;
bool OculusManager::_programInitialized = false;
#endif

ovrTexture OculusManager::_eyeTextures[ovrEye_Count];
bool OculusManager::_isConnected = false;
ovrHmd OculusManager::_ovrHmd;
ovrFovPort OculusManager::_eyeFov[ovrEye_Count];
ovrVector3f OculusManager::_eyeOffset[ovrEye_Count];
ovrEyeRenderDesc OculusManager::_eyeRenderDesc[ovrEye_Count];
ovrSizei OculusManager::_renderTargetSize;
glm::mat4 OculusManager::_eyeProjection[ovrEye_Count];
unsigned int OculusManager::_frameIndex = 0;
bool OculusManager::_frameTimingActive = false;
Camera* OculusManager::_camera = NULL;
ovrEyeType OculusManager::_activeEye = ovrEye_Count;
bool OculusManager::_hswDismissed = false;

float OculusManager::CALIBRATION_DELTA_MINIMUM_LENGTH = 0.02f;
float OculusManager::CALIBRATION_DELTA_MINIMUM_ANGLE = 5.0f * RADIANS_PER_DEGREE;
float OculusManager::CALIBRATION_ZERO_MAXIMUM_LENGTH = 0.01f;
float OculusManager::CALIBRATION_ZERO_MAXIMUM_ANGLE = 2.0f * RADIANS_PER_DEGREE;
quint64 OculusManager::CALIBRATION_ZERO_HOLD_TIME = 3000000; // usec
float OculusManager::CALIBRATION_MESSAGE_DISTANCE = 2.5f;
OculusManager::CalibrationState OculusManager::_calibrationState;
glm::vec3 OculusManager::_calibrationPosition;
glm::quat OculusManager::_calibrationOrientation;
quint64 OculusManager::_calibrationStartTime;
int OculusManager::_calibrationMessage = 0;
glm::vec3 OculusManager::_eyePositions[ovrEye_Count];
// TODO expose this as a developer toggle
bool OculusManager::_eyePerFrameMode = false;
ovrEyeType OculusManager::_lastEyeRendered = ovrEye_Count;
ovrSizei OculusManager::_recommendedTexSize = { 0, 0 };
float OculusManager::_offscreenRenderScale = 1.0;


void OculusManager::initSdk() {
    ovr_Initialize();
    _ovrHmd = ovrHmd_Create(0);
    if (!_ovrHmd) {
      _ovrHmd = ovrHmd_CreateDebug(ovrHmd_DK2);
    }
}

void OculusManager::shutdownSdk() {
    if (_ovrHmd) {
        ovrHmd_Destroy(_ovrHmd);
        _ovrHmd = nullptr;
        ovr_Shutdown();
    }
}

void OculusManager::init() {
#ifdef OVR_DIRECT_MODE
	initSdk();
#endif
}

void OculusManager::deinit() {
#ifdef OVR_DIRECT_MODE
    shutdownSdk();
#endif
}

void OculusManager::connect() {
#ifndef OVR_DIRECT_MODE
	initSdk();
#endif
    _calibrationState = UNCALIBRATED;
    qCDebug(interfaceapp) << "Oculus SDK" << OVR_VERSION_STRING;
    if (_ovrHmd) {
        if (!_isConnected) {
            UserActivityLogger::getInstance().connectedDevice("hmd", "oculus");
        }
        _isConnected = true;

        for_each_eye([&](ovrEyeType eye) {
            _eyeFov[eye] = _ovrHmd->DefaultEyeFov[eye];
        });

        ovrGLConfig cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
        cfg.OGL.Header.BackBufferSize = _ovrHmd->Resolution;
        cfg.OGL.Header.Multisample = 1;

        int distortionCaps = 0
            | ovrDistortionCap_Vignette
            | ovrDistortionCap_Overdrive
            | ovrDistortionCap_TimeWarp;

        int configResult = ovrHmd_ConfigureRendering(_ovrHmd, &cfg.Config,
            distortionCaps, _eyeFov, _eyeRenderDesc);
        assert(configResult);


        _recommendedTexSize = ovrHmd_GetFovTextureSize(_ovrHmd, ovrEye_Left, _eyeFov[ovrEye_Left], 1.0f);
        _renderTargetSize = { _recommendedTexSize.w * 2, _recommendedTexSize.h };
        for_each_eye([&](ovrEyeType eye) {
            //Get texture size
            _eyeTextures[eye].Header.API = ovrRenderAPI_OpenGL;
            _eyeTextures[eye].Header.TextureSize = _renderTargetSize;
            _eyeTextures[eye].Header.RenderViewport.Pos = { 0, 0 };
        });
        _eyeTextures[ovrEye_Right].Header.RenderViewport.Pos.x = _recommendedTexSize.w;

        ovrHmd_SetEnabledCaps(_ovrHmd, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);

        ovrHmd_ConfigureTracking(_ovrHmd, ovrTrackingCap_Orientation | ovrTrackingCap_Position |
                                 ovrTrackingCap_MagYawCorrection,
                                 ovrTrackingCap_Orientation);

        if (!_camera) {
            _camera = new Camera;
            configureCamera(*_camera, 0, 0); // no need to use screen dimensions; they're ignored
        }
#ifdef OVR_CLIENT_DISTORTION
        if (!_programInitialized) {
            // Shader program
            _programInitialized = true;
            _program.addShaderFromSourceFile(QGLShader::Vertex, PathUtils::resourcesPath() + "shaders/oculus.vert");
            _program.addShaderFromSourceFile(QGLShader::Fragment, PathUtils::resourcesPath() + "shaders/oculus.frag");
            _program.link();

            // Uniforms
            _textureLocation = _program.uniformLocation("texture");
            _eyeToSourceUVScaleLocation = _program.uniformLocation("EyeToSourceUVScale");
            _eyeToSourceUVOffsetLocation = _program.uniformLocation("EyeToSourceUVOffset");
            _eyeRotationStartLocation = _program.uniformLocation("EyeRotationStart");
            _eyeRotationEndLocation = _program.uniformLocation("EyeRotationEnd");

            // Attributes
            _positionAttributeLocation = _program.attributeLocation("position");
            _colorAttributeLocation = _program.attributeLocation("color");
            _texCoord0AttributeLocation = _program.attributeLocation("texCoord0");
            _texCoord1AttributeLocation = _program.attributeLocation("texCoord1");
            _texCoord2AttributeLocation = _program.attributeLocation("texCoord2");
        }

        //Generate the distortion VBOs
        generateDistortionMesh();
#endif
    } else {
        _isConnected = false;
        
        // we're definitely not in "VR mode" so tell the menu that
        Menu::getInstance()->getActionForOption(MenuOption::EnableVRMode)->setChecked(false);
    }
}

//Disconnects and deallocates the OR
void OculusManager::disconnect() {
    if (_isConnected) {
        _isConnected = false;
        // Prepare to potentially have to dismiss the HSW again
        // if the user re-enables VR
        _hswDismissed = false;
#ifndef OVR_DIRECT_MODE
        shutdownSdk();
#endif

#ifdef OVR_CLIENT_DISTORTION
        //Free the distortion mesh data
        for (int i = 0; i < ovrEye_Count; i++) {
            if (_vertices[i] != 0) {
                glDeleteBuffers(1, &(_vertices[i]));
                _vertices[i] = 0;
            }
            if (_indices[i] != 0) {
                glDeleteBuffers(1, &(_indices[i]));
                _indices[i] = 0;
            }
        }
#endif
    }
}

void OculusManager::positionCalibrationBillboard(Text3DOverlay* billboard) {
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    glm::quat headOrientation = myAvatar->getHeadOrientation();
    headOrientation.x = 0;
    headOrientation.z = 0;
    glm::normalize(headOrientation);
    billboard->setPosition(myAvatar->getHeadPosition()
        + headOrientation * glm::vec3(0.0f, 0.0f, -CALIBRATION_MESSAGE_DISTANCE));
    billboard->setRotation(headOrientation);
}

void OculusManager::calibrate(glm::vec3 position, glm::quat orientation) {
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

#ifdef OVR_CLIENT_DISTORTION
void OculusManager::generateDistortionMesh() {

    //Check if we already have the distortion mesh
    if (_vertices[0] != 0) {
        printf("WARNING: Tried to generate Oculus distortion mesh twice without freeing the VBOs.");
        return;
    }

    for (int eyeNum = 0; eyeNum < ovrEye_Count; eyeNum++) {
        // Allocate and generate distortion mesh vertices
        ovrDistortionMesh meshData;
        ovrHmd_CreateDistortionMesh(_ovrHmd, _eyeRenderDesc[eyeNum].Eye, _eyeRenderDesc[eyeNum].Fov, _ovrHmd->DistortionCaps, &meshData);

        // Parse the vertex data and create a render ready vertex buffer
        DistortionVertex* pVBVerts = new DistortionVertex[meshData.VertexCount];
        _meshSize[eyeNum] = meshData.IndexCount;

        // Convert the oculus vertex data to the DistortionVertex format.
        DistortionVertex* v = pVBVerts;
        ovrDistortionVertex* ov = meshData.pVertexData;
        for (unsigned int vertNum = 0; vertNum < meshData.VertexCount; vertNum++) {
            v->pos.x = ov->ScreenPosNDC.x;
            v->pos.y = ov->ScreenPosNDC.y;
            v->texR.x = ov->TanEyeAnglesR.x;
            v->texR.y = ov->TanEyeAnglesR.y;
            v->texG.x = ov->TanEyeAnglesG.x;
            v->texG.y = ov->TanEyeAnglesG.y;
            v->texB.x = ov->TanEyeAnglesB.x;
            v->texB.y = ov->TanEyeAnglesB.y;
            v->color.r = v->color.g = v->color.b = (GLubyte)(ov->VignetteFactor * 255.99f);
            v->color.a = (GLubyte)(ov->TimeWarpFactor * 255.99f);
            v++; 
            ov++;
        }

        //vertices
        glGenBuffers(1, &(_vertices[eyeNum]));
        glBindBuffer(GL_ARRAY_BUFFER, _vertices[eyeNum]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(DistortionVertex) * meshData.VertexCount, pVBVerts, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //indices
        glGenBuffers(1, &(_indices[eyeNum]));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indices[eyeNum]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * meshData.IndexCount, meshData.pIndexData, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        
        //Now that we have the VBOs we can get rid of the mesh data
        delete [] pVBVerts;
        ovrHmd_DestroyDistortionMesh(&meshData);
    }

}
#endif

bool OculusManager::isConnected() {
    return _isConnected && Menu::getInstance()->isOptionChecked(MenuOption::EnableVRMode);
}

//Begins the frame timing for oculus prediction purposes
void OculusManager::beginFrameTiming() {
    if (_frameTimingActive) {
        printf("WARNING: Called OculusManager::beginFrameTiming() twice in a row, need to call OculusManager::endFrameTiming().");
    }

#ifdef OVR_CLIENT_DISTORTION
    _hmdFrameTiming = ovrHmd_BeginFrameTiming(_ovrHmd, _frameIndex);
#endif
   _frameTimingActive = true;
}

bool OculusManager::allowSwap() {
    return false;
}

//Ends frame timing
void OculusManager::endFrameTiming() {
#ifdef OVR_CLIENT_DISTORTION
    ovrHmd_EndFrameTiming(_ovrHmd);
#endif
    _frameIndex++;
    _frameTimingActive = false;
}

//Sets the camera FoV and aspect ratio
void OculusManager::configureCamera(Camera& camera, int screenWidth, int screenHeight) {
    camera.setAspectRatio(_renderTargetSize.w * 0.5f / _renderTargetSize.h);
    camera.setFieldOfView(atan(_eyeFov[0].UpTan) * DEGREES_PER_RADIAN * 2.0f);
}

//Displays everything for the oculus, frame timing must be active
void OculusManager::display(QGLWidget * glCanvas, const glm::quat &bodyOrientation, const glm::vec3 &position, Camera& whichCamera) {

#ifdef DEBUG
    // Ensure the frame counter always increments by exactly 1
    static int oldFrameIndex = -1;
    assert(oldFrameIndex == -1 || (unsigned int)oldFrameIndex == _frameIndex - 1);
    oldFrameIndex = _frameIndex;
#endif

    // Every so often do some additional timing calculations and debug output
    bool debugFrame = 0 == _frameIndex % 400;
    
#if 0
    // Try to measure the amount of time taken to do the distortion
    // (does not seem to work on OSX with SDK based distortion)
    // FIXME can't use a static object here, because it will cause a crash when the
    // query attempts deconstruct after the GL context is gone.
    static bool timerActive = false;
    static QOpenGLTimerQuery timerQuery;
    if (!timerQuery.isCreated()) {
        timerQuery.create();
    }
    
    if (timerActive && timerQuery.isResultAvailable()) {
        auto result = timerQuery.waitForResult();
        if (result) { qCDebug(interfaceapp) << "Distortion took "  << result << "ns"; };
        timerActive = false;
    }
#endif
    
#ifdef OVR_DIRECT_MODE
    static bool attached = false;
    if (!attached) {
        attached = true;
        void * nativeWindowHandle = (void*)(size_t)glCanvas->effectiveWinId();
        if (nullptr != nativeWindowHandle) {
            ovrHmd_AttachToWindow(_ovrHmd, nativeWindowHandle, nullptr, nullptr);
        }
    }
#endif
    
#ifndef OVR_CLIENT_DISTORTION
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

    ApplicationOverlay& applicationOverlay = Application::getInstance()->getApplicationOverlay();

    // We only need to render the overlays to a texture once, then we just render the texture on the hemisphere
    // PrioVR will only work if renderOverlay is called, calibration is connected to Application::renderingOverlay() 
    applicationOverlay.renderOverlay();
   
    //Bind our framebuffer object. If we are rendering the glow effect, we let the glow effect shader take care of it
    if (Menu::getInstance()->isOptionChecked(MenuOption::EnableGlowEffect)) {
        DependencyManager::get<GlowEffect>()->prepare();
    } else {
        auto primaryFBO = DependencyManager::get<TextureCache>()->getPrimaryFramebuffer();
        glBindFramebuffer(GL_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(primaryFBO));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }


    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
  
    glm::quat orientation;
    glm::vec3 trackerPosition;
    
    ovrTrackingState ts = ovrHmd_GetTrackingState(_ovrHmd, ovr_GetTimeInSeconds());
    ovrVector3f ovrHeadPosition = ts.HeadPose.ThePose.Position;
    
    trackerPosition = glm::vec3(ovrHeadPosition.x, ovrHeadPosition.y, ovrHeadPosition.z);

    if (_calibrationState != CALIBRATED) {
        ovrQuatf ovrHeadOrientation = ts.HeadPose.ThePose.Orientation;
        orientation = glm::quat(ovrHeadOrientation.w, ovrHeadOrientation.x, ovrHeadOrientation.y, ovrHeadOrientation.z);
        calibrate(trackerPosition, orientation);
    }

    trackerPosition = bodyOrientation * trackerPosition;
    static ovrVector3f eyeOffsets[2] = { { 0, 0, 0 }, { 0, 0, 0 } };
    ovrPosef eyePoses[ovrEye_Count];
    ovrHmd_GetEyePoses(_ovrHmd, _frameIndex, eyeOffsets, eyePoses, nullptr);
    ovrHmd_BeginFrame(_ovrHmd, _frameIndex);
    static ovrPosef eyeRenderPose[ovrEye_Count];
    //Render each eye into an fbo
    for_each_eye(_ovrHmd, [&](ovrEyeType eye){
        // If we're in eye-per-frame mode, only render one eye
        // per call to display, and allow timewarp to correct for
        // the other eye.  Poor man's perf improvement
        if (_eyePerFrameMode && eye == _lastEyeRendered) {
            return;
        }
        _lastEyeRendered = _activeEye = eye;
        eyeRenderPose[eye] = eyePoses[eye];
        // Set the camera rotation for this eye
        orientation.x = eyeRenderPose[eye].Orientation.x;
        orientation.y = eyeRenderPose[eye].Orientation.y;
        orientation.z = eyeRenderPose[eye].Orientation.z;
        orientation.w = eyeRenderPose[eye].Orientation.w;

        // Update the application camera with the latest HMD position
        whichCamera.setHmdPosition(trackerPosition);
        whichCamera.setHmdRotation(orientation);

        // Update our camera to what the application camera is doing
        _camera->setRotation(whichCamera.getRotation());
        _camera->setPosition(whichCamera.getPosition());

        //  Store the latest left and right eye render locations for things that need to know
        glm::vec3 thisEyePosition = position + trackerPosition +
            (bodyOrientation * glm::quat(orientation.x, orientation.y, orientation.z, orientation.w) *
            glm::vec3(_eyeRenderDesc[eye].HmdToEyeViewOffset.x, _eyeRenderDesc[eye].HmdToEyeViewOffset.y, _eyeRenderDesc[eye].HmdToEyeViewOffset.z));

        _eyePositions[eye] = thisEyePosition;
        _camera->update(1.0f / Application::getInstance()->getFps());

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        const ovrFovPort& port = _eyeFov[_activeEye];
        float nearClip = whichCamera.getNearClip(), farClip = whichCamera.getFarClip();
        glFrustum(-nearClip * port.LeftTan, nearClip * port.RightTan, -nearClip * port.DownTan,
            nearClip * port.UpTan, nearClip, farClip);

        ovrRecti & vp = _eyeTextures[eye].Header.RenderViewport;
        vp.Size.h = _recommendedTexSize.h * _offscreenRenderScale;
        vp.Size.w = _recommendedTexSize.w * _offscreenRenderScale;
        
        glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // HACK: instead of passing the stereo eye offset directly in the matrix, pass it in the camera offset
        //glTranslatef(_eyeRenderDesc[eye].ViewAdjust.x, _eyeRenderDesc[eye].ViewAdjust.y, _eyeRenderDesc[eye].ViewAdjust.z);

        _camera->setEyeOffsetPosition(glm::vec3(-_eyeRenderDesc[eye].HmdToEyeViewOffset.x, -_eyeRenderDesc[eye].HmdToEyeViewOffset.y, -_eyeRenderDesc[eye].HmdToEyeViewOffset.z));
        Application::getInstance()->displaySide(*_camera, false, RenderArgs::MONO);

        applicationOverlay.displayOverlayTextureHmd(*_camera);
    });
    _activeEye = ovrEye_Count;

    glPopMatrix();

    gpu::FramebufferPointer finalFbo;
    //Bind the output texture from the glow shader. If glow effect is disabled, we just grab the texture
    if (Menu::getInstance()->isOptionChecked(MenuOption::EnableGlowEffect)) {
        //Full texture viewport for glow effect
        glViewport(0, 0, _renderTargetSize.w, _renderTargetSize.h);
        finalFbo = DependencyManager::get<GlowEffect>()->render(true);
    } else {
        finalFbo = DependencyManager::get<TextureCache>()->getPrimaryFramebuffer(); 
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    // restore our normal viewport
    auto deviceSize = qApp->getDeviceSize();
    glViewport(0, 0, deviceSize.width(), deviceSize.height());

#if 0
    if (debugFrame && !timerActive) {
        timerQuery.begin();
    }
#endif

#ifdef OVR_CLIENT_DISTORTION
    
    //Wait till time-warp to reduce latency
    ovr_WaitTillTime(_hmdFrameTiming.TimewarpPointSeconds);

    //Clear the color buffer to ensure that there isnt any residual color
    //Left over from when OR was not connected.
    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(finalFbo->getRenderBuffer(0)));
    
    //Renders the distorted mesh onto the screen
    renderDistortionMesh(eyeRenderPose);

    glBindTexture(GL_TEXTURE_2D, 0);
    glCanvas->swapBuffers();

#else

    for_each_eye([&](ovrEyeType eye) {
        ovrGLTexture & glEyeTexture = reinterpret_cast<ovrGLTexture&>(_eyeTextures[eye]);
        glEyeTexture.OGL.TexId = finalFbo->texture();
        
    });

    ovrHmd_EndFrame(_ovrHmd, eyeRenderPose, _eyeTextures);

#endif

#if 0
    if (debugFrame && !timerActive) {
        timerQuery.end();
        timerActive = true;
    }
#endif

    // No DK2, no message.
    {
        float latencies[5] = {};
        if (debugFrame && ovrHmd_GetFloatArray(_ovrHmd, "DK2Latency", latencies, 5) == 5)
        {
            bool nonZero = false;
            for (int i = 0; i < 5; ++i)
            {
                nonZero |= (latencies[i] != 0.f);
            }

            if (nonZero)
            {
                qCDebug(interfaceapp) << QString().sprintf("M2P Latency: Ren: %4.2fms TWrp: %4.2fms PostPresent: %4.2fms Err: %4.2fms %4.2fms",
                                             latencies[0], latencies[1], latencies[2], latencies[3], latencies[4]);
            }
        }
    }
    
}

#ifdef OVR_CLIENT_DISTORTION
void OculusManager::renderDistortionMesh(ovrPosef eyeRenderPose[ovrEye_Count]) {

    glLoadIdentity();
    auto deviceSize = qApp->getDeviceSize();
    glOrtho(0, deviceSize.width(), 0, deviceSize.height(), -1.0, 1.0);

    glDisable(GL_DEPTH_TEST);

    glDisable(GL_BLEND);
    _program.bind();
    _program.setUniformValue(_textureLocation, 0);

    _program.enableAttributeArray(_positionAttributeLocation);
    _program.enableAttributeArray(_colorAttributeLocation);
    _program.enableAttributeArray(_texCoord0AttributeLocation);
    _program.enableAttributeArray(_texCoord1AttributeLocation);
    _program.enableAttributeArray(_texCoord2AttributeLocation);

    //Render the distortion meshes for each eye
    for (int eyeNum = 0; eyeNum < ovrEye_Count; eyeNum++) {
        
        ovrHmd_GetRenderScaleAndOffset(_eyeRenderDesc[eyeNum].Fov, _renderTargetSize, _eyeTextures[eyeNum].Header.RenderViewport,
                                       _UVScaleOffset[eyeNum]);

        GLfloat uvScale[2] = { _UVScaleOffset[eyeNum][0].x, _UVScaleOffset[eyeNum][0].y };
        _program.setUniformValueArray(_eyeToSourceUVScaleLocation, uvScale, 1, 2);
        GLfloat uvOffset[2] = { _UVScaleOffset[eyeNum][1].x, 1.0f - _UVScaleOffset[eyeNum][1].y };
        _program.setUniformValueArray(_eyeToSourceUVOffsetLocation, uvOffset, 1, 2);

        ovrMatrix4f timeWarpMatrices[2];
        glm::mat4 transposeMatrices[2];
        //Grabs the timewarp matrices to be used in the shader
        ovrHmd_GetEyeTimewarpMatrices(_ovrHmd, (ovrEyeType)eyeNum, eyeRenderPose[eyeNum], timeWarpMatrices);
        //Have to transpose the matrices before using them
        transposeMatrices[0] = glm::transpose(toGlm(timeWarpMatrices[0]));
        transposeMatrices[1] = glm::transpose(toGlm(timeWarpMatrices[1]));

        glUniformMatrix4fv(_eyeRotationStartLocation, 1, GL_FALSE, (GLfloat *)&transposeMatrices[0][0][0]);
        glUniformMatrix4fv(_eyeRotationEndLocation, 1, GL_FALSE, (GLfloat *)&transposeMatrices[1][0][0]);

        glBindBuffer(GL_ARRAY_BUFFER, _vertices[eyeNum]);

        //Set vertex attribute pointers
        glVertexAttribPointer(_positionAttributeLocation, 2, GL_FLOAT, GL_FALSE, sizeof(DistortionVertex), (void *)0);
        glVertexAttribPointer(_texCoord0AttributeLocation, 2, GL_FLOAT, GL_FALSE, sizeof(DistortionVertex), (void *)8);
        glVertexAttribPointer(_texCoord1AttributeLocation, 2, GL_FLOAT, GL_FALSE, sizeof(DistortionVertex), (void *)16);
        glVertexAttribPointer(_texCoord2AttributeLocation, 2, GL_FLOAT, GL_FALSE, sizeof(DistortionVertex), (void *)24);
        glVertexAttribPointer(_colorAttributeLocation, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(DistortionVertex), (void *)32);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indices[eyeNum]);
        glDrawElements(GL_TRIANGLES, _meshSize[eyeNum], GL_UNSIGNED_SHORT, 0);
    }

    _program.disableAttributeArray(_positionAttributeLocation);
    _program.disableAttributeArray(_colorAttributeLocation);
    _program.disableAttributeArray(_texCoord0AttributeLocation);
    _program.disableAttributeArray(_texCoord1AttributeLocation);
    _program.disableAttributeArray(_texCoord2AttributeLocation);

    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    _program.release();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
#endif

//Tries to reconnect to the sensors
void OculusManager::reset() {
    if (_isConnected) {
        ovrHmd_RecenterPose(_ovrHmd);
    }
}

//Gets the current predicted angles from the oculus sensors
void OculusManager::getEulerAngles(float& yaw, float& pitch, float& roll) {
    ovrTrackingState ts = ovrHmd_GetTrackingState(_ovrHmd, ovr_GetTimeInSeconds());
    if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)) {
        glm::vec3 euler = glm::eulerAngles(toGlm(ts.HeadPose.ThePose.Orientation));
        yaw = euler.y;
        pitch = euler.x;
        roll = euler.z;
    } else {
        yaw = 0.0f;
        pitch = 0.0f;
        roll = 0.0f;
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

//Used to set the size of the glow framebuffers
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
}

