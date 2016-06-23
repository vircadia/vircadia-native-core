//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusDisplayPlugin.h"

// Odd ordering of header is required to avoid 'macro redinition warnings'
#include <AudioClient.h>

#include <OVR_CAPI_Audio.h>

#include <shared/NsightHelpers.h>

#include "OculusHelpers.h"

const QString OculusDisplayPlugin::NAME("Oculus Rift");
static ovrPerfHudMode currentDebugMode = ovrPerfHud_Off;

bool OculusDisplayPlugin::internalActivate() {
    bool result = Parent::internalActivate();
    currentDebugMode = ovrPerfHud_Off;
    if (result && _session) {
        ovr_SetInt(_session, OVR_PERF_HUD_MODE, currentDebugMode);
    }
    return result;
}

void OculusDisplayPlugin::init() {
    Plugin::init();

    emit deviceConnected(getName());
}

void OculusDisplayPlugin::cycleDebugOutput() {
    if (_session) {
        currentDebugMode = static_cast<ovrPerfHudMode>((currentDebugMode + 1) % ovrPerfHud_Count);
        ovr_SetInt(_session, OVR_PERF_HUD_MODE, currentDebugMode);
    }
}

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
    using namespace oglplus;
    
    // Present a final black frame to the HMD
    _compositeFramebuffer->Bound(FramebufferTarget::Draw, [] {
        Context::ClearColor(0, 0, 0, 1);
        Context::Clear().ColorBuffer();
    });

    hmdPresent();
    
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

void OculusDisplayPlugin::hmdPresent() {

    PROFILE_RANGE_EX(__FUNCTION__, 0xff00ff00, (uint64_t)_currentPresentFrameIndex)

    if (!_currentSceneTexture) {
        return;
    }

    blit(_compositeFramebuffer, _sceneFbo);
    _sceneFbo->Commit();
    {
        _sceneLayer.SensorSampleTime = _currentPresentFrameInfo.sensorSampleTime;
        _sceneLayer.RenderPose[ovrEyeType::ovrEye_Left] = ovrPoseFromGlm(_currentPresentFrameInfo.renderPose);
        _sceneLayer.RenderPose[ovrEyeType::ovrEye_Right] = ovrPoseFromGlm(_currentPresentFrameInfo.renderPose);
        ovrLayerHeader* layers = &_sceneLayer.Header;
        ovrResult result = ovr_SubmitFrame(_session, _currentPresentFrameIndex, &_viewScaleDesc, &layers, 1);
        if (!OVR_SUCCESS(result)) {
            logWarning("Failed to present");
        }
    }
}

bool OculusDisplayPlugin::isHmdMounted() const {
    ovrSessionStatus status;
    return (OVR_SUCCESS(ovr_GetSessionStatus(_session, &status)) && 
        (ovrFalse != status.HmdMounted));
}

QString OculusDisplayPlugin::getPreferredAudioInDevice() const { 
    WCHAR buffer[OVR_AUDIO_MAX_DEVICE_STR_SIZE];
    if (!OVR_SUCCESS(ovr_GetAudioDeviceInGuidStr(buffer))) {
        return QString();
    }
    return AudioClient::friendlyNameForAudioDevice(buffer);
}

QString OculusDisplayPlugin::getPreferredAudioOutDevice() const { 
    WCHAR buffer[OVR_AUDIO_MAX_DEVICE_STR_SIZE];
    if (!OVR_SUCCESS(ovr_GetAudioDeviceOutGuidStr(buffer))) {
        return QString();
    }
    return AudioClient::friendlyNameForAudioDevice(buffer);
}

