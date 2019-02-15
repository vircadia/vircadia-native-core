//
//  Created by Bradley Austin Davis on 2018/11/15
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Helpers.h"

#include <atomic>
#include <algorithm>
#include <android/log.h>
#include <VrApi_Helpers.h>

using namespace ovr;

void Fov::extend(const Fov& other) {
    for (size_t i = 0; i < 4; ++i) {
        leftRightUpDown[i] = std::max(leftRightUpDown[i], other.leftRightUpDown[i]);
    }
}

void Fov::extract(const ovrMatrix4f& mat) {
    auto& fs = leftRightUpDown;
    ovrMatrix4f_ExtractFov( &mat, fs, fs + 1, fs + 2, fs + 3);
}

glm::mat4 Fov::withZ(float nearZ, float farZ) const {
    const auto& fs = leftRightUpDown;
    return ovr::toGlm(ovrMatrix4f_CreateProjectionAsymmetricFov(fs[0], fs[1], fs[2], fs[3], nearZ, farZ));
}

glm::mat4 Fov::withZ(const glm::mat4& other) const {
    // FIXME
    return withZ(0.01f, 1000.0f);
}

