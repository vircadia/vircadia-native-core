//
//  OculusManager.cpp
//  interface/src/devices
//
//  Created by Stephen Birarda on 5/9/13.
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterfaceConfig.h"

#include <QOpenGLFramebufferObject>

#include <glm/glm.hpp>

#include "Application.h"
#include "OculusManager.h"

ProgramObject OculusManager::_program;
int OculusManager::_textureLocation;
int OculusManager::_lensCenterLocation;
int OculusManager::_screenCenterLocation;
int OculusManager::_scaleLocation;
int OculusManager::_scaleInLocation;
int OculusManager::_hmdWarpParamLocation;
bool OculusManager::_isConnected = false;

#ifdef HAVE_LIBOVR
using namespace OVR;
using namespace OVR::Util::Render;

Ptr<DeviceManager> OculusManager::_deviceManager;
Ptr<HMDDevice> OculusManager::_hmdDevice;
Ptr<SensorDevice> OculusManager::_sensorDevice;
SensorFusion* OculusManager::_sensorFusion;
StereoConfig OculusManager::_stereoConfig;
#endif

void OculusManager::connect() {
#ifdef HAVE_LIBOVR
    System::Init();
    _deviceManager = *DeviceManager::Create();
    _hmdDevice = *_deviceManager->EnumerateDevices<HMDDevice>().CreateDevice();

    if (_hmdDevice) {
        _isConnected = true;
        
        _sensorDevice = *_hmdDevice->GetSensor();
        _sensorFusion = new SensorFusion;
        _sensorFusion->AttachToSensor(_sensorDevice);
        
        HMDInfo info;
        _hmdDevice->GetDeviceInfo(&info);
        _stereoConfig.SetHMDInfo(info);
        
        _program.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() + "shaders/oculus.frag");
        _program.link();
        
        _textureLocation = _program.uniformLocation("texture");
        _lensCenterLocation = _program.uniformLocation("lensCenter");
        _screenCenterLocation = _program.uniformLocation("screenCenter");
        _scaleLocation = _program.uniformLocation("scale");
        _scaleInLocation = _program.uniformLocation("scaleIn");
        _hmdWarpParamLocation = _program.uniformLocation("hmdWarpParam");        
    } else {
        _deviceManager.Clear();
        System::Destroy();
    }
#endif
}

void OculusManager::configureCamera(Camera& camera, int screenWidth, int screenHeight) {
#ifdef HAVE_LIBOVR
    _stereoConfig.SetFullViewport(Viewport(0, 0, screenWidth, screenHeight));
    camera.setAspectRatio(_stereoConfig.GetAspect());
    camera.setFieldOfView(_stereoConfig.GetYFOVDegrees());
#endif    
}

void OculusManager::display(Camera& whichCamera) {
#ifdef HAVE_LIBOVR
    ApplicationOverlay& applicationOverlay = Application::getInstance()->getApplicationOverlay();
    // We only need to render the overlays to a texture once, then we just render the texture as a quad
    applicationOverlay.renderOverlay(true);
    
    Application::getInstance()->getGlowEffect()->prepare(); 

    // render the left eye view to the left side of the screen
    const StereoEyeParams& leftEyeParams = _stereoConfig.GetEyeRenderParams(StereoEye_Left);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(_stereoConfig.GetProjectionCenterOffset(), 0, 0);
    gluPerspective(whichCamera.getFieldOfView(), whichCamera.getAspectRatio(),
        whichCamera.getNearClip(), whichCamera.getFarClip());
    
    glViewport(leftEyeParams.VP.x, leftEyeParams.VP.y, leftEyeParams.VP.w, leftEyeParams.VP.h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(_stereoConfig.GetIPD() * 0.5f, 0, 0);
    
    Application::getInstance()->displaySide(whichCamera);

    applicationOverlay.displayOverlayTextureOculus(whichCamera);
    
    // and the right eye to the right side
    const StereoEyeParams& rightEyeParams = _stereoConfig.GetEyeRenderParams(StereoEye_Right);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glTranslatef(-_stereoConfig.GetProjectionCenterOffset(), 0, 0);
    gluPerspective(whichCamera.getFieldOfView(), whichCamera.getAspectRatio(),
        whichCamera.getNearClip(), whichCamera.getFarClip());
    
    glViewport(rightEyeParams.VP.x, rightEyeParams.VP.y, rightEyeParams.VP.w, rightEyeParams.VP.h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(_stereoConfig.GetIPD() * -0.5f, 0, 0);
    
    Application::getInstance()->displaySide(whichCamera);

    applicationOverlay.displayOverlayTextureOculus(whichCamera);
   
    glPopMatrix();
    
    // restore our normal viewport
    const Viewport& fullViewport = _stereoConfig.GetFullViewport();
    glViewport(fullViewport.x, fullViewport.y, fullViewport.w, fullViewport.h);

    QOpenGLFramebufferObject* fbo = Application::getInstance()->getGlowEffect()->render(true);
    glBindTexture(GL_TEXTURE_2D, fbo->texture());

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(fullViewport.x, fullViewport.x + fullViewport.w, fullViewport.y, fullViewport.y + fullViewport.h);
    glDisable(GL_DEPTH_TEST);
    
    // for reference on setting these values, see SDK file Samples/OculusRoomTiny/RenderTiny_Device.cpp
    
    float scaleFactor = 1.0 / _stereoConfig.GetDistortionScale();
    float aspectRatio = _stereoConfig.GetAspect();
    
    glDisable(GL_BLEND);
    _program.bind();
    _program.setUniformValue(_textureLocation, 0);
    const DistortionConfig& distortionConfig = _stereoConfig.GetDistortionConfig();
    _program.setUniformValue(_lensCenterLocation, (0.5 + distortionConfig.XCenterOffset * 0.5) * 0.5, 0.5);
    _program.setUniformValue(_screenCenterLocation, 0.25, 0.5);
    _program.setUniformValue(_scaleLocation, 0.25 * scaleFactor, 0.5 * scaleFactor * aspectRatio);
    _program.setUniformValue(_scaleInLocation, 4, 2 / aspectRatio);
    _program.setUniformValue(_hmdWarpParamLocation, distortionConfig.K[0], distortionConfig.K[1],
        distortionConfig.K[2], distortionConfig.K[3]);

    glColor3f(1, 0, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(0, 0);
    glTexCoord2f(0.5, 0);
    glVertex2f(leftEyeParams.VP.w, 0);
    glTexCoord2f(0.5, 1);
    glVertex2f(leftEyeParams.VP.w, leftEyeParams.VP.h);
    glTexCoord2f(0, 1);
    glVertex2f(0, leftEyeParams.VP.h);
    glEnd();
    
    _program.setUniformValue(_lensCenterLocation, 0.5 + (0.5 - distortionConfig.XCenterOffset * 0.5) * 0.5, 0.5);
    _program.setUniformValue(_screenCenterLocation, 0.75, 0.5);
    
    glBegin(GL_QUADS);
    glTexCoord2f(0.5, 0);
    glVertex2f(leftEyeParams.VP.w, 0);
    glTexCoord2f(1, 0);
    glVertex2f(fullViewport.w, 0);
    glTexCoord2f(1, 1);
    glVertex2f(fullViewport.w, leftEyeParams.VP.h);
    glTexCoord2f(0.5, 1);
    glVertex2f(leftEyeParams.VP.w, leftEyeParams.VP.h);
    glEnd();
    
    glEnable(GL_BLEND);           
    glBindTexture(GL_TEXTURE_2D, 0);
    _program.release();
    
    glPopMatrix();
#endif
}

void OculusManager::reset() {
#ifdef HAVE_LIBOVR
    _sensorFusion->Reset();
#endif
}

void OculusManager::getEulerAngles(float& yaw, float& pitch, float& roll) {
#ifdef HAVE_LIBOVR
    _sensorFusion->GetOrientation().GetEulerAngles<Axis_Y, Axis_X, Axis_Z, Rotate_CCW, Handed_R>(&yaw, &pitch, &roll);
#endif
}

