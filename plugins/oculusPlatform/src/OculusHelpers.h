//
//  Created by Bradley Austin Davis on 2015/05/26
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <QtCore/QLoggingCategory>

#include <ovr_capi.h>
#include <GLMHelpers.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <controllers/Forward.h>

Q_DECLARE_LOGGING_CATEGORY(displayplugins)
Q_DECLARE_LOGGING_CATEGORY(oculusLog)

namespace hifi { 
    
struct ovr {
    static bool available();
    static ovrSession acquireRenderSession();
    static void releaseRenderSession(ovrSession session);
    static void withSession(const std::function<void(ovrSession)>& f);
    static ovrSessionStatus getStatus();
    static ovrSessionStatus getStatus(ovrResult& result);
    static ovrTrackingState getTrackingState(double absTime = 0.0, ovrBool latencyMarker = ovrFalse);
    static QString getError();
    static bool handleOVREvents();

    static inline bool quitRequested() { return quitRequested(getStatus()); }
    static inline bool reorientRequested() { return reorientRequested(getStatus()); }
    static inline bool hmdMounted() { return hmdMounted(getStatus()); }
    static inline bool hasInputFocus() { return hasInputFocus(getStatus()); }

    static inline bool quitRequested(const ovrSessionStatus& status) { return status.ShouldQuit != ovrFalse; }
    static inline bool displayLost(const ovrSessionStatus& status) { return status.DisplayLost != ovrFalse; }
    static inline bool isVisible(const ovrSessionStatus& status) { return status.IsVisible != ovrFalse; }
    static inline bool reorientRequested(const ovrSessionStatus& status) { return status.ShouldRecenter != ovrFalse; }
    static inline bool hmdMounted(const ovrSessionStatus& status) { return status.HmdMounted != ovrFalse; }
    static inline bool hasInputFocus(const ovrSessionStatus& status) { return status.HasInputFocus != ovrFalse; }

    // Convenience method for looping over each eye with a lambda
    static inline void for_each_eye(const std::function<void(ovrEyeType eye)>& f) {
        for (ovrEyeType eye = ovrEye_Left; eye < ovrEye_Count; eye = static_cast<ovrEyeType>(eye + 1)) {
            f(eye);
        }
    }

    static inline void for_each_hand(const std::function<void(ovrHandType eye)>& f) {
        for (ovrHandType hand = ovrHand_Left; hand < ovrHand_Count; hand = static_cast<ovrHandType>(hand + 1)) {
            f(hand);
        }
    }

    static inline glm::mat4 toGlm(const ovrMatrix4f& om) {
        return glm::transpose(glm::make_mat4(&om.M[0][0]));
    }

    static inline glm::mat4 toGlm(const ovrFovPort& fovport, float nearPlane = 0.01f, float farPlane = 10000.0f) {
        return toGlm(ovrMatrix4f_Projection(fovport, nearPlane, farPlane, true));
    }

    static inline glm::vec3 toGlm(const ovrVector3f& ov) {
        return glm::make_vec3(&ov.x);
    }

    static inline glm::vec2 toGlm(const ovrVector2f& ov) {
        return glm::make_vec2(&ov.x);
    }

    static inline glm::uvec2 toGlm(const ovrSizei& ov) {
        return glm::uvec2(ov.w, ov.h);
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

    static inline ovrSizei fromGlm(const glm::uvec2& v) {
        return { (int)v.x, (int)v.y };
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

    static controller::Pose toControllerPose(ovrHandType hand, const ovrPoseStatef& handPose);
    static controller::Pose toControllerPose(ovrHandType hand, const ovrPoseStatef& handPose, const ovrPoseStatef& lastHandPose);

};

}  // namespace hifi
