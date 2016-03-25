//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusDisplayPlugin.h"
#include <shared/NsightHelpers.h>
#include "OculusHelpers.h"

const QString OculusDisplayPlugin::NAME("Oculus Rift");

void OculusDisplayPlugin::customizeContext() {
    Parent::customizeContext();
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
    Parent::uncustomizeContext();
}


template <typename SrcFbo, typename DstFbo>
void blit(const SrcFbo& srcFbo, const DstFbo& dstFbo) {
    using namespace oglplus;
    srcFbo->Bound(FramebufferTarget::Read, [&] {
        dstFbo->Bound(FramebufferTarget::Draw, [&] {
            Context::BlitFramebuffer(
                0, 0, srcFbo->size.x, srcFbo->size.y,
                0, 0, dstFbo->size.x, dstFbo->size.y,
                BufferSelectBit::ColorBuffer, BlitFilter::Linear);
        });
    });
}

void OculusDisplayPlugin::updateFrameData() {
    Parent::updateFrameData();
    _sceneLayer.RenderPose[ovrEyeType::ovrEye_Left] = ovrPoseFromGlm(_currentRenderEyePoses[Left]);
    _sceneLayer.RenderPose[ovrEyeType::ovrEye_Right] = ovrPoseFromGlm(_currentRenderEyePoses[Right]);
}

void OculusDisplayPlugin::hmdPresent() {

    PROFILE_RANGE_EX(__FUNCTION__, 0xff00ff00, (uint64_t)_currentRenderFrameIndex)

    if (!_currentSceneTexture) {
        return;
    }

    blit(_compositeFramebuffer, _sceneFbo);
    {
        ovrLayerHeader* layers = &_sceneLayer.Header;
        ovrResult result = ovr_SubmitFrame(_session, _currentRenderFrameIndex, &_viewScaleDesc, &layers, 1);
        if (!OVR_SUCCESS(result)) {
            qDebug() << result;
        }
    }
    _sceneFbo->Increment();
}
