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

glm::quat averageQuats(size_t numQuats, const glm::quat* quats);

float accumulateTime(float startFrame, float endFrame, float timeScale, float currentFrame, float dt, bool loopFlag,
                     const QString& id, AnimNode::Triggers& triggersOut);

inline glm::quat safeLerp(const glm::quat& a, const glm::quat& b, float alpha) {
    // adjust signs if necessary
    glm::quat bTemp = b;
    float dot = glm::dot(a, bTemp);
    if (dot < 0.0f) {
        bTemp = -bTemp;
    }
    return glm::normalize(glm::lerp(a, bTemp, alpha));
}

AnimPose boneLookAt(const glm::vec3& target, const AnimPose& bone);

#endif
