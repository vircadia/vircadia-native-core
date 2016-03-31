//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusDisplayPlugin.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Mmsystem.h>
#include <mmdeviceapi.h>
#include <devicetopology.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <VersionHelpers.h>
#endif

#include <OVR_CAPI_Audio.h>
#include <QtCore/QThread>

#include <shared/NsightHelpers.h>
#include <AudioClient.h>
#include "OculusHelpers.h"

const QString OculusDisplayPlugin::NAME("Oculus Rift");
static ovrPerfHudMode currentDebugMode = ovrPerfHud_Off;

bool OculusDisplayPlugin::internalActivate() {
    bool result = Parent::internalActivate();
    currentDebugMode = ovrPerfHud_Off;
    if (result && _session) {
        ovr_SetInt(_session, OVR_PERF_HUD_MODE, currentDebugMode);
    }

    auto audioClient = DependencyManager::get<AudioClient>();
    QString riftAudioIn = getPreferredAudioInDevice();
    if (riftAudioIn != QString()) {
        _savedAudioIn = audioClient->getDeviceName(QAudio::Mode::AudioInput);
        QMetaObject::invokeMethod(audioClient.data(), "switchInputToAudioDevice", Q_ARG(const QString&, riftAudioIn));
    } else {
        _savedAudioIn.clear();
    }

    QString riftAudioOut = getPreferredAudioOutDevice();
    if (riftAudioOut != QString()) {
        _savedAudioOut = audioClient->getDeviceName(QAudio::Mode::AudioOutput);
        QMetaObject::invokeMethod(audioClient.data(), "switchOutputToAudioDevice", Q_ARG(const QString&, riftAudioOut));
    } else {
        _savedAudioOut.clear();
    }

    return result;
}

void OculusDisplayPlugin::internalDeactivate() {
    auto audioClient = DependencyManager::get<AudioClient>();
    if (_savedAudioIn != QString()) {
        QMetaObject::invokeMethod(audioClient.data(), "switchInputToAudioDevice", Q_ARG(const QString&, _savedAudioIn));
    }
    if (_savedAudioOut != QString()) {
        QMetaObject::invokeMethod(audioClient.data(), "switchOutputToAudioDevice", Q_ARG(const QString&, _savedAudioOut));
    }
    Parent::internalDeactivate();
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

    PROFILE_RANGE_EX(__FUNCTION__, 0xff00ff00, (uint64_t)_currentRenderFrameIndex)

    if (!_currentSceneTexture) {
        return;
    }

    blit(_compositeFramebuffer, _sceneFbo);
    _sceneFbo->Commit();
    {
        _sceneLayer.SensorSampleTime = _currentPresentFrameInfo.sensorSampleTime;
        _sceneLayer.RenderPose[ovrEyeType::ovrEye_Left] = ovrPoseFromGlm(_currentPresentFrameInfo.headPose);
        _sceneLayer.RenderPose[ovrEyeType::ovrEye_Right] = ovrPoseFromGlm(_currentPresentFrameInfo.headPose);
        ovrLayerHeader* layers = &_sceneLayer.Header;
        ovrResult result = ovr_SubmitFrame(_session, _currentRenderFrameIndex, &_viewScaleDesc, &layers, 1);
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

static QString audioDeviceFriendlyName(LPCWSTR device) {
    QString deviceName;

    HRESULT hr = S_OK;
    CoInitialize(NULL);
    IMMDeviceEnumerator* pMMDeviceEnumerator = NULL;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pMMDeviceEnumerator);
    IMMDevice* pEndpoint;
    hr = pMMDeviceEnumerator->GetDevice(device, &pEndpoint);
    if (hr == E_NOTFOUND) {
        printf("Audio Error: device not found\n");
        deviceName = QString("NONE");
    } else {
        IPropertyStore* pPropertyStore;
        pEndpoint->OpenPropertyStore(STGM_READ, &pPropertyStore);
        pEndpoint->Release();
        pEndpoint = NULL;
        PROPVARIANT pv;
        PropVariantInit(&pv);
        hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv);
        pPropertyStore->Release();
        pPropertyStore = NULL;
        deviceName = QString::fromWCharArray((wchar_t*)pv.pwszVal);
        if (!IsWindows8OrGreater()) {
            // Windows 7 provides only the 31 first characters of the device name.
            const DWORD QT_WIN7_MAX_AUDIO_DEVICENAME_LEN = 31;
            deviceName = deviceName.left(QT_WIN7_MAX_AUDIO_DEVICENAME_LEN);
        }
        qDebug() << " device:" << deviceName;
        PropVariantClear(&pv);
    }
    pMMDeviceEnumerator->Release();
    pMMDeviceEnumerator = NULL;
    CoUninitialize();
    return deviceName;
}


QString OculusDisplayPlugin::getPreferredAudioInDevice() const { 
    WCHAR buffer[OVR_AUDIO_MAX_DEVICE_STR_SIZE];
    if (!OVR_SUCCESS(ovr_GetAudioDeviceInGuidStr(buffer))) {
        return QString();
    }
    return audioDeviceFriendlyName(buffer);
}

QString OculusDisplayPlugin::getPreferredAudioOutDevice() const { 
    WCHAR buffer[OVR_AUDIO_MAX_DEVICE_STR_SIZE];
    if (!OVR_SUCCESS(ovr_GetAudioDeviceOutGuidStr(buffer))) {
        return QString();
    }
    return audioDeviceFriendlyName(buffer);
}

