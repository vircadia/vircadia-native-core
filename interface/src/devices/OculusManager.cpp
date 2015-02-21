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
#include <QOpenGLFramebufferObject>
#include <QScreen>

#include <glm/glm.hpp>

#include <avatar/AvatarManager.h>
#include <avatar/MyAvatar.h>
#include <GlowEffect.h>
#include <PathUtils.h>
#include <SharedUtil.h>
#include <UserActivityLogger.h>
#include "Application.h"

using namespace OVR;

template <typename Function>
void for_each_eye(Function function) {
    for (ovrEyeType eye = ovrEyeType::ovrEye_Left;
        eye < ovrEyeType::ovrEye_Count;
        eye = static_cast<ovrEyeType>(eye + 1)) {
        function(eye);
    }
}

ovrHmd OculusManager::_ovrHmd;
ovrFovPort OculusManager::_eyeFov[ovrEye_Count];
ovrVector3f OculusManager::_eyeOffset[ovrEye_Count];
ovrSizei OculusManager::_renderTargetSize;
glm::mat4 OculusManager::_eyeProjection[ovrEye_Count];
unsigned int OculusManager::_frameIndex = 0;
bool OculusManager::_frameTimingActive = false;
bool OculusManager::_programInitialized = false;
Camera* OculusManager::_camera = NULL;
int OculusManager::_activeEyeIndex = -1;

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
int OculusManager::_calibrationMessage = NULL;
glm::vec3 OculusManager::_eyePositions[ovrEye_Count];

void OculusManager::init() {
    ovr_Initialize();
    _ovrHmd = ovrHmd_Create(0);
    if (_ovrHmd) {
        // FIXME Not sure this is effective prior to starting rendering
        ovrHmd_SetEnabledCaps(_ovrHmd, ovrHmdCap_LowPersistence);
        ovrHmd_ConfigureTracking(_ovrHmd, ovrTrackingCap_Orientation | ovrTrackingCap_Position |
            ovrTrackingCap_MagYawCorrection,
            ovrTrackingCap_Orientation);

        for_each_eye([&](ovrEyeType eye) {
            _eyeFov[eye] = _ovrHmd->DefaultEyeFov[eye];
        });

        if (!_camera) {
            _camera = new Camera;
            configureCamera(*_camera, 0, 0); // no need to use screen dimensions; they're ignored
        }
    } else {
        // we're definitely not in "VR mode" so tell the menu that
        Menu::getInstance()->getActionForOption(MenuOption::EnableVRMode)->setChecked(false);
    }
}

// FIXME store like the others
ovrGLTexture _eyeTextures[ovrEye_Count];

void OculusManager::connect() {
    _calibrationState = UNCALIBRATED;
    qDebug() << "Oculus SDK" << OVR_VERSION_STRING;
    if (_ovrHmd) {
        UserActivityLogger::getInstance().connectedDevice("hmd", "oculus");

        MainWindow* applicationWindow = Application::getInstance()->getWindow();

        ovrGLConfig cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
        cfg.OGL.Header.BackBufferSize = _ovrHmd->Resolution;
        cfg.OGL.Header.Multisample = 1;

        int distortionCaps = 0
            | ovrDistortionCap_Chromatic
            | ovrDistortionCap_Vignette
            | ovrDistortionCap_Overdrive
            | ovrDistortionCap_TimeWarp;

        ovrEyeRenderDesc eyeRenderDescs[2];
        int configResult = ovrHmd_ConfigureRendering(_ovrHmd, &cfg.Config,
            distortionCaps, _eyeFov, eyeRenderDescs);
        assert(configResult);

        for_each_eye([&](ovrEyeType eye) {
            _eyeFov[eye] = _ovrHmd->DefaultEyeFov[eye];
        });

        _renderTargetSize = { 0, 0 };
        memset(_eyeTextures, 0, sizeof(_eyeTextures));
        for_each_eye([&](ovrEyeType eye) {
            //Get texture size
            ovrSizei recommendedTexSize = ovrHmd_GetFovTextureSize(_ovrHmd, eye, _eyeFov[eye], 1.0f);
            auto & eyeTexture = _eyeTextures[eye].Texture.Header;
            eyeTexture.API = ovrRenderAPI_OpenGL;
            eyeTexture.RenderViewport.Size = recommendedTexSize;
            eyeTexture.RenderViewport.Pos = { _renderTargetSize.w, 0 };

            _renderTargetSize.h = std::max(_renderTargetSize.h, recommendedTexSize.h);
            _renderTargetSize.w += recommendedTexSize.w;
            const ovrEyeRenderDesc & erd = eyeRenderDescs[eye];
            ovrMatrix4f ovrPerspectiveProjection = ovrMatrix4f_Projection(erd.Fov, 0.01f, 100000.0f, true);
            _eyeProjection[eye] = toGlm(ovrPerspectiveProjection);
            _eyeOffset[eye] = erd.HmdToEyeViewOffset;
        });

        for_each_eye([&](ovrEyeType eye) {
            ovrGLTexture & eyeTexture = _eyeTextures[eye];
            eyeTexture.Texture.Header.TextureSize = _renderTargetSize;
        });
    }
}

//Disconnects and deallocates the OR
void OculusManager::disconnect() {
    if (_ovrHmd) {
        ovrHmd_Destroy(_ovrHmd);
        _ovrHmd = nullptr;
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
                    qDebug() << "Hold still to calibrate HMD";

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
                    qDebug() << "HMD calibrated";
                    Application::getInstance()->getOverlays().deleteOverlay(_calibrationMessage);
                    _calibrationMessage = NULL;
                    Application::getInstance()->resetSensors();
                } else {
                    quint64 quarterSeconds = (usecTimestampNow() - _calibrationStartTime) / 250000;
                    if (quarterSeconds + 1 > progressMessage.length()) {
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
        qDebug() << "Abandoned HMD calibration";
        Application::getInstance()->getOverlays().deleteOverlay(_calibrationMessage);
        _calibrationMessage = NULL;
    }
}


bool OculusManager::isConnected() {
    return _ovrHmd && Menu::getInstance()->isOptionChecked(MenuOption::EnableVRMode);
}

//Sets the camera FoV and aspect ratio
void OculusManager::configureCamera(Camera& camera, int screenWidth, int screenHeight) {
    // FIXME the oculus projection matrix is assymetrical, this will not work.
    camera.setAspectRatio(_renderTargetSize.w * 0.5f / _renderTargetSize.h);
    camera.setFieldOfView(atan(_eyeFov[0].UpTan) * DEGREES_PER_RADIAN * 2.0f);
}

RenderArgs::RenderSide RENDER_SIDES[ovrEye_Count] = { RenderArgs::STEREO_LEFT, RenderArgs::STEREO_RIGHT };

//Displays everything for the oculus, frame timing must be active
void OculusManager::display(const glm::quat &bodyOrientation, const glm::vec3 &position, Camera& whichCamera) {

    static const glm::vec2 topLeft(-1.0f, -1.0f);
    static const glm::vec2 bottomRight(1.0f, 1.0f);
    static const glm::vec2 texCoordTopLeft(0.0f, 0.0f);
    static const glm::vec2 texCoordBottomRight(1.0f, 1.0f);

    auto glCanvas = DependencyManager::get<GLCanvas>();

    static bool attached = false;
    if (!attached) {
        attached = true;
        void * nativeWindowHandle = (void*)(size_t)glCanvas->effectiveWinId();
        if (nullptr != nativeWindowHandle) {
            ovrHmd_AttachToWindow(_ovrHmd, nativeWindowHandle, nullptr, nullptr);
        }
    }

    ovrHmd_BeginFrame(_ovrHmd, ++_frameIndex);
    ApplicationOverlay& applicationOverlay = Application::getInstance()->getApplicationOverlay();

    // We only need to render the overlays to a texture once, then we just render the texture on the hemisphere
    // PrioVR will only work if renderOverlay is called, calibration is connected to Application::renderingOverlay() 
    applicationOverlay.renderOverlay(true);
   
    //Bind our framebuffer object. If we are rendering the glow effect, we let the glow effect shader take care of it
    if (Menu::getInstance()->isOptionChecked(MenuOption::EnableGlowEffect)) {
        DependencyManager::get<GlowEffect>()->prepare();
    } else {
        DependencyManager::get<TextureCache>()->getPrimaryFramebufferObject()->bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    if (_calibrationState != CALIBRATED) {
        ovrTrackingState ts = ovrHmd_GetTrackingState(_ovrHmd, ovr_GetTimeInSeconds());
        ovrVector3f ovrHeadPosition = ts.HeadPose.ThePose.Position;
        glm::vec3 trackerPosition = glm::vec3(ovrHeadPosition.x, ovrHeadPosition.y, ovrHeadPosition.z);
        calibrate(trackerPosition, toGlm(ts.HeadPose.ThePose.Orientation));
        trackerPosition = bodyOrientation * trackerPosition;
    }

    //Render each eye into an fbo
    ovrPosef eyeRenderPose[ovrEye_Count];
    ovrHmd_GetEyePoses(_ovrHmd, 0, _eyeOffset, eyeRenderPose, nullptr);
    for_each_eye([&](int i) {
        _activeEyeIndex = i;
        ovrEyeType eye = _ovrHmd->EyeRenderOrder[_activeEyeIndex];
        const ovrPosef & pose = eyeRenderPose[eye];
        // Update the application camera with the latest HMD position
        whichCamera.setHmdPosition(bodyOrientation * toGlm(pose.Position));
        whichCamera.setHmdRotation(toGlm(pose.Orientation));

        // Update our camera to what the application camera is doing
        _camera->setRotation(whichCamera.getRotation());
        _camera->setPosition(whichCamera.getPosition());
        RenderArgs::RenderSide renderSide = RENDER_SIDES[eye];
        glm::vec3 thisEyePosition = position + toGlm(eyeRenderPose[eye].Position);
        //  Store the latest left and right eye render locations for things that need to know
        _eyePositions[eye] = thisEyePosition;
        _camera->update(1.0f / Application::getInstance()->getFps());

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        const ovrFovPort& port = _eyeFov[eye];
        float nearClip = whichCamera.getNearClip(), farClip = whichCamera.getFarClip();
        glFrustum(-nearClip * port.LeftTan, nearClip * port.RightTan, -nearClip * port.DownTan,
            nearClip * port.UpTan, nearClip, farClip);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        auto vp = _eyeTextures[eye].Texture.Header.RenderViewport;
        glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
        // HACK: instead of passing the stereo eye offset directly in the matrix, pass it in the camera offset
        //glTranslatef(_eyeRenderDesc[eye].ViewAdjust.x, _eyeRenderDesc[eye].ViewAdjust.y, _eyeRenderDesc[eye].ViewAdjust.z);
        //_camera->setEyeOffsetPosition(glm::vec3(-_eyeRenderDesc[eye].ViewAdjust.x, -_eyeRenderDesc[eye].ViewAdjust.y, -_eyeRenderDesc[eye].ViewAdjust.z));
        Application::getInstance()->displaySide(*_camera, false, RenderArgs::MONO);
        applicationOverlay.displayOverlayTextureOculus(*_camera);
    });
    _activeEyeIndex = -1;

    glPopMatrix();
    //Full texture viewport for glow effect
    glViewport(0, 0, _renderTargetSize.w, _renderTargetSize.h);

    int finalTexture = 0;
    //Bind the output texture from the glow shader. If glow effect is disabled, we just grab the texture
    if (Menu::getInstance()->isOptionChecked(MenuOption::EnableGlowEffect)) {
        QOpenGLFramebufferObject* fbo = DependencyManager::get<GlowEffect>()->render(true);
        finalTexture = fbo->texture();
    } else {
        DependencyManager::get<TextureCache>()->getPrimaryFramebufferObject()->release();
        finalTexture = DependencyManager::get<TextureCache>()->getPrimaryFramebufferObject()->texture();
    }
    for_each_eye([&](int eye) {
        _eyeTextures[eye].OGL.TexId = finalTexture;
    }); 

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    // restore our normal viewport
    glViewport(0, 0, glCanvas->getDeviceWidth(), glCanvas->getDeviceHeight());

    ovrHmd_EndFrame(_ovrHmd, eyeRenderPose, &(_eyeTextures[0].Texture));
}

//Tries to reconnect to the sensors
void OculusManager::reset() {
    if (_ovrHmd) {
        ovrHmd_RecenterPose(_ovrHmd);
    }
}

//Gets the current predicted angles from the oculus sensors
void OculusManager::getEulerAngles(float& yaw, float& pitch, float& roll) {
    ovrTrackingState ts = ovrHmd_GetTrackingState(_ovrHmd, ovr_GetTimeInSeconds());
    if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)) {
        ovrPosef headPose = ts.HeadPose.ThePose;
        glm::vec3 angles = safeEulerAngles(toGlm(headPose.Orientation));
        yaw = angles.y;
        pitch = angles.x;
        roll = angles.z;
    } else {
        yaw = 0.0f;
        pitch = 0.0f;
        roll = 0.0f;
    }
}

glm::vec3 OculusManager::getRelativePosition() {
    ovrTrackingState trackingState = ovrHmd_GetTrackingState(_ovrHmd, ovr_GetTimeInSeconds());
    ovrVector3f headPosition = trackingState.HeadPose.ThePose.Position;
    return glm::vec3(headPosition.x, headPosition.y, headPosition.z);
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
    if (_activeEyeIndex != -1) {
        const ovrFovPort& port = _eyeFov[_activeEyeIndex];
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

