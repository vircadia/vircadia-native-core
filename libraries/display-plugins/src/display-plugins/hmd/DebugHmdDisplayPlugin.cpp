//
//  Created by Bradley Austin Davis on 2016/07/31
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "DebugHmdDisplayPlugin.h"

#include <QtCore/QProcessEnvironment>

#include <ViewFrustum.h>
#include <controllers/Pose.h>
#include <gpu/Frame.h>

const QString DebugHmdDisplayPlugin::NAME("HMD Simulator");

static const QString DEBUG_FLAG("HIFI_DEBUG_HMD");
static bool enableDebugHmd = QProcessEnvironment::systemEnvironment().contains(DEBUG_FLAG);


bool DebugHmdDisplayPlugin::isSupported() const {
    return enableDebugHmd;
}

void DebugHmdDisplayPlugin::resetSensors() {
    _currentRenderFrameInfo.renderPose = glm::mat4(); // identity
}

bool DebugHmdDisplayPlugin::beginFrameRender(uint32_t frameIndex) {
    _currentRenderFrameInfo = FrameInfo();
    _currentRenderFrameInfo.sensorSampleTime = secTimestampNow();
    _currentRenderFrameInfo.predictedDisplayTime = _currentRenderFrameInfo.sensorSampleTime;
    // FIXME simulate head movement
    //_currentRenderFrameInfo.renderPose = ;
    //_currentRenderFrameInfo.presentPose = _currentRenderFrameInfo.renderPose;

    withNonPresentThreadLock([&] {
        _uiModelTransform = DependencyManager::get<CompositorHelper>()->getModelTransform();
        _frameInfos[frameIndex] = _currentRenderFrameInfo;
        
        _handPoses[0] = glm::translate(mat4(), vec3(0.3f * cosf(secTimestampNow() * 3.0f), -0.3f * sinf(secTimestampNow() * 5.0f), 0.0f));
        _handLasers[0].color = vec4(1, 0, 0, 1);
        _handLasers[0].mode = HandLaserMode::Overlay;

        _handPoses[1] = glm::translate(mat4(), vec3(0.3f * sinf(secTimestampNow() * 3.0f), -0.3f * cosf(secTimestampNow() * 5.0f), 0.0f));
        _handLasers[1].color = vec4(0, 1, 1, 1);
        _handLasers[1].mode = HandLaserMode::Overlay;
    });
    return Parent::beginFrameRender(frameIndex);
}

// DLL based display plugins MUST initialize GLEW inside the DLL code.
void DebugHmdDisplayPlugin::customizeContext() {
    glewExperimental = true;
    glewInit();
    glGetError(); // clear the potential error from glewExperimental
    Parent::customizeContext();
}

bool DebugHmdDisplayPlugin::internalActivate() {
    _ipd = 0.0327499993f * 2.0f;
    _eyeProjections[0][0] = vec4{ 0.759056330, 0.000000000, 0.000000000, 0.000000000 };
    _eyeProjections[0][1] = vec4{ 0.000000000, 0.682773232, 0.000000000, 0.000000000 };
    _eyeProjections[0][2] = vec4{ -0.0580431037, -0.00619550655, -1.00000489, -1.00000000 };
    _eyeProjections[0][3] = vec4{ 0.000000000, 0.000000000, -0.0800003856, 0.000000000 };
    _eyeProjections[1][0] = vec4{ 0.752847493, 0.000000000, 0.000000000, 0.000000000 };
    _eyeProjections[1][1] = vec4{ 0.000000000, 0.678060353, 0.000000000, 0.000000000 };
    _eyeProjections[1][2] = vec4{ 0.0578232110, -0.00669418881, -1.00000489, -1.000000000 };
    _eyeProjections[1][3] = vec4{ 0.000000000, 0.000000000, -0.0800003856, 0.000000000 };
    _eyeInverseProjections[0] = glm::inverse(_eyeProjections[0]);
    _eyeInverseProjections[1] = glm::inverse(_eyeProjections[1]);
    _eyeOffsets[0][3] = vec4{ -0.0327499993, 0.0, 0.0149999997, 1.0 };
    _eyeOffsets[0][3] = vec4{ 0.0327499993, 0.0, 0.0149999997, 1.0 };
    _renderTargetSize = { 3024, 1680 };
    _cullingProjection = _eyeProjections[0];
    // This must come after the initialization, so that the values calculated
    // above are available during the customizeContext call (when not running
    // in threaded present mode)
    return Parent::internalActivate();
}

void DebugHmdDisplayPlugin::updatePresentPose() {
    float yaw = sinf(secTimestampNow()) * 0.25f;
    float pitch = cosf(secTimestampNow()) * 0.25f;
    // Simulates head pose latency correction
    _currentPresentFrameInfo.presentPose = 
        glm::mat4_cast(glm::angleAxis(yaw, Vectors::UP)) * 
        glm::mat4_cast(glm::angleAxis(pitch, Vectors::RIGHT));
}
