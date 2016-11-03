//
//  ViewFrustumTests.cpp
//  tests/octree/src
//
//  Created by Andrew Meadows on 2016.02.19
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ViewFrustumTests.h"

#include <glm/glm.hpp>

#include <GLMHelpers.h>
#include <NumericalConstants.h>
#include <ViewFrustum.h>

//#include <StreamUtils.h>
#include <../GLMTestUtils.h>
#include <../QTestExtensions.h>


const float ACCEPTABLE_FLOAT_ERROR = 1.0e-6f;
const float ACCEPTABLE_DOT_ERROR = 1.0e-5f;
const float ACCEPTABLE_CLIP_ERROR = 3e-4f;

const glm::vec3 localRight(1.0f, 0.0f, 0.0f);
const glm::vec3 localUp(0.0f, 1.0f, 0.0f);
const glm::vec3 localForward(0.0f, 0.0f, -1.0f);


QTEST_MAIN(ViewFrustumTests)

void ViewFrustumTests::testInit() {
    float aspect = 1.0f;
    float fovX = PI / 2.0f;
    float nearClip = 1.0f;
    float farClip = 100.0f;
    float holeRadius = 10.0f;

    glm::vec3 center = glm::vec3(12.3f, 4.56f, 89.7f);

    float angle = PI / 7.0f;
    glm::vec3 axis = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
    glm::quat rotation = glm::angleAxis(angle, axis);

    ViewFrustum view;
    view.setProjection(glm::perspective(fovX, aspect, nearClip, farClip));
    view.setPosition(center);
    view.setOrientation(rotation);
    view.setCenterRadius(holeRadius);
    view.calculate();

    // check frustum dimensions
    QCOMPARE_WITH_ABS_ERROR(fovX, glm::radians(view.getFieldOfView()), ACCEPTABLE_FLOAT_ERROR);
    QCOMPARE_WITH_ABS_ERROR(aspect, view.getAspectRatio(), ACCEPTABLE_FLOAT_ERROR);
    QCOMPARE_WITH_ABS_ERROR(farClip, view.getFarClip(), ACCEPTABLE_CLIP_ERROR);
    QCOMPARE_WITH_ABS_ERROR(nearClip, view.getNearClip(), ACCEPTABLE_CLIP_ERROR);

    // check transform
    QCOMPARE_WITH_ABS_ERROR(view.getPosition(), center, ACCEPTABLE_FLOAT_ERROR);
    float rotationDot = glm::abs(glm::dot(rotation, view.getOrientation()));
    QCOMPARE_WITH_ABS_ERROR(rotationDot, 1.0f, ACCEPTABLE_DOT_ERROR);

    // check view directions
    glm::vec3 expectedForward = rotation * localForward;
    float forwardDot = glm::dot(expectedForward, view.getDirection());
    QCOMPARE_WITH_ABS_ERROR(forwardDot, 1.0f, ACCEPTABLE_DOT_ERROR);

    glm::vec3 expectedRight = rotation * localRight;
    float rightDot = glm::dot(expectedRight, view.getRight());
    QCOMPARE_WITH_ABS_ERROR(rightDot, 1.0f, ACCEPTABLE_DOT_ERROR);

    glm::vec3 expectedUp = rotation * localUp;
    float upDot = glm::dot(expectedUp, view.getUp());
    QCOMPARE_WITH_ABS_ERROR(upDot, 1.0f, ACCEPTABLE_DOT_ERROR);
}

void ViewFrustumTests::testCubeKeyholeIntersection() {
    float aspect = 1.0f;
    float fovX = PI / 2.0f;
    float fovY = 2.0f * asinf(sinf(0.5f * fovX) / aspect);
    float nearClip = 1.0f;
    float farClip = 100.0f;
    float holeRadius = 10.0f;

    glm::vec3 center = glm::vec3(12.3f, 4.56f, 89.7f);

    float angle = PI / 7.0f;
    glm::vec3 axis = Vectors::UNIT_Y;
    glm::quat rotation = glm::angleAxis(angle, axis);

    ViewFrustum view;
    view.setProjection(glm::perspective(fovX, aspect, nearClip, farClip));
    view.setPosition(center);
    view.setOrientation(rotation);
    view.setCenterRadius(holeRadius);
    view.calculate();

    float delta = 0.1f;
    float deltaAngle = 0.01f;
    glm::quat elevation, swing;
    glm::vec3 cubeCenter, localOffset;

    float cubeScale = 2.68f; // must be much smaller than cubeDistance for small angle approx below
    glm::vec3 halfScaleOffset = 0.5f * glm::vec3(cubeScale);
    float cubeDistance = farClip;
    float cubeBoundingRadius = 0.5f * sqrtf(3.0f) * cubeScale;
    float cubeAngle = cubeBoundingRadius / cubeDistance; // sine of small angles approximation
    AACube cube(center, cubeScale);

    // farPlane
    localOffset = (cubeDistance - cubeBoundingRadius - delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INSIDE);

    localOffset = cubeDistance * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INTERSECT);

    localOffset = (cubeDistance + cubeBoundingRadius + delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::OUTSIDE);

    // nearPlane
    localOffset = (nearClip + 2.0f * cubeBoundingRadius + delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INSIDE);

    localOffset = (nearClip + delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INSIDE);

    // topPlane
    angle = 0.5f * fovY;
    elevation = glm::angleAxis(angle - cubeAngle - deltaAngle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INSIDE);

    elevation = glm::angleAxis(angle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INTERSECT);

    elevation = glm::angleAxis(angle + cubeAngle + deltaAngle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::OUTSIDE);

    // bottom plane
    angle = -0.5f * fovY;
    elevation = glm::angleAxis(angle + cubeAngle + deltaAngle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INSIDE);

    elevation = glm::angleAxis(angle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INTERSECT);

    elevation = glm::angleAxis(angle - cubeAngle - deltaAngle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::OUTSIDE);

    // right plane
    angle = 0.5f * fovX;
    swing = glm::angleAxis(angle - cubeAngle - deltaAngle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INSIDE);

    swing = glm::angleAxis(angle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INTERSECT);

    swing = glm::angleAxis(angle + cubeAngle + deltaAngle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::OUTSIDE);

    // left plane
    angle = -0.5f * fovX;
    swing = glm::angleAxis(angle + cubeAngle + deltaAngle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INSIDE);

    swing = glm::angleAxis(angle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INTERSECT);

    swing = glm::angleAxis(angle - cubeAngle - deltaAngle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::OUTSIDE);

    // central sphere right
    localOffset = (holeRadius - cubeBoundingRadius - delta) * localRight;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INSIDE);

    localOffset = holeRadius * localRight;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INTERSECT);

    localOffset = (holeRadius + cubeBoundingRadius + delta) * localRight;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::OUTSIDE);

    // central sphere up
    localOffset = (holeRadius - cubeBoundingRadius - delta) * localUp;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INSIDE);

    localOffset = holeRadius * localUp;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INTERSECT);

    localOffset = (holeRadius + cubeBoundingRadius + delta) * localUp;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::OUTSIDE);

    // central sphere back
    localOffset = (-holeRadius + cubeBoundingRadius + delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INSIDE);

    localOffset = - holeRadius * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INTERSECT);

    localOffset = (-holeRadius - cubeBoundingRadius - delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::OUTSIDE);

    // central sphere center
    float bigCubeScale = 2.0f * holeRadius / sqrtf(3.0f) - delta;
    cube.setBox(center - glm::vec3(0.5f * bigCubeScale), bigCubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INSIDE); // smaller than sphere

    bigCubeScale = 2.0f * holeRadius / sqrtf(3.0f) + delta;
    cube.setBox(center - glm::vec3(0.5f * bigCubeScale), bigCubeScale);
    QCOMPARE(view.calculateCubeKeyholeIntersection(cube), ViewFrustum::INTERSECT); // larger than sphere
}

void ViewFrustumTests::testCubeFrustumIntersection() {
    float aspect = 1.0f;
    float fovX = PI / 2.0f;
    float fovY = 2.0f * asinf(sinf(0.5f * fovX) / aspect);
    float nearClip = 1.0f;
    float farClip = 100.0f;
    float holeRadius = 10.0f;

    glm::vec3 center = glm::vec3(12.3f, 4.56f, 89.7f);

    float angle = PI / 7.0f;
    glm::vec3 axis = Vectors::UNIT_Y;
    glm::quat rotation = glm::angleAxis(angle, axis);

    ViewFrustum view;
    view.setProjection(glm::perspective(fovX, aspect, nearClip, farClip));
    view.setPosition(center);
    view.setOrientation(rotation);
    view.setCenterRadius(holeRadius);
    view.calculate();

    float delta = 0.1f;
    float deltaAngle = 0.01f;
    glm::quat elevation, swing;
    glm::vec3 cubeCenter, localOffset;

    float cubeScale = 2.68f; // must be much smaller than cubeDistance for small angle approx below
    glm::vec3 halfScaleOffset = 0.5f * glm::vec3(cubeScale);
    float cubeDistance = farClip;
    float cubeBoundingRadius = 0.5f * sqrtf(3.0f) * cubeScale;
    float cubeAngle = cubeBoundingRadius / cubeDistance; // sine of small angles approximation
    AACube cube(center, cubeScale);

    // farPlane
    localOffset = (cubeDistance - cubeBoundingRadius - delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::INSIDE);

    localOffset = cubeDistance * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::INTERSECT);

    localOffset = (cubeDistance + cubeBoundingRadius + delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::OUTSIDE);

    // nearPlane
    localOffset = (nearClip + 2.0f * cubeBoundingRadius + delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::INSIDE);

    localOffset = (nearClip + delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::INTERSECT);

    localOffset = (nearClip - cubeBoundingRadius - delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::OUTSIDE);

    // topPlane
    angle = 0.5f * fovY;
    elevation = glm::angleAxis(angle - cubeAngle - deltaAngle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::INSIDE);

    elevation = glm::angleAxis(angle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::INTERSECT);

    elevation = glm::angleAxis(angle + cubeAngle + deltaAngle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::OUTSIDE);

    // bottom plane
    angle = -0.5f * fovY;
    elevation = glm::angleAxis(angle + cubeAngle + deltaAngle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::INSIDE);

    elevation = glm::angleAxis(angle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::INTERSECT);

    elevation = glm::angleAxis(angle - cubeAngle - deltaAngle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::OUTSIDE);

    // right plane
    angle = 0.5f * fovX;
    swing = glm::angleAxis(angle - cubeAngle - deltaAngle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::INSIDE);

    swing = glm::angleAxis(angle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::INTERSECT);

    swing = glm::angleAxis(angle + cubeAngle + deltaAngle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::OUTSIDE);

    // left plane
    angle = -0.5f * fovX;
    swing = glm::angleAxis(angle + cubeAngle + deltaAngle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::INSIDE);

    swing = glm::angleAxis(angle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::INTERSECT);

    swing = glm::angleAxis(angle - cubeAngle - deltaAngle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.calculateCubeFrustumIntersection(cube), ViewFrustum::OUTSIDE);
}

void ViewFrustumTests::testPointIntersectsFrustum() {
    float aspect = 1.0f;
    float fovX = PI / 2.0f;
    float fovY = 2.0f * asinf(sinf(0.5f * fovX) / aspect);
    float nearClip = 1.0f;
    float farClip = 100.0f;
    float holeRadius = 10.0f;

    glm::vec3 center = glm::vec3(12.3f, 4.56f, 89.7f);

    float angle = PI / 7.0f;
    glm::vec3 axis = Vectors::UNIT_Y;
    glm::quat rotation = glm::angleAxis(angle, axis);

    ViewFrustum view;
    view.setProjection(glm::perspective(fovX, aspect, nearClip, farClip));
    view.setPosition(center);
    view.setOrientation(rotation);
    view.setCenterRadius(holeRadius);
    view.calculate();

    float delta = 0.1f;
    float deltaAngle = 0.01f;
    glm::quat elevation, swing;
    glm::vec3 point, localOffset;
    float pointDistance = farClip;

    // farPlane
    localOffset = (pointDistance - delta) * localForward;
    point = center + rotation * localOffset;
    QCOMPARE(view.pointIntersectsFrustum(point), true); // inside

    localOffset = (pointDistance + delta) * localForward;
    point = center + rotation * localOffset;
    QCOMPARE(view.pointIntersectsFrustum(point), false); // outside

    // nearPlane
    localOffset = (nearClip + delta) * localForward;
    point = center + rotation * localOffset;
    QCOMPARE(view.pointIntersectsFrustum(point), true); // inside

    localOffset = (nearClip - delta) * localForward;
    point = center + rotation * localOffset;
    QCOMPARE(view.pointIntersectsFrustum(point), false); // outside

    // topPlane
    angle = 0.5f * fovY;
    elevation = glm::angleAxis(angle - deltaAngle, localRight);
    localOffset = elevation * (pointDistance * localForward);
    point = center + rotation * localOffset;
    QCOMPARE(view.pointIntersectsFrustum(point), true); // inside

    elevation = glm::angleAxis(angle + deltaAngle, localRight);
    localOffset = elevation * (pointDistance * localForward);
    point = center + rotation * localOffset;
    QCOMPARE(view.pointIntersectsFrustum(point), false); // outside

    // bottom plane
    angle = -0.5f * fovY;
    elevation = glm::angleAxis(angle + deltaAngle, localRight);
    localOffset = elevation * (pointDistance * localForward);
    point = center + rotation * localOffset;
    QCOMPARE(view.pointIntersectsFrustum(point), true); // inside

    elevation = glm::angleAxis(angle - deltaAngle, localRight);
    localOffset = elevation * (pointDistance * localForward);
    point = center + rotation * localOffset;
    QCOMPARE(view.pointIntersectsFrustum(point), false); // outside

    // right plane
    angle = 0.5f * fovX;
    swing = glm::angleAxis(angle - deltaAngle, localUp);
    localOffset = swing * (pointDistance * localForward);
    point = center + rotation * localOffset;
    QCOMPARE(view.pointIntersectsFrustum(point), true); // inside

    swing = glm::angleAxis(angle + deltaAngle, localUp);
    localOffset = swing * (pointDistance * localForward);
    point = center + rotation * localOffset;
    QCOMPARE(view.pointIntersectsFrustum(point), false); // outside

    // left plane
    angle = -0.5f * fovX;
    swing = glm::angleAxis(angle + deltaAngle, localUp);
    localOffset = swing * (pointDistance * localForward);
    point = center + rotation * localOffset;
    QCOMPARE(view.pointIntersectsFrustum(point), true); // inside

    swing = glm::angleAxis(angle - deltaAngle, localUp);
    localOffset = swing * (pointDistance * localForward);
    point = center + rotation * localOffset;
    QCOMPARE(view.pointIntersectsFrustum(point), false); // outside
}

void ViewFrustumTests::testSphereIntersectsFrustum() {
    float aspect = 1.0f;
    float fovX = PI / 2.0f;
    float fovY = 2.0f * asinf(sinf(0.5f * fovX) / aspect);
    float nearClip = 1.0f;
    float farClip = 100.0f;
    float holeRadius = 10.0f;

    glm::vec3 center = glm::vec3(12.3f, 4.56f, 89.7f);

    float angle = PI / 7.0f;
    glm::vec3 axis = Vectors::UNIT_Y;
    glm::quat rotation = glm::angleAxis(angle, axis);

    ViewFrustum view;
    view.setProjection(glm::perspective(fovX, aspect, nearClip, farClip));
    view.setPosition(center);
    view.setOrientation(rotation);
    view.setCenterRadius(holeRadius);
    view.calculate();

    float delta = 0.1f;
    float deltaAngle = 0.01f;
    glm::quat elevation, swing;
    glm::vec3 sphereCenter, localOffset;

    float sphereRadius = 2.68f; // must be much smaller than sphereDistance for small angle approx below
    float sphereDistance = farClip;
    float sphereAngle = sphereRadius / sphereDistance; // sine of small angles approximation

    // farPlane
    localOffset = (sphereDistance - sphereRadius - delta) * localForward;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), true); // inside

    localOffset = (sphereDistance + sphereRadius - delta) * localForward;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), true); // straddle

    localOffset = (sphereDistance + sphereRadius + delta) * localForward;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), false); // outside

    // nearPlane
    localOffset = (nearClip + 2.0f * sphereRadius + delta) * localForward;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), true); // inside

    localOffset = (nearClip - sphereRadius + delta) * localForward;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), true); // straddle

    localOffset = (nearClip - sphereRadius - delta) * localForward;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), false); // outside

    // topPlane
    angle = 0.5f * fovY - sphereAngle;
    elevation = glm::angleAxis(angle - deltaAngle, localRight);
    localOffset = elevation * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), true); // inside

    angle = 0.5f * fovY + sphereAngle;
    elevation = glm::angleAxis(angle - deltaAngle, localRight);
    localOffset = elevation * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), true); // straddle

    elevation = glm::angleAxis(angle + deltaAngle, localRight);
    localOffset = elevation * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), false); // outside

    // bottom plane
    angle = -0.5f * fovY + sphereAngle;
    elevation = glm::angleAxis(angle + deltaAngle, localRight);
    localOffset = elevation * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), true); // inside

    angle = -0.5f * fovY - sphereAngle;
    elevation = glm::angleAxis(angle + deltaAngle, localRight);
    localOffset = elevation * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), true); // straddle

    elevation = glm::angleAxis(angle - deltaAngle, localRight);
    localOffset = elevation * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), false); // outside

    // right plane
    angle = 0.5f * fovX - sphereAngle;
    swing = glm::angleAxis(angle - deltaAngle, localUp);
    localOffset = swing * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), true); // inside

    angle = 0.5f * fovX + sphereAngle;
    swing = glm::angleAxis(angle - deltaAngle, localUp);
    localOffset = swing * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), true); // straddle

    swing = glm::angleAxis(angle + deltaAngle, localUp);
    localOffset = swing * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), false); // outside

    // left plane
    angle = -0.5f * fovX + sphereAngle;
    swing = glm::angleAxis(angle + deltaAngle, localUp);
    localOffset = swing * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), true); // inside

    angle = -0.5f * fovX - sphereAngle;
    swing = glm::angleAxis(angle + deltaAngle, localUp);
    localOffset = swing * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), true); // straddle

    swing = glm::angleAxis(angle - sphereAngle - deltaAngle, localUp);
    localOffset = swing * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsFrustum(sphereCenter, sphereRadius), false); // outside
}

void ViewFrustumTests::testBoxIntersectsFrustum() {
    float aspect = 1.0f;
    float fovX = PI / 2.0f;
    float fovY = 2.0f * asinf(sinf(0.5f * fovX) / aspect);
    float nearClip = 1.0f;
    float farClip = 100.0f;
    float holeRadius = 10.0f;

    glm::vec3 center = glm::vec3(12.3f, 4.56f, 89.7f);

    float angle = PI / 7.0f;
    glm::vec3 axis = Vectors::UNIT_Y;
    glm::quat rotation = glm::angleAxis(angle, axis);

    ViewFrustum view;
    view.setProjection(glm::perspective(fovX, aspect, nearClip, farClip));
    view.setPosition(center);
    view.setOrientation(rotation);
    view.setCenterRadius(holeRadius);
    view.calculate();

    float delta = 0.1f;
    float deltaAngle = 0.01f;
    glm::quat elevation, swing;
    glm::vec3 boxCenter, localOffset;

    glm::vec3 boxScale = glm::vec3(2.68f, 1.78f, 0.431f);
    float boxDistance = farClip;
    float boxBoundingRadius = 0.5f * glm::length(boxScale);
    float boxAngle = boxBoundingRadius / boxDistance; // sine of small angles approximation
    AABox box(center, boxScale);

    // farPlane
    localOffset = (boxDistance - boxBoundingRadius - delta) * localForward;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), true); // inside

    localOffset = boxDistance * localForward;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), true); // straddle

    localOffset = (boxDistance + boxBoundingRadius + delta) * localForward;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), false); // outside

    // nearPlane
    localOffset = (nearClip + 2.0f * boxBoundingRadius + delta) * localForward;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), true); // inside

    localOffset = nearClip * localForward;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), true); // straddle

    localOffset = (nearClip - boxBoundingRadius - delta) * localForward;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), false); // outside

    // topPlane
    angle = 0.5f * fovY;
    elevation = glm::angleAxis(angle - boxAngle - deltaAngle, localRight);
    localOffset = elevation * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), true); // inside

    elevation = glm::angleAxis(angle, localRight);
    localOffset = elevation * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), true); // straddle

    elevation = glm::angleAxis(angle + boxAngle + deltaAngle, localRight);
    localOffset = elevation * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), false); // outside

    // bottom plane
    angle = -0.5f * fovY;
    elevation = glm::angleAxis(angle + boxAngle + deltaAngle, localRight);
    localOffset = elevation * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), true); // inside

    elevation = glm::angleAxis(angle, localRight);
    localOffset = elevation * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), true); // straddle

    elevation = glm::angleAxis(angle - boxAngle - deltaAngle, localRight);
    localOffset = elevation * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), false); // outside

    // right plane
    angle = 0.5f * fovX;
    swing = glm::angleAxis(angle - boxAngle - deltaAngle, localUp);
    localOffset = swing * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), true); // inside

    swing = glm::angleAxis(angle, localUp);
    localOffset = swing * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), true); // straddle

    swing = glm::angleAxis(angle + boxAngle + deltaAngle, localUp);
    localOffset = swing * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), false); // outside

    // left plane
    angle = -0.5f * fovX;
    swing = glm::angleAxis(angle + boxAngle + deltaAngle, localUp);
    localOffset = swing * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), true); // inside

    swing = glm::angleAxis(angle, localUp);
    localOffset = swing * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), true); // straddle

    swing = glm::angleAxis(angle - boxAngle - deltaAngle, localUp);
    localOffset = swing * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - 0.5f * boxScale, boxScale);
    QCOMPARE(view.boxIntersectsFrustum(box), false); // outside
}

void ViewFrustumTests::testSphereIntersectsKeyhole() {
    float aspect = 1.0f;
    float fovX = PI / 2.0f;
    float fovY = 2.0f * asinf(sinf(0.5f * fovX) / aspect);
    float nearClip = 1.0f;
    float farClip = 100.0f;
    float holeRadius = 10.0f;

    glm::vec3 center = glm::vec3(12.3f, 4.56f, 89.7f);

    float angle = PI / 7.0f;
    glm::vec3 axis = Vectors::UNIT_Y;
    glm::quat rotation = glm::angleAxis(angle, axis);

    ViewFrustum view;
    view.setProjection(glm::perspective(fovX, aspect, nearClip, farClip));
    view.setPosition(center);
    view.setOrientation(rotation);
    view.setCenterRadius(holeRadius);
    view.calculate();

    float delta = 0.1f;
    float deltaAngle = 0.01f;
    glm::quat elevation, swing;
    glm::vec3 sphereCenter, localOffset;

    float sphereRadius = 2.68f; // must be much smaller than sphereDistance for small angle approx below
    float sphereDistance = farClip;
    float sphereAngle = sphereRadius / sphereDistance; // sine of small angles approximation

    // farPlane
    localOffset = (sphereDistance - sphereRadius - delta) * localForward;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // inside

    localOffset = sphereDistance * localForward;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // straddle

    localOffset = (sphereDistance + sphereRadius + delta) * localForward;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), false); // outside

    // nearPlane
    localOffset = (nearClip + 2.0f * sphereRadius + delta) * localForward;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // inside

    localOffset = (nearClip - sphereRadius + delta) * localForward;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // straddle

    localOffset = (nearClip - sphereRadius - delta) * localForward;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // touches central sphere

    // topPlane
    angle = 0.5f * fovY - sphereAngle;
    elevation = glm::angleAxis(angle - deltaAngle, localRight);
    localOffset = elevation * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // inside

    angle = 0.5f * fovY + sphereAngle;
    elevation = glm::angleAxis(angle - deltaAngle, localRight);
    localOffset = elevation * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // straddle

    elevation = glm::angleAxis(angle + deltaAngle, localRight);
    localOffset = elevation * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), false); // outside

    // bottom plane
    angle = -0.5f * fovY + sphereAngle;
    elevation = glm::angleAxis(angle + deltaAngle, localRight);
    localOffset = elevation * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // inside

    angle = -0.5f * fovY - sphereAngle;
    elevation = glm::angleAxis(angle + deltaAngle, localRight);
    localOffset = elevation * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // straddle

    elevation = glm::angleAxis(angle - deltaAngle, localRight);
    localOffset = elevation * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), false); // outside

    // right plane
    angle = 0.5f * fovX - sphereAngle;
    swing = glm::angleAxis(angle - deltaAngle, localUp);
    localOffset = swing * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // inside

    angle = 0.5f * fovX + sphereAngle;
    swing = glm::angleAxis(angle - deltaAngle, localUp);
    localOffset = swing * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // straddle

    swing = glm::angleAxis(angle + deltaAngle, localUp);
    localOffset = swing * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), false); // outside

    // left plane
    angle = -0.5f * fovX + sphereAngle;
    swing = glm::angleAxis(angle + deltaAngle, localUp);
    localOffset = swing * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // inside

    angle = -0.5f * fovX - sphereAngle;
    swing = glm::angleAxis(angle + deltaAngle, localUp);
    localOffset = swing * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // straddle

    swing = glm::angleAxis(angle - sphereAngle - deltaAngle, localUp);
    localOffset = swing * (sphereDistance * localForward);
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), false); // outside

    // central sphere right
    localOffset = (holeRadius - sphereRadius - delta) * localRight;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // inside right

    localOffset = holeRadius * localRight;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // straddle right

    localOffset = (holeRadius + sphereRadius + delta) * localRight;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), false); // outside right

    // central sphere up
    localOffset = (holeRadius - sphereRadius - delta) * localUp;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // inside up

    localOffset = holeRadius * localUp;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // straddle up

    localOffset = (holeRadius + sphereRadius + delta) * localUp;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), false); // outside up

    // central sphere back
    localOffset = (-holeRadius + sphereRadius + delta) * localForward;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // inside back

    localOffset = - holeRadius * localForward;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), true); // straddle back

    localOffset = (-holeRadius - sphereRadius - delta) * localForward;
    sphereCenter = center + rotation * localOffset;
    QCOMPARE(view.sphereIntersectsKeyhole(sphereCenter, sphereRadius), false); // outside back
}

void ViewFrustumTests::testCubeIntersectsKeyhole() {
    float aspect = 1.0f;
    float fovX = PI / 2.0f;
    float fovY = 2.0f * asinf(sinf(0.5f * fovX) / aspect);
    float nearClip = 1.0f;
    float farClip = 100.0f;
    float holeRadius = 10.0f;

    glm::vec3 center = glm::vec3(12.3f, 4.56f, 89.7f);

    float angle = PI / 7.0f;
    glm::vec3 axis = Vectors::UNIT_Y;
    glm::quat rotation = glm::angleAxis(angle, axis);

    ViewFrustum view;
    view.setProjection(glm::perspective(fovX, aspect, nearClip, farClip));
    view.setPosition(center);
    view.setOrientation(rotation);
    view.setCenterRadius(holeRadius);
    view.calculate();

    float delta = 0.1f;
    float deltaAngle = 0.01f;
    glm::quat elevation, swing;
    glm::vec3 cubeCenter, localOffset;

    float cubeScale = 2.68f; // must be much smaller than cubeDistance for small angle approx below
    glm::vec3 halfScaleOffset = 0.5f * glm::vec3(cubeScale);
    float cubeDistance = farClip;
    float cubeBoundingRadius = 0.5f * sqrtf(3.0f) * cubeScale;
    float cubeAngle = cubeBoundingRadius / cubeDistance; // sine of small angles approximation
    AACube cube(center, cubeScale);

    // farPlane
    localOffset = (cubeDistance - cubeBoundingRadius - delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true);

    localOffset = cubeDistance * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true);

    localOffset = (cubeDistance + cubeBoundingRadius + delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), false);

    // nearPlane
    localOffset = (nearClip + 2.0f * cubeBoundingRadius + delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // inside

    localOffset = (nearClip + delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // straddle

    localOffset = (nearClip - cubeBoundingRadius - delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // touches centeral sphere

    // topPlane
    angle = 0.5f * fovY;
    elevation = glm::angleAxis(angle - cubeAngle - deltaAngle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // inside

    elevation = glm::angleAxis(angle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // straddle

    elevation = glm::angleAxis(angle + cubeAngle + deltaAngle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), false); // outside

    // bottom plane
    angle = -0.5f * fovY;
    elevation = glm::angleAxis(angle + cubeAngle + deltaAngle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // inside

    elevation = glm::angleAxis(angle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // straddle

    elevation = glm::angleAxis(angle - cubeAngle - deltaAngle, localRight);
    localOffset = elevation * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), false); // outside

    // right plane
    angle = 0.5f * fovX;
    swing = glm::angleAxis(angle - cubeAngle - deltaAngle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // inside

    swing = glm::angleAxis(angle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // straddle

    swing = glm::angleAxis(angle + cubeAngle + deltaAngle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), false); // outside

    // left plane
    angle = -0.5f * fovX;
    swing = glm::angleAxis(angle + cubeAngle + deltaAngle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // inside

    swing = glm::angleAxis(angle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // straddle

    swing = glm::angleAxis(angle - cubeAngle - deltaAngle, localUp);
    localOffset = swing * (cubeDistance * localForward);
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), false); // outside

    // central sphere right
    localOffset = (holeRadius - cubeBoundingRadius - delta) * localRight;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // inside right

    localOffset = holeRadius * localRight;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // straddle right

    localOffset = (holeRadius + cubeBoundingRadius + delta) * localRight;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), false); // outside right

    // central sphere up
    localOffset = (holeRadius - cubeBoundingRadius - delta) * localUp;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // inside up

    localOffset = holeRadius * localUp;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // straddle up

    localOffset = (holeRadius + cubeBoundingRadius + delta) * localUp;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), false); // outside up

    // central sphere back
    localOffset = (-holeRadius + cubeBoundingRadius + delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // inside back

    localOffset = - holeRadius * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), true); // straddle back

    localOffset = (-holeRadius - cubeBoundingRadius - delta) * localForward;
    cubeCenter = center + rotation * localOffset;
    cube.setBox(cubeCenter - halfScaleOffset, cubeScale);
    QCOMPARE(view.cubeIntersectsKeyhole(cube), false); // outside back
}

void ViewFrustumTests::testBoxIntersectsKeyhole() {
    float aspect = 1.0f;
    float fovX = PI / 2.0f;
    float fovY = 2.0f * asinf(sinf(0.5f * fovX) / aspect);
    float nearClip = 1.0f;
    float farClip = 100.0f;
    float holeRadius = 10.0f;

    glm::vec3 center = glm::vec3(12.3f, 4.56f, 89.7f);

    float angle = PI / 7.0f;
    glm::vec3 axis = Vectors::UNIT_Y;
    glm::quat rotation = glm::angleAxis(angle, axis);

    ViewFrustum view;
    view.setProjection(glm::perspective(fovX, aspect, nearClip, farClip));
    view.setPosition(center);
    view.setOrientation(rotation);
    view.setCenterRadius(holeRadius);
    view.calculate();

    float delta = 0.1f;
    float deltaAngle = 0.01f;
    glm::quat elevation, swing;
    glm::vec3 boxCenter, localOffset;

    glm::vec3 boxScale = glm::vec3(2.68f, 1.78f, 0.431f); // sides must be much smaller than boxDistance
    glm::vec3 halfScaleOffset = 0.5f * boxScale;
    float boxDistance = farClip;
    float boxBoundingRadius = 0.5f * glm::length(boxScale);
    float boxAngle = boxBoundingRadius / boxDistance; // sine of small angles approximation
    AABox box(center, boxScale);

    // farPlane
    localOffset = (boxDistance - boxBoundingRadius - delta) * localForward;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true);

    localOffset = boxDistance * localForward;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true);

    localOffset = (boxDistance + boxBoundingRadius + delta) * localForward;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), false);

    // nearPlane
    localOffset = (nearClip + 2.0f * boxBoundingRadius + delta) * localForward;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // inside

    localOffset = (nearClip + delta) * localForward;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // straddle

    localOffset = (nearClip - boxBoundingRadius - delta) * localForward;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // touches centeral sphere

    // topPlane
    angle = 0.5f * fovY;
    elevation = glm::angleAxis(angle - boxAngle - deltaAngle, localRight);
    localOffset = elevation * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // inside

    elevation = glm::angleAxis(angle, localRight);
    localOffset = elevation * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // straddle

    elevation = glm::angleAxis(angle + boxAngle + deltaAngle, localRight);
    localOffset = elevation * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), false); // outside

    // bottom plane
    angle = -0.5f * fovY;
    elevation = glm::angleAxis(angle + boxAngle + deltaAngle, localRight);
    localOffset = elevation * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // inside

    elevation = glm::angleAxis(angle, localRight);
    localOffset = elevation * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // straddle

    elevation = glm::angleAxis(angle - boxAngle - deltaAngle, localRight);
    localOffset = elevation * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), false); // outside

    // right plane
    angle = 0.5f * fovX;
    swing = glm::angleAxis(angle - boxAngle - deltaAngle, localUp);
    localOffset = swing * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // inside

    swing = glm::angleAxis(angle, localUp);
    localOffset = swing * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // straddle

    swing = glm::angleAxis(angle + boxAngle + deltaAngle, localUp);
    localOffset = swing * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), false); // outside

    // left plane
    angle = -0.5f * fovX;
    swing = glm::angleAxis(angle + boxAngle + deltaAngle, localUp);
    localOffset = swing * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // inside

    swing = glm::angleAxis(angle, localUp);
    localOffset = swing * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // straddle

    swing = glm::angleAxis(angle - boxAngle - deltaAngle, localUp);
    localOffset = swing * (boxDistance * localForward);
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), false); // outside

    // central sphere right
    localOffset = (holeRadius - boxBoundingRadius - delta) * localRight;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // inside right

    localOffset = holeRadius * localRight;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // straddle right

    localOffset = (holeRadius + boxBoundingRadius + delta) * localRight;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), false); // outside right

    // central sphere up
    localOffset = (holeRadius - boxBoundingRadius - delta) * localUp;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // inside up

    localOffset = holeRadius * localUp;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // straddle up

    localOffset = (holeRadius + boxBoundingRadius + delta) * localUp;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), false); // outside up

    // central sphere back
    localOffset = (-holeRadius + boxBoundingRadius + delta) * localForward;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // inside back

    localOffset = - holeRadius * localForward;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), true); // straddle back

    localOffset = (-holeRadius - boxBoundingRadius - delta) * localForward;
    boxCenter = center + rotation * localOffset;
    box.setBox(boxCenter - halfScaleOffset, boxScale);
    QCOMPARE(view.boxIntersectsKeyhole(box), false); // outside back
}
