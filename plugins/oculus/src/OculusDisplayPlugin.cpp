//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusDisplayPlugin.h"
#include "OculusHelpers.h"

const QString OculusDisplayPlugin::NAME("Oculus Rift");

void OculusDisplayPlugin::activate() {
    OculusBaseDisplayPlugin::activate();
}

void OculusDisplayPlugin::customizeContext() {
    OculusBaseDisplayPlugin::customizeContext();
    _sceneFbo = SwapFboPtr(new SwapFramebufferWrapper(_session));
    _sceneFbo->Init(getRecommendedRenderSize());

    // We're rendering both eyes to the same texture, so only one of the 
    // pointers is populated
    _sceneLayer.ColorTexture[0] = _sceneFbo->color;
    // not needed since the structure was zeroed on init, but explicit
    _sceneLayer.ColorTexture[1] = nullptr;

    enableVsync(false);
    // Only enable mirroring if we know vsync is disabled
    _enablePreview = !isVsyncEnabled();
}

void OculusDisplayPlugin::uncustomizeContext() {
#if (OVR_MAJOR_VERSION >= 6)
    _sceneFbo.reset();
#endif
    OculusBaseDisplayPlugin::uncustomizeContext();
}

void OculusDisplayPlugin::internalPresent() {
    if (!_currentSceneTexture) {
        return;
    }

    using namespace oglplus;
    const auto& size = _sceneFbo->size;
    _sceneFbo->Bound([&] {
        Context::Viewport(size.x, size.y);
        glBindTexture(GL_TEXTURE_2D, _currentSceneTexture);
        //glEnable(GL_FRAMEBUFFER_SRGB);
        GLenum err = glGetError();
        drawUnitQuad();
        //glDisable(GL_FRAMEBUFFER_SRGB);
    });

    uint32_t frameIndex { 0 };
    EyePoses eyePoses;
    {
        Lock lock(_mutex);
        Q_ASSERT(_sceneTextureToFrameIndexMap.contains(_currentSceneTexture));
        frameIndex = _sceneTextureToFrameIndexMap[_currentSceneTexture];
        Q_ASSERT(_frameEyePoses.contains(frameIndex));
        eyePoses = _frameEyePoses[frameIndex];
    }

    _sceneLayer.RenderPose[ovrEyeType::ovrEye_Left] = eyePoses.first;
    _sceneLayer.RenderPose[ovrEyeType::ovrEye_Right] = eyePoses.second;

    {

        ovrLayerHeader* layers = &_sceneLayer.Header;
        ovrResult result = ovr_SubmitFrame(_session, frameIndex, &_viewScaleDesc, &layers, 1);
        if (!OVR_SUCCESS(result)) {
            qDebug() << result;
        }
    }
    _sceneFbo->Increment();

    // Handle mirroring to screen in base class
    HmdDisplayPlugin::internalPresent();
}

void OculusDisplayPlugin::setEyeRenderPose(uint32_t frameIndex, Eye eye, const glm::mat4& pose) {
    auto ovrPose = ovrPoseFromGlm(pose);
    {
        Lock lock(_mutex);
        if (eye == Eye::Left) {
            _frameEyePoses[frameIndex].first = ovrPose;
        } else {
            _frameEyePoses[frameIndex].second = ovrPose;
        }
    }
}
