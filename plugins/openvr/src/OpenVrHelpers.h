//
//  Created by Bradley Austin Davis on 2015/06/12
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <openvr.h>
#include <GLMHelpers.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <controllers/Forward.h>
#include <plugins/Forward.h>

bool openVrSupported();

vr::IVRSystem* acquireOpenVrSystem();
void releaseOpenVrSystem();
void handleOpenVrEvents();
bool openVrQuitRequested();
void enableOpenVrKeyboard(PluginContainer* container);
void disableOpenVrKeyboard();
bool isOpenVrKeyboardShown();


template<typename F>
void openvr_for_each_eye(F f) {
    f(vr::Hmd_Eye::Eye_Left);
    f(vr::Hmd_Eye::Eye_Right);
}

inline mat4 toGlm(const vr::HmdMatrix44_t& m) {
    return glm::transpose(glm::make_mat4(&m.m[0][0]));
}

inline vec3 toGlm(const vr::HmdVector3_t& v) {
    return vec3(v.v[0], v.v[1], v.v[2]);
}

inline mat4 toGlm(const vr::HmdMatrix34_t& m) {
    mat4 result = mat4(
        m.m[0][0], m.m[1][0], m.m[2][0], 0.0,
        m.m[0][1], m.m[1][1], m.m[2][1], 0.0,
        m.m[0][2], m.m[1][2], m.m[2][2], 0.0,
        m.m[0][3], m.m[1][3], m.m[2][3], 1.0f);
    return result;
}

inline vr::HmdMatrix34_t toOpenVr(const mat4& m) {
    vr::HmdMatrix34_t result;
    for (uint8_t i = 0; i < 3; ++i) {
        for (uint8_t j = 0; j < 4; ++j) {
            result.m[i][j] = m[j][i];
        }
    }
    return result;
}

struct PoseData {
    uint32_t frameIndex{ 0 };
    vr::TrackedDevicePose_t vrPoses[vr::k_unMaxTrackedDeviceCount];
    mat4 poses[vr::k_unMaxTrackedDeviceCount];
    vec3 linearVelocities[vr::k_unMaxTrackedDeviceCount];
    vec3 angularVelocities[vr::k_unMaxTrackedDeviceCount];

    void update(const glm::mat4& resetMat) {
        for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
            poses[i] = resetMat * toGlm(vrPoses[i].mDeviceToAbsoluteTracking);
            linearVelocities[i] = transformVectorFast(resetMat, toGlm(vrPoses[i].vVelocity));
            angularVelocities[i] = transformVectorFast(resetMat, toGlm(vrPoses[i].vAngularVelocity));
        }
    }
};


controller::Pose openVrControllerPoseToHandPose(bool isLeftHand, const mat4& mat, const vec3& linearVelocity, const vec3& angularVelocity);
