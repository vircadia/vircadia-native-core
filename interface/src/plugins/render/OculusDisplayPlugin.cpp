//
//  OculusDisplayPlugin.cpp
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusDisplayPlugin.h"

#include <OVR_CAPI.h>

bool OculusDisplayPlugin::isSupported() {
    ovr_Initialize();
    bool result = false;
    if (ovrHmd_Detect() != 0) {
        result = true;
    }
    ovr_Shutdown();
    return result;
}

void OculusDisplayPlugin::activate() {
}

void OculusDisplayPlugin::deactivate() {
}

void OculusDisplayPlugin::init() {
}

void OculusDisplayPlugin::deinit() {
}


void OculusDisplayPlugin::overrideOffAxisFrustum(
                                    float& left, float& right, float& bottom, float& top,
                                    float& nearVal, float& farVal,
                                    glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const {
    
}



#if 0
// Set the desired FBO texture size. If it hasn't changed, this does nothing.
// Otherwise, it must rebuild the FBOs
if (OculusManager::isConnected()) {
    DependencyManager::get<TextureCache>()->setFrameBufferSize(OculusManager::getRenderTargetSize());
} else {
#endif

#if 0
    // Update camera position
    if (!OculusManager::isConnected()) {
        _myCamera.update(1.0f / _fps);
    }
#endif

#if 0
    if (OculusManager::isConnected()) {
        //When in mirror mode, use camera rotation. Otherwise, use body rotation
        if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
            OculusManager::display(_myCamera.getRotation(), _myCamera.getPosition(), _myCamera);
        } else {
            OculusManager::display(_myAvatar->getWorldAlignedOrientation(), _myAvatar->getDefaultEyePosition(), _myCamera);
        }
        _myCamera.update(1.0f / _fps);

#endif

#if 0
        OculusManager::abandonCalibration();
#endif

#if 0
        if (OculusManager::isConnected()) {
            return getMouseX() >= 0 && getMouseX() <= _glWidget->getDeviceWidth() &&
                getMouseY() >= 0 && getMouseY() <= _glWidget->getDeviceHeight();
        }
#endif

#if 0
        if (OculusManager::isConnected()) {
            glm::vec2 pos = _applicationOverlay.screenToOverlay(glm::vec2(getTrueMouseX(), getTrueMouseY()));
            return pos.x;
        }
#endif
#if 0
        if (OculusManager::isConnected()) {
            glm::vec2 pos = _applicationOverlay.screenToOverlay(glm::vec2(getTrueMouseX(), getTrueMouseY()));
            return pos.y;
        }
#endif

