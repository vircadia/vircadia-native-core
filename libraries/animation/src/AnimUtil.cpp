//
//  AnimUtil.cpp
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimUtil.h"
#include <GLMHelpers.h>
#include <NumericalConstants.h>
#include <DebugDraw.h>

// TODO: use restrict keyword
// TODO: excellent candidate for simd vectorization.
void blend(size_t numPoses, const AnimPose* a, const AnimPose* b, float alpha, AnimPose* result) {
    for (size_t i = 0; i < numPoses; i++) {
        const AnimPose& aPose = a[i];
        const AnimPose& bPose = b[i];

        result[i].scale() = lerp(aPose.scale(), bPose.scale(), alpha);
        result[i].rot() = safeLerp(aPose.rot(), bPose.rot(), alpha);
        result[i].trans() = lerp(aPose.trans(), bPose.trans(), alpha);
    }
}

void blend3(size_t numPoses, const AnimPose* a, const AnimPose* b, const AnimPose* c, float* alphas, AnimPose* result) {
    for (size_t i = 0; i < numPoses; i++) {
        const AnimPose& aPose = a[i];
        const AnimPose& bPose = b[i];
        const AnimPose& cPose = c[i];

        result[i].scale() = alphas[0] * aPose.scale() + alphas[1] * bPose.scale() + alphas[2] * cPose.scale();
        result[i].rot() = safeLinearCombine3(aPose.rot(), bPose.rot(), cPose.rot(), alphas);
        result[i].trans() = alphas[0] * aPose.trans() + alphas[1] * bPose.trans() + alphas[2] * cPose.trans();
    }
}

void blend4(size_t numPoses, const AnimPose* a, const AnimPose* b, const AnimPose* c, const AnimPose* d, float* alphas, AnimPose* result) {
    for (size_t i = 0; i < numPoses; i++) {
        const AnimPose& aPose = a[i];
        const AnimPose& bPose = b[i];
        const AnimPose& cPose = c[i];
        const AnimPose& dPose = d[i];

        result[i].scale() = alphas[0] * aPose.scale() + alphas[1] * bPose.scale() + alphas[2] * cPose.scale() + alphas[3] * dPose.scale();
        result[i].rot() = safeLinearCombine4(aPose.rot(), bPose.rot(), cPose.rot(), dPose.rot(), alphas);
        result[i].trans() = alphas[0] * aPose.trans() + alphas[1] * bPose.trans() + alphas[2] * cPose.trans() + alphas[3] * dPose.trans();
    }
}

// additive blend
void blendAdd(size_t numPoses, const AnimPose* a, const AnimPose* b, float alpha, AnimPose* result) {

    const glm::vec3 IDENTITY_SCALE = glm::vec3(1.0f);
    const glm::quat IDENTITY_ROT = glm::quat();

    for (size_t i = 0; i < numPoses; i++) {
        const AnimPose& aPose = a[i];
        const AnimPose& bPose = b[i];

        result[i].scale() = aPose.scale() * lerp(IDENTITY_SCALE, bPose.scale(), alpha);

        // ensure that delta has the same "polarity" as the identity quat.
        // we don't need to do a full dot product, just sign of w is sufficient.
        glm::quat delta = bPose.rot();
        if (delta.w < 0.0f) {
            delta = -delta;
        }
        delta = glm::lerp(IDENTITY_ROT, delta, alpha);
        result[i].rot() = glm::normalize(aPose.rot() * delta);
        result[i].trans() = aPose.trans() + (alpha * bPose.trans());
    }
}

glm::quat averageQuats(size_t numQuats, const glm::quat* quats) {
    if (numQuats == 0) {
        return glm::quat();
    }
    glm::quat accum = quats[0];
    glm::quat firstRot = quats[0];
    for (size_t i = 1; i < numQuats; i++) {
        glm::quat rot = quats[i];
        float dot = glm::dot(firstRot, rot);
        if (dot < 0.0f) {
            rot = -rot;
        }
        accum += rot;
    }
    return glm::normalize(accum);
}

float accumulateTime(float startFrame, float endFrame, float timeScale, float currentFrame, float dt, bool loopFlag,
                     const QString& id, AnimVariantMap& triggersOut) {

    const float EPSILON = 0.0001f;
    float frame = currentFrame;
    const float clampedStartFrame = std::min(startFrame, endFrame);
    if (fabsf(clampedStartFrame - endFrame) <= 1.0f) {
        // An animation of a single frame should not send loop or done triggers.
        frame = endFrame;
    } else if (timeScale > EPSILON && dt > EPSILON) {
        // accumulate time, keeping track of loops and end of animation events.
        const float FRAMES_PER_SECOND = 30.0f;
        float framesRemaining = (dt * timeScale) * FRAMES_PER_SECOND;

        // prevent huge dt or timeScales values from causing many trigger events.
        uint32_t triggerCount = 0;
        const uint32_t MAX_TRIGGER_COUNT = 3;

        while (framesRemaining > EPSILON && triggerCount < MAX_TRIGGER_COUNT) {
            float framesTillEnd = endFrame - frame;
            // when looping, add one frame between start and end.
            if (loopFlag) {
                framesTillEnd += 1.0f;
            }
            if (framesRemaining >= framesTillEnd) {
                if (loopFlag) {
                    // anim loop
                    triggersOut.setTrigger(id + "OnLoop");
                    framesRemaining -= framesTillEnd;
                    frame = clampedStartFrame;
                } else {
                    // anim end
                    triggersOut.setTrigger(id + "OnDone");
                    frame = endFrame;
                    framesRemaining = 0.0f;
                }
                triggerCount++;
            } else {
                frame += framesRemaining;
                framesRemaining = 0.0f;
            }
        }
    }
    return frame;
}

// rotate bone's y-axis with target.
AnimPose boneLookAt(const glm::vec3& target, const AnimPose& bone) {
    glm::vec3 u, v, w;
    generateBasisVectors(target - bone.trans(), bone.rot() * Vectors::UNIT_X, u, v, w);
    glm::mat4 lookAt(glm::vec4(v, 0.0f),
                     glm::vec4(u, 0.0f),
                     // AJT: TODO REVISIT THIS, this could be -w.
                     glm::vec4(glm::normalize(glm::cross(v, u)), 0.0f),
                     glm::vec4(bone.trans(), 1.0f));
    return AnimPose(lookAt);
}

// This will attempt to determine the proper body facing of a characters body
// assumes headRot is z-forward and y-up.
// and returns a bodyRot that is also z-forward and y-up
glm::quat computeBodyFacingFromHead(const glm::quat& headRot, const glm::vec3& up) {

    glm::vec3 bodyUp = glm::normalize(up);

    // initially take the body facing from the head.
    glm::vec3 headUp = headRot * Vectors::UNIT_Y;
    glm::vec3 headForward = headRot * Vectors::UNIT_Z;
    glm::vec3 headLeft = headRot * Vectors::UNIT_X;
    const float NOD_THRESHOLD = cosf(glm::radians(45.0f));
    const float TILT_THRESHOLD = cosf(glm::radians(30.0f));

    glm::vec3 bodyForward = headForward;

    float nodDot = glm::dot(headForward, bodyUp);
    float tiltDot = glm::dot(headLeft, bodyUp);

    if (fabsf(tiltDot) < TILT_THRESHOLD) { // if we are not tilting too much
        if (nodDot < -NOD_THRESHOLD) { // head is looking downward
            // the body should face in the same direction as the top the head.
            bodyForward = headUp;
        } else if (nodDot > NOD_THRESHOLD) {  // head is looking upward
            // the body should face away from the top of the head.
            bodyForward = -headUp;
        }
    }

    // cancel out upward component
    bodyForward = glm::normalize(bodyForward - nodDot * bodyUp);

    glm::vec3 u, v, w;
    generateBasisVectors(bodyForward, bodyUp, u, v, w);

    // create matrix from orthogonal basis vectors
    glm::mat4 bodyMat(glm::vec4(w, 0.0f), glm::vec4(v, 0.0f), glm::vec4(u, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    return glmExtractRotation(bodyMat);
}


const float INV_SQRT_3 = 1.0f / sqrtf(3.0f);
const int DOP14_COUNT = 14;
const glm::vec3 DOP14_NORMALS[DOP14_COUNT] = {
    Vectors::UNIT_X,
    -Vectors::UNIT_X,
    Vectors::UNIT_Y,
    -Vectors::UNIT_Y,
    Vectors::UNIT_Z,
    -Vectors::UNIT_Z,
    glm::vec3(INV_SQRT_3, INV_SQRT_3, INV_SQRT_3),
    -glm::vec3(INV_SQRT_3, INV_SQRT_3, INV_SQRT_3),
    glm::vec3(INV_SQRT_3, -INV_SQRT_3, INV_SQRT_3),
    -glm::vec3(INV_SQRT_3, -INV_SQRT_3, INV_SQRT_3),
    glm::vec3(INV_SQRT_3, INV_SQRT_3, -INV_SQRT_3),
    -glm::vec3(INV_SQRT_3, INV_SQRT_3, -INV_SQRT_3),
    glm::vec3(INV_SQRT_3, -INV_SQRT_3, -INV_SQRT_3),
    -glm::vec3(INV_SQRT_3, -INV_SQRT_3, -INV_SQRT_3)
};

// returns true if the given point lies inside of the k-dop, specified by shapeInfo & shapePose.
// if the given point does lie within the k-dop, it also returns the amount of displacement necessary to push that point outward
// such that it lies on the surface of the kdop.
bool findPointKDopDisplacement(const glm::vec3& point, const AnimPose& shapePose, const HFMJointShapeInfo& shapeInfo, glm::vec3& displacementOut) {

    // transform point into local space of jointShape.
    glm::vec3 localPoint = shapePose.inverse().xformPoint(point);

    // Only works for 14-dop shape infos.
    if (shapeInfo.dots.size() != DOP14_COUNT) {
        return false;
    }

    glm::vec3 minDisplacement(FLT_MAX);
    float minDisplacementLen = FLT_MAX;
    glm::vec3 p = localPoint - shapeInfo.avgPoint;
    float pLen = glm::length(p);
    if (pLen > 0.0f) {
        int slabCount = 0;
        for (int i = 0; i < DOP14_COUNT; i++) {
            float dot = glm::dot(p, DOP14_NORMALS[i]);
            if (dot > 0.0f && dot < shapeInfo.dots[i]) {
                slabCount++;
                float distToPlane = pLen * (shapeInfo.dots[i] / dot);
                float displacementLen = distToPlane - pLen;

                // keep track of the smallest displacement
                if (displacementLen < minDisplacementLen) {
                    minDisplacementLen = displacementLen;
                    minDisplacement = (p / pLen) * displacementLen;
                }
            }
        }
        if (slabCount == (DOP14_COUNT / 2) && minDisplacementLen != FLT_MAX) {
            // we are within the k-dop so push the point along the minimum displacement found
            displacementOut = shapePose.xformVectorFast(minDisplacement);
            return true;
        } else {
            // point is outside of kdop
            return false;
        }
    } else {
        // point is directly on top of shapeInfo.avgPoint.
        // push the point out along the x axis.
        displacementOut = shapePose.xformVectorFast(shapeInfo.points[0]);
        return true;
    }
}

// See https://easings.net/en# for a graphical visualiztion of easing types.
float easingFunc(float alpha, EasingType type) {
    switch (type) {
    case EasingType_Linear:
        return alpha;
    case EasingType_EaseInSine:
        return sinf((alpha - 1.0f) * PI_OVER_TWO) + 1.0f;
    case EasingType_EaseOutSine:
        return sinf(alpha * PI_OVER_TWO);
    case EasingType_EaseInOutSine:
        return 0.5f * (1.0f - cosf(alpha * PI));
    case EasingType_EaseInQuad:
        return alpha * alpha;
    case EasingType_EaseOutQuad:
        return -(alpha * (alpha - 2.0f));
    case EasingType_EaseInOutQuad:
        return (alpha < 0.5f) ? (2.0f * alpha * alpha) : ((-2.0f * alpha * alpha) + (4.0f * alpha) - 1.0f);
    case EasingType_EaseInCubic:
        return alpha * alpha * alpha;
    case EasingType_EaseOutCubic: {
            float temp = alpha - 1.0f;
            return temp * temp * temp + 1.0f;
        }
    case EasingType_EaseInOutCubic:
        if (alpha < 0.5f) {
            return 4.0f * alpha * alpha * alpha;
        } else {
            float temp = ((2.0f * alpha) - 2.0f);
            return 0.5f * temp * temp * temp + 1.0f;
        }
    case EasingType_EaseInQuart:
        return alpha * alpha * alpha * alpha;
    case EasingType_EaseOutQuart: {
            float temp = alpha - 1.0f;
            return temp * temp * temp * (1.0f - alpha) + 1.0f;
        }
    case EasingType_EaseInOutQuart:
        if (alpha < 0.5f) {
            return 8.0f * alpha * alpha * alpha * alpha;
        } else {
            float temp = alpha - 1.0f;
            return -8.0f * temp * temp * temp * temp + 1.0f;
        }
    case EasingType_EaseInQuint:
        return alpha * alpha * alpha * alpha * alpha;
    case EasingType_EaseOutQuint: {
            float temp = (alpha - 1.0f);
            return temp * temp * temp * temp * temp + 1.0f;
        }
    case EasingType_EaseInOutQuint:
        if (alpha < 0.5f) {
            return 16.0f * alpha * alpha * alpha * alpha * alpha;
        } else {
            float temp = ((2.0f * alpha) - 2.0f);
            return 0.5f * temp * temp * temp * temp * temp + 1.0f;
        }
    case EasingType_EaseInExpo:
        return (alpha == 0.0f) ? alpha : powf(2.0f, 10.0f * (alpha - 1.0f));
    case EasingType_EaseOutExpo:
        return (alpha == 1.0f) ? alpha : 1.0f - powf(2.0f, -10.0f * alpha);
    case EasingType_EaseInOutExpo:
        if (alpha == 0.0f || alpha == 1.0f)
            return alpha;
        else if (alpha < 0.5f) {
            return 0.5f * powf(2.0f, (20.0f * alpha) - 10.0f);
        } else {
            return -0.5f * powf(2.0f, (-20.0f * alpha) + 10.0f) + 1.0f;
        }
    case EasingType_EaseInCirc:
        return 1.0f - sqrtf(1.0f - (alpha * alpha));
    case EasingType_EaseOutCirc:
        return sqrtf((2.0f - alpha) * alpha);
    case EasingType_EaseInOutCirc:
        if (alpha < 0.5f) {
            return 0.5f * (1.0f - sqrtf(1.0f - 4.0f * (alpha * alpha)));
        } else {
            return 0.5f * (sqrtf(-((2.0f * alpha) - 3.0f) * ((2.0f * alpha) - 1.0f)) + 1.0f);
        }
    default:
        return alpha;
    }
}
