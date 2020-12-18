//
//  Created by Bradley Austin Davis on 2018/11/15
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <VrApi_Types.h>

namespace ovr {

struct Fov {
    float leftRightUpDown[4];
    Fov() {}
    Fov(const ovrMatrix4f& mat) { extract(mat); }
    void extract(const ovrMatrix4f& mat);
    void extend(const Fov& other);
    glm::mat4 withZ(const glm::mat4& other) const;
    glm::mat4 withZ(float nearZ, float farZ) const;
};

// Convenience method for looping over each eye with a lambda
static inline void for_each_eye(const std::function<void(ovrEye)>& f) {
    f(VRAPI_EYE_LEFT);
    f(VRAPI_EYE_RIGHT);
}

static inline void for_each_hand(const std::function<void(ovrTrackedDeviceTypeId)>& f) {
    f(VRAPI_TRACKED_DEVICE_HAND_LEFT);
    f(VRAPI_TRACKED_DEVICE_HAND_RIGHT);
}

static inline glm::mat4 toGlm(const ovrMatrix4f& om) {
    return glm::transpose(glm::make_mat4(&om.M[0][0]));
}

static inline glm::vec3 toGlm(const ovrVector3f& ov) {
    return glm::make_vec3(&ov.x);
}

static inline glm::vec2 toGlm(const ovrVector2f& ov) {
    return glm::make_vec2(&ov.x);
}

static inline glm::quat toGlm(const ovrQuatf& oq) {
    return glm::make_quat(&oq.x);
}

static inline glm::mat4 toGlm(const ovrPosef& op) {
    glm::mat4 orientation = glm::mat4_cast(toGlm(op.Orientation));
    glm::mat4 translation = glm::translate(glm::mat4(), toGlm(op.Position));
    return translation * orientation;
}

static inline ovrMatrix4f fromGlm(const glm::mat4& m) {
    ovrMatrix4f result;
    glm::mat4 transposed(glm::transpose(m));
    memcpy(result.M, &(transposed[0][0]), sizeof(float) * 16);
    return result;
}

static inline ovrVector3f fromGlm(const glm::vec3& v) {
    return { v.x, v.y, v.z };
}

static inline ovrVector2f fromGlm(const glm::vec2& v) {
    return { v.x, v.y };
}

static inline ovrQuatf fromGlm(const glm::quat& q) {
    return { q.x, q.y, q.z, q.w };
}

static inline ovrPosef poseFromGlm(const glm::mat4& m) {
    glm::vec3 translation = glm::vec3(m[3]) / m[3].w;
    glm::quat orientation = glm::quat_cast(m);
    ovrPosef result;
    result.Orientation = fromGlm(orientation);
    result.Position = fromGlm(translation);
    return result;
}

}





