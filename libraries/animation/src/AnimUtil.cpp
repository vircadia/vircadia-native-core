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

void blend(size_t numPoses, const AnimPose* a, const AnimPose* b, float alpha, AnimPose* result) {
    for (size_t i = 0; i < numPoses; i++) {
        const AnimPose& aPose = a[i];
        const AnimPose& bPose = b[i];
        result[i].scale = lerp(aPose.scale, bPose.scale, alpha);
        result[i].rot = glm::normalize(glm::lerp(aPose.rot, bPose.rot, alpha));
        result[i].trans = lerp(aPose.trans, bPose.trans, alpha);
    }
}
