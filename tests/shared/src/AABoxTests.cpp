//
//  AABoxTests.cpp
//  tests/shared/src
//
//  Created by Andrew Meadows on 2016.02.19
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>

#include "AABoxTests.h"

#include <GLMHelpers.h>
#include <NumericalConstants.h>
#include <StreamUtils.h>

#include <../GLMTestUtils.h>
#include <../QTestExtensions.h>


QTEST_MAIN(AABoxTests)

void AABoxTests::testCtorsAndSetters() {
    const glm::vec3 corner(1.23f, 4.56f, 7.89f);
    const glm::vec3 scale(2.34f, 7.53f, 9.14f);

    // test ctor
    AABox box(corner, scale);
    QCOMPARE_WITH_ABS_ERROR(box.getCorner(), corner, EPSILON);
    QCOMPARE_WITH_ABS_ERROR(box.getScale(), scale, EPSILON);

    // test copy ctor
    AABox copyBox(box);
    QCOMPARE_WITH_ABS_ERROR(copyBox.getCorner(), corner, EPSILON);
    QCOMPARE_WITH_ABS_ERROR(copyBox.getScale(), scale, EPSILON);

    // test setBox()
    const glm::vec3 newCorner(9.87f, 6.54f, 3.21f);
    const glm::vec3 newScale = glm::vec3(4.32f, 8.95f, 10.31f);
    box.setBox(newCorner, newScale);
    QCOMPARE_WITH_ABS_ERROR(box.getCorner(), newCorner, EPSILON);
    QCOMPARE_WITH_ABS_ERROR(box.getScale(), newScale, EPSILON);

    // test misc
    QCOMPARE_WITH_ABS_ERROR(newCorner, box.getMinimumPoint(), EPSILON);

    glm::vec3 expectedMaxCorner = newCorner + glm::vec3(newScale);
    QCOMPARE_WITH_ABS_ERROR(expectedMaxCorner, box.getMaximumPoint(), EPSILON);

    glm::vec3 expectedCenter = newCorner + glm::vec3(0.5f * newScale);
    QCOMPARE_WITH_ABS_ERROR(expectedCenter, box.calcCenter(), EPSILON);
}

void AABoxTests::testContainsPoint() {
    const glm::vec3 corner(4.56f, 7.89f, -1.35f);
    const glm::vec3 scale(2.34f, 7.53f, 9.14f);
    AABox box(corner, scale);

    float delta = 0.00001f;
    glm::vec3 center = box.calcCenter();
    QCOMPARE(box.contains(center), true);

    for (int i = 0; i < 3; ++i) {
        glm::vec3 halfScale = Vectors::ZERO;
        halfScale[i] = 0.5f * scale[i];
        glm::vec3 deltaOffset = Vectors::ZERO;
        deltaOffset[i] = delta;

        QCOMPARE(box.contains(center + halfScale + deltaOffset), false); // outside +face
        QCOMPARE(box.contains(center + halfScale - deltaOffset), true); // inside +face
        QCOMPARE(box.contains(center - halfScale + deltaOffset), true); // inside -face
        QCOMPARE(box.contains(center - halfScale - deltaOffset), false); // outside -face
    }
}

void AABoxTests::testTouchesSphere() {
    glm::vec3 corner(-4.56f, 7.89f, -1.35f);
    float scale = 1.23f;
    AABox box(corner, scale);

    float delta = 0.00001f;
    glm::vec3 cubeCenter = box.calcCenter();
    float sphereRadius = 0.468f;

    for (int i = 0; i < 3; ++i) {
        int j = (i + 1) % 3;
        int k = (j + 1) % 3;

        { // faces
            glm::vec3 scaleOffset = Vectors::ZERO;
            scaleOffset[i] = 0.5f * scale + sphereRadius;

            glm::vec3 deltaOffset = Vectors::ZERO;
            deltaOffset[i] = delta;

            // outside +face
            glm::vec3 sphereCenter = cubeCenter + scaleOffset + deltaOffset;
            QCOMPARE(box.touchesSphere(sphereCenter, sphereRadius), false);

            // inside +face
            sphereCenter = cubeCenter + scaleOffset - deltaOffset;
            QCOMPARE(box.touchesSphere(sphereCenter, sphereRadius), true);

            // inside -face
            sphereCenter = cubeCenter - scaleOffset + deltaOffset;
            QCOMPARE(box.touchesSphere(sphereCenter, sphereRadius), true);

            // outside -face
            sphereCenter = cubeCenter - scaleOffset - deltaOffset;
            QCOMPARE(box.touchesSphere(sphereCenter, sphereRadius), false);
        }

        { // edges
            glm::vec3 edgeOffset = Vectors::ZERO;
            edgeOffset[i] = 0.5f * scale;
            edgeOffset[j] = 0.5f * scale;
            glm::vec3 edgeDirection = glm::normalize(edgeOffset);
            glm::vec3 sphereCenter;

            // inside ij
            sphereCenter = cubeCenter + edgeOffset + (sphereRadius - delta) * edgeDirection;
            QCOMPARE(box.touchesSphere(sphereCenter, sphereRadius), true);
            sphereCenter = cubeCenter - edgeOffset - (sphereRadius - delta) * edgeDirection;
            QCOMPARE(box.touchesSphere(sphereCenter, sphereRadius), true);

            // outside ij
            sphereCenter = cubeCenter + edgeOffset + (sphereRadius + delta) * edgeDirection;
            QCOMPARE(box.touchesSphere(sphereCenter, sphereRadius), false);
            sphereCenter = cubeCenter - edgeOffset - (sphereRadius + delta) * edgeDirection;
            QCOMPARE(box.touchesSphere(sphereCenter, sphereRadius), false);

            edgeOffset[j] = 0.0f;
            edgeOffset[k] = 0.5f * scale;
            edgeDirection = glm::normalize(edgeOffset);

            // inside ik
            sphereCenter = cubeCenter + edgeOffset + (sphereRadius - delta) * edgeDirection;
            QCOMPARE(box.touchesSphere(sphereCenter, sphereRadius), true);
            sphereCenter = cubeCenter - edgeOffset - (sphereRadius - delta) * edgeDirection;
            QCOMPARE(box.touchesSphere(sphereCenter, sphereRadius), true);

            // outside ik
            sphereCenter = cubeCenter + edgeOffset + (sphereRadius + delta) * edgeDirection;
            QCOMPARE(box.touchesSphere(sphereCenter, sphereRadius), false);
            sphereCenter = cubeCenter - edgeOffset - (sphereRadius + delta) * edgeDirection;
            QCOMPARE(box.touchesSphere(sphereCenter, sphereRadius), false);
        }
    }
}

void AABoxTests::testScale() {
    AABox box1(glm::vec3(2.0f), glm::vec3(1.0f));
    QCOMPARE(box1.contains(glm::vec3(0.0f)), false);
    box1.scale(glm::vec3(10.0f));
    QCOMPARE(box1.contains(glm::vec3(0.0f)), false);
    QCOMPARE(box1.contains(glm::vec3(2.0f * 10.0f)), true);

    AABox box2(glm::vec3(2.0f), glm::vec3(1.0f));
    QCOMPARE(box2.contains(glm::vec3(0.0f)), false);
    box2.embiggen(glm::vec3(10.0f));
    QCOMPARE(box2.contains(glm::vec3(0.0f)), true);

    AABox box3;
    box3 += glm::vec3(0.0f, 0.0f, 0.0f);
    box3 += glm::vec3(1.0f, 1.0f, 1.0f);
    box3 += glm::vec3(-1.0f, -1.0f, -1.0f);
    QCOMPARE(box3.contains(glm::vec3(0.5f, 0.5f, 0.5f)), true);
}
