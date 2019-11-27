//
//  AnimUtil.h
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimUtil_h
#define hifi_AnimUtil_h

#include "AnimNode.h"

// this is where the magic happens
void blend(size_t numPoses, const AnimPose* a, const AnimPose* b, float alpha, AnimPose* result);

// blend between three sets of poses
void blend3(size_t numPoses, const AnimPose* a, const AnimPose* b, const AnimPose* c, float* alphas, AnimPose* result);

// blend between four sets of poses
void blend4(size_t numPoses, const AnimPose* a, const AnimPose* b, const AnimPose* c, const AnimPose* d, float* alphas, AnimPose* result);

// additive blending
void blendAdd(size_t numPoses, const AnimPose* a, const AnimPose* b, float alpha, AnimPose* result);

glm::quat averageQuats(size_t numQuats, const glm::quat* quats);

float accumulateTime(float startFrame, float endFrame, float timeScale, float currentFrame, float dt, bool loopFlag,
                     const QString& id, AnimVariantMap& triggersOut);

inline glm::quat safeLerp(const glm::quat& a, const glm::quat& b, float alpha) {
    // adjust signs if necessary
    glm::quat bTemp = b;
    float dot = glm::dot(a, bTemp);
    if (dot < 0.0f) {
        bTemp = -bTemp;
    }
    return glm::normalize(glm::lerp(a, bTemp, alpha));
}

inline glm::quat safeLinearCombine3(const glm::quat& a, const glm::quat& b, const glm::quat& c, float* alphas) {
    // adjust signs for b & c if necessary
    glm::quat bTemp = b;
    float dot = glm::dot(a, bTemp);
    if (dot < 0.0f) {
        bTemp = -bTemp;
    }
    glm::quat cTemp = c;
    dot = glm::dot(a, cTemp);
    if (dot < 0.0f) {
        cTemp = -cTemp;
    }
    return glm::normalize(alphas[0] * a + alphas[1] * bTemp + alphas[2] * cTemp);
}

inline glm::quat safeLinearCombine4(const glm::quat& a, const glm::quat& b, const glm::quat& c, const glm::quat& d, float* alphas) {
    // adjust signs for b, c & d if necessary
    glm::quat bTemp = b;
    float dot = glm::dot(a, bTemp);
    if (dot < 0.0f) {
        bTemp = -bTemp;
    }
    glm::quat cTemp = c;
    dot = glm::dot(a, cTemp);
    if (dot < 0.0f) {
        cTemp = -cTemp;
    }
    glm::quat dTemp = d;
    dot = glm::dot(a, dTemp);
    if (dot < 0.0f) {
        dTemp = -dTemp;
    }
    return glm::normalize(alphas[0] * a + alphas[1] * bTemp + alphas[2] * cTemp + alphas[3] * dTemp);
}

AnimPose boneLookAt(const glm::vec3& target, const AnimPose& bone);

// This will attempt to determine the proper body facing of a characters body
// assumes headRot is z-forward and y-up.
// and returns a bodyRot that is also z-forward and y-up
glm::quat computeBodyFacingFromHead(const glm::quat& headRot, const glm::vec3& up);


// Uses a approximation of a critically damped spring to smooth full AnimPoses.
// It provides seperate timescales for horizontal, vertical and rotation components.
// The timescale is roughly how much time it will take the spring will reach halfway toward it's target.
class CriticallyDampedSpringPoseHelper {
public:
    CriticallyDampedSpringPoseHelper() : _prevPoseValid(false) {}

    void setHorizontalTranslationTimescale(float timescale) {
        _horizontalTranslationTimescale = timescale;
    }
    void setVerticalTranslationTimescale(float timescale) {
        _verticalTranslationTimescale = timescale;
    }
    void setRotationTimescale(float timescale) {
        _rotationTimescale = timescale;
    }

    AnimPose update(const AnimPose& pose, float deltaTime) {
        if (!_prevPoseValid) {
            _prevPose = pose;
            _prevPoseValid = true;
        }

        const float horizontalTranslationAlpha = glm::min(deltaTime / _horizontalTranslationTimescale, 1.0f);
        const float verticalTranslationAlpha = glm::min(deltaTime / _verticalTranslationTimescale, 1.0f);
        const float rotationAlpha = glm::min(deltaTime / _rotationTimescale, 1.0f);

        const float poseY = pose.trans().y;
        AnimPose newPose = _prevPose;
        newPose.trans() = lerp(_prevPose.trans(), pose.trans(), horizontalTranslationAlpha);
        newPose.trans().y = lerp(_prevPose.trans().y, poseY, verticalTranslationAlpha);
        newPose.rot() = safeLerp(_prevPose.rot(), pose.rot(), rotationAlpha);

        _prevPose = newPose;
        _prevPoseValid = true;

        return newPose;
    }

    void teleport(const AnimPose& pose) {
        _prevPoseValid = true;
        _prevPose = pose;
    }

protected:
    AnimPose _prevPose;
    float _horizontalTranslationTimescale { 0.15f };
    float _verticalTranslationTimescale { 0.15f };
    float _rotationTimescale { 0.15f };
    bool _prevPoseValid;
};

class SnapshotBlendPoseHelper {
public:
    SnapshotBlendPoseHelper() : _snapshotValid(false) {}

    void setBlendDuration(float duration) {
        _duration = duration;
    }

    void setSnapshot(const AnimPose& pose) {
        _snapshotValid = true;
        _snapshotPose = pose;
        _timer = _duration;
    }

    AnimPose update(const AnimPose& targetPose, float deltaTime) {
        _timer -= deltaTime;
        if (_timer > 0.0f) {
            float alpha = (_duration - _timer) / _duration;

            // ease in expo
            alpha = 1.0f - powf(2.0f, -10.0f * alpha);

            AnimPose newPose = targetPose;
            newPose.blend(_snapshotPose, alpha);
            return newPose;
        } else {
            return targetPose;
        }
    }

protected:
    AnimPose _snapshotPose;
    float _duration { 1.0f };
    float _timer { 0.0f };
    bool _snapshotValid { false };
};

// returns true if the given point lies inside of the k-dop, specified by shapeInfo & shapePose.
// if the given point does lie within the k-dop, it also returns the amount of displacement necessary to push that point outward
// such that it lies on the surface of the kdop.
bool findPointKDopDisplacement(const glm::vec3& point, const AnimPose& shapePose, const HFMJointShapeInfo& shapeInfo, glm::vec3& displacementOut);

enum EasingType {
    EasingType_Linear,
    EasingType_EaseInSine,
    EasingType_EaseOutSine,
    EasingType_EaseInOutSine,
    EasingType_EaseInQuad,
    EasingType_EaseOutQuad,
    EasingType_EaseInOutQuad,
    EasingType_EaseInCubic,
    EasingType_EaseOutCubic,
    EasingType_EaseInOutCubic,
    EasingType_EaseInQuart,
    EasingType_EaseOutQuart,
    EasingType_EaseInOutQuart,
    EasingType_EaseInQuint,
    EasingType_EaseOutQuint,
    EasingType_EaseInOutQuint,
    EasingType_EaseInExpo,
    EasingType_EaseOutExpo,
    EasingType_EaseInOutExpo,
    EasingType_EaseInCirc,
    EasingType_EaseOutCirc,
    EasingType_EaseInOutCirc,
    EasingType_NumTypes
};

float easingFunc(float alpha, EasingType type);

#endif
