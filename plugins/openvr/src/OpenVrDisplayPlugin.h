//
//  Created by Bradley Austin Davis on 2015/06/12
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <QtGlobal>

#include <openvr.h>

#include <display-plugins/hmd/HmdDisplayPlugin.h>

const float TARGET_RATE_OpenVr = 90.0f;  // FIXME: get from sdk tracked device property? This number is vive-only.

#define OPENVR_THREADED_SUBMIT 1

#if OPENVR_THREADED_SUBMIT
class OpenVrSubmitThread;
class OffscreenGLCanvas;
static const size_t COMPOSITING_BUFFER_SIZE = 3;

struct CompositeInfo {
    using Queue = std::queue<CompositeInfo>;
    using Array = std::array<CompositeInfo, COMPOSITING_BUFFER_SIZE>;
    
    gpu::TexturePointer texture;
    GLuint textureID { 0 };
    glm::mat4 pose;
    GLsync fence{ 0 };
};
#endif

class OpenVrDisplayPlugin : public HmdDisplayPlugin {
    using Parent = HmdDisplayPlugin;
public:
    bool isSupported() const override;
    const QString& getName() const override { return NAME; }

    void init() override;

    float getTargetFrameRate() const override { return TARGET_RATE_OpenVr; }

    void customizeContext() override;
    void uncustomizeContext() override;

    // Stereo specific methods
    void resetSensors() override;
    bool beginFrameRender(uint32_t frameIndex) override;
    void cycleDebugOutput() override { _lockCurrentTexture = !_lockCurrentTexture; }

    bool suppressKeyboard() override;
    void unsuppressKeyboard() override;
    bool isKeyboardVisible() override;

protected:
    bool internalActivate() override;
    void internalDeactivate() override;
    void updatePresentPose() override;

    void compositeLayers() override;
    void hmdPresent() override;
    bool isHmdMounted() const override;
    void postPreview() override;


private:
    vr::IVRSystem* _system { nullptr };
    std::atomic<vr::EDeviceActivityLevel> _hmdActivityLevel { vr::k_EDeviceActivityLevel_Unknown };
    std::atomic<uint32_t> _keyboardSupressionCount{ 0 };
    static const QString NAME;

    vr::HmdMatrix34_t _lastGoodHMDPose;
    mat4 _sensorResetMat;

#if OPENVR_THREADED_SUBMIT
    CompositeInfo::Array _compositeInfos;
    size_t _renderingIndex { 0 };
    std::shared_ptr<OpenVrSubmitThread> _submitThread;
    std::shared_ptr<OffscreenGLCanvas> _submitCanvas;
    friend class OpenVrSubmitThread;
#endif
};
