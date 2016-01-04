//
//  GLMHelpersTests.cpp
//  tests/shared/src
//
//  Created by Anthony Thibault on 2015.12.29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLMHelpersTests.h"

#include <NumericalConstants.h>
#include <StreamUtils.h>

#include <../QTestExtensions.h>


QTEST_MAIN(GLMHelpersTests)

void GLMHelpersTests::testEulerDecomposition() {
    // quat to euler and back again....

    const glm::quat ROT_X_90 = glm::angleAxis(PI / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::quat ROT_Y_180 = glm::angleAxis(PI, glm::vec3(0.0f, 1.0, 0.0f));
    const glm::quat ROT_Z_30 = glm::angleAxis(PI / 6.0f, glm::vec3(1.0f, 0.0f, 0.0f));

    const float EPSILON = 0.00001f;

    std::vector<glm::quat> quatVec = {
        glm::quat(),
        ROT_X_90,
        ROT_Y_180,
        ROT_Z_30,
        ROT_X_90 * ROT_Y_180 * ROT_Z_30,
        ROT_X_90 * ROT_Z_30 * ROT_Y_180,
        ROT_Y_180 * ROT_Z_30 * ROT_X_90,
        ROT_Y_180 * ROT_X_90 * ROT_Z_30,
        ROT_Z_30 * ROT_X_90 * ROT_Y_180,
        ROT_Z_30 * ROT_Y_180 * ROT_X_90,
    };

    for (auto& q : quatVec) {
        glm::vec3 euler = safeEulerAngles(q);
        glm::quat r(euler);

        // when the axis and angle are flipped.
        if (glm::dot(q, r) < 0.0f) {
            r = -r;
        }

        QCOMPARE_WITH_ABS_ERROR(q, r, EPSILON);
    }
}


