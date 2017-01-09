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
#include "GLMHelpers.h"

// TODO: use restrict keyword
// TODO: excellent candidate for simd vectorization.

void blend(size_t numPoses, const AnimPose* a, const AnimPose* b, float alpha, AnimPose* result) {
    for (size_t i = 0; i < numPoses; i++) {
        const AnimPose& aPose = a[i];
        const AnimPose& bPose = b[i];

        // adjust signs if necessary
        const glm::quat& q1 = aPose.rot();
        glm::quat q2 = bPose.rot();
        float dot = glm::dot(q1, q2);
        if (dot < 0.0f) {
            q2 = -q2;
        }

        result[i].scale() = lerp(aPose.scale(), bPose.scale(), alpha);
        result[i].rot() = glm::normalize(glm::lerp(aPose.rot(), q2, alpha));
        result[i].trans() = lerp(aPose.trans(), bPose.trans(), alpha);
    }
}

float accumulateTime(float startFrame, float endFrame, float timeScale, float currentFrame, float dt, bool loopFlag,
                     const QString& id, AnimNode::Triggers& triggersOut) {

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
                    triggersOut.push_back(id + "OnLoop");
                    framesRemaining -= framesTillEnd;
                    frame = clampedStartFrame;
                } else {
                    // anim end
                    triggersOut.push_back(id + "OnDone");
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

