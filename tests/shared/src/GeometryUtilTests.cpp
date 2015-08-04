//
//  GeometryUtilTests.cpp
//  tests/physics/src
//
//  Created by Andrew Meadows on 2015.07.27
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>

#include "GeometryUtilTests.h"

#include <GeometryUtil.h>
#include <NumericalConstants.h>
#include <StreamUtils.h>

#include <../QTestExtensions.h>


QTEST_MAIN(GeometryUtilTests)

float getErrorDifference(const float& a, const float& b) {
    return fabsf(a - b);
}

float getErrorDifference(const glm::vec3& a, const glm::vec3& b) {
    return glm::distance(a, b);
}

void GeometryUtilTests::testLocalRayRectangleIntersection() {
    glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
    glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
    glm::vec3 zAxis(0.0f, 0.0f, 1.0f);

    // create a rectangle in the local frame on the XY plane with normal along -zAxis
    // (this is the assumed local orientation of the rectangle in findRayRectangleIntersection())
    glm::vec2 rectDimensions(3.0f, 5.0f);
    glm::vec3 rectCenter(0.0f, 0.0f, 0.0f);
    glm::quat rectRotation = glm::quat(); // identity

    // create points for generating rays that hit the plane and don't
    glm::vec3 rayStart(1.0f, 2.0f, 3.0f);
    float delta = 0.1f;

    { // verify hit
        glm::vec3 rayEnd = rectCenter + rectRotation * ((0.5f * rectDimensions.x - delta) * xAxis);
        glm::vec3 rayHitDirection = glm::normalize(rayEnd - rayStart);
        float expectedDistance = glm::length(rayEnd - rayStart);

        float distance = FLT_MAX;
        bool hit = findRayRectangleIntersection(rayStart, rayHitDirection, rectRotation, rectCenter, rectDimensions, distance);
        QCOMPARE(hit, true);
        QCOMPARE_WITH_ABS_ERROR(distance, expectedDistance, EPSILON);
    }

    { // verify miss
        glm::vec3 rayEnd = rectCenter + rectRotation * ((0.5f * rectDimensions.y + delta) * yAxis);
        glm::vec3 rayMissDirection = glm::normalize(rayEnd - rayStart);
        float distance = FLT_MAX;
        bool hit = findRayRectangleIntersection(rayStart, rayMissDirection, rectRotation, rectCenter, rectDimensions, distance);
        QCOMPARE(hit, false);
        QCOMPARE(distance, FLT_MAX); // distance should be unchanged
    }

    { // hit with co-planer line
        float yFraction = 0.25f;
        rayStart = rectCenter + rectRotation * (rectDimensions.x * xAxis + yFraction * rectDimensions.y * yAxis);

        glm::vec3 rayEnd = rectCenter - rectRotation * (rectDimensions.x * xAxis + yFraction * rectDimensions.y * yAxis);
        glm::vec3 rayHitDirection = glm::normalize(rayEnd - rayStart);
        float expectedDistance = rectDimensions.x;

        float distance = FLT_MAX;
        bool hit = findRayRectangleIntersection(rayStart, rayHitDirection, rectRotation, rectCenter, rectDimensions, distance);
        QCOMPARE(hit, true);
        QCOMPARE_WITH_ABS_ERROR(distance, expectedDistance, EPSILON);
    }

    { // miss with co-planer line
        float yFraction = 0.75f;
        rayStart = rectCenter + rectRotation * (rectDimensions.x * xAxis + (yFraction * rectDimensions.y) * yAxis);

        glm::vec3 rayEnd = rectCenter + rectRotation * (- rectDimensions.x * xAxis + (yFraction * rectDimensions.y) * yAxis);
        glm::vec3 rayHitDirection = glm::normalize(rayEnd - rayStart);

        float distance = FLT_MAX;
        bool hit = findRayRectangleIntersection(rayStart, rayHitDirection, rectRotation, rectCenter, rectDimensions, distance);
        QCOMPARE(hit, false);
        QCOMPARE(distance, FLT_MAX); // distance should be unchanged
    }
}

void GeometryUtilTests::testWorldRayRectangleIntersection() {
    glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
    glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
    glm::vec3 zAxis(0.0f, 0.0f, 1.0f);

    // create a rectangle in the local frame on the XY plane with normal along -zAxis
    // (this is the assumed local orientation of the rectangle in findRayRectangleIntersection())
    // and then rotate and shift everything into the world frame
    glm::vec2 rectDimensions(3.0f, 5.0f);
    glm::vec3 rectCenter(0.7f, 0.5f, -0.3f);
    glm::quat rectRotation = glm::quat(); // identity


    // create points for generating rays that hit the plane and don't
    glm::vec3 rayStart(1.0f, 2.0f, 3.0f);
    float delta = 0.1f;

    { // verify hit
        glm::vec3 rayEnd = rectCenter + rectRotation * ((0.5f * rectDimensions.x - delta) * xAxis);
        glm::vec3 rayHitDirection = glm::normalize(rayEnd - rayStart);
        float expectedDistance = glm::length(rayEnd - rayStart);

        float distance = FLT_MAX;
        bool hit = findRayRectangleIntersection(rayStart, rayHitDirection, rectRotation, rectCenter, rectDimensions, distance);
        QCOMPARE(hit, true);
        QCOMPARE_WITH_ABS_ERROR(distance, expectedDistance, EPSILON);
    }

    { // verify miss
        glm::vec3 rayEnd = rectCenter + rectRotation * ((0.5f * rectDimensions.y + delta) * yAxis);
        glm::vec3 rayMissDirection = glm::normalize(rayEnd - rayStart);
        float distance = FLT_MAX;
        bool hit = findRayRectangleIntersection(rayStart, rayMissDirection, rectRotation, rectCenter, rectDimensions, distance);
        QCOMPARE(hit, false);
        QCOMPARE(distance, FLT_MAX); // distance should be unchanged
    }

    { // hit with co-planer line
        float yFraction = 0.25f;
        rayStart = rectCenter + rectRotation * (rectDimensions.x * xAxis + (yFraction * rectDimensions.y) * yAxis);

        glm::vec3 rayEnd = rectCenter - rectRotation * (rectDimensions.x * xAxis + (yFraction * rectDimensions.y) * yAxis);
        glm::vec3 rayHitDirection = glm::normalize(rayEnd - rayStart);
        float expectedDistance = rectDimensions.x;

        float distance = FLT_MAX;
        bool hit = findRayRectangleIntersection(rayStart, rayHitDirection, rectRotation, rectCenter, rectDimensions, distance);
        QCOMPARE(hit, true);
        QCOMPARE_WITH_ABS_ERROR(distance, expectedDistance, EPSILON);
    }

    { // miss with co-planer line
        float yFraction = 0.75f;
        rayStart = rectCenter + rectRotation * (rectDimensions.x * xAxis + (yFraction * rectDimensions.y) * yAxis);

        glm::vec3 rayEnd = rectCenter + rectRotation * (-rectDimensions.x * xAxis + (yFraction * rectDimensions.y) * yAxis);
        glm::vec3 rayHitDirection = glm::normalize(rayEnd - rayStart);

        float distance = FLT_MAX;
        bool hit = findRayRectangleIntersection(rayStart, rayHitDirection, rectRotation, rectCenter, rectDimensions, distance);
        QCOMPARE(hit, false);
        QCOMPARE(distance, FLT_MAX); // distance should be unchanged
    }
}

