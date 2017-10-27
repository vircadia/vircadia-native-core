//
// CubicHermiteSplineTests.cpp
// tests/shared/src
//
// Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CubicHermiteSplineTests.h"
#include "../QTestExtensions.h"
#include <QtCore/QDebug>
#include "CubicHermiteSpline.h"

QTEST_MAIN(CubicHermiteSplineTests)

void CubicHermiteSplineTests::testCubicHermiteSplineFunctor() {
    glm::vec3 p0(0.0f, 0.0f, 0.0f);
    glm::vec3 m0(1.0f, 0.0f, 0.0f);
    glm::vec3 p1(1.0f, 1.0f, 0.0f);
    glm::vec3 m1(2.0f, 0.0f, 0.0f);
    CubicHermiteSplineFunctor hermiteSpline(p0, m0, p1, m1);

    const float EPSILON = 0.0001f;

    QCOMPARE_WITH_ABS_ERROR(p0, hermiteSpline(0.0f), EPSILON);
    QCOMPARE_WITH_ABS_ERROR(p1, hermiteSpline(1.0f), EPSILON);

    // these values were computed offline.
    const glm::vec3 oneFourth(0.203125f, 0.15625f, 0.0f);
    const glm::vec3 oneHalf(0.375f, 0.5f, 0.0f);
    const glm::vec3 threeFourths(0.609375f, 0.84375f, 0.0f);

    QCOMPARE_WITH_ABS_ERROR(oneFourth, hermiteSpline(0.25f), EPSILON);
    QCOMPARE_WITH_ABS_ERROR(oneHalf, hermiteSpline(0.5f), EPSILON);
    QCOMPARE_WITH_ABS_ERROR(threeFourths, hermiteSpline(0.75f), EPSILON);
}

void CubicHermiteSplineTests::testCubicHermiteSplineFunctorWithArcLength() {
    glm::vec3 p0(0.0f, 0.0f, 0.0f);
    glm::vec3 m0(1.0f, 0.0f, 0.0f);
    glm::vec3 p1(1.0f, 1.0f, 0.0f);
    glm::vec3 m1(2.0f, 0.0f, 0.0f);
    CubicHermiteSplineFunctorWithArcLength hermiteSpline(p0, m0, p1, m1);

    const float EPSILON = 0.001f;

    float arcLengths[5] = {
        hermiteSpline.arcLength(0.0f),
        hermiteSpline.arcLength(0.25f),
        hermiteSpline.arcLength(0.5f),
        hermiteSpline.arcLength(0.75f),
        hermiteSpline.arcLength(1.0f)
    };

    // these values were computed offline
    float referenceArcLengths[5] = {
        0.0f,
        0.268317f,
        0.652788f,
        1.07096f,
        1.50267f
    };

    QCOMPARE_WITH_ABS_ERROR(arcLengths[0], referenceArcLengths[0], EPSILON);
    QCOMPARE_WITH_ABS_ERROR(arcLengths[1], referenceArcLengths[1], EPSILON);
    QCOMPARE_WITH_ABS_ERROR(arcLengths[2], referenceArcLengths[2], EPSILON);
    QCOMPARE_WITH_ABS_ERROR(arcLengths[3], referenceArcLengths[3], EPSILON);
    QCOMPARE_WITH_ABS_ERROR(arcLengths[4], referenceArcLengths[4], EPSILON);

    QCOMPARE_WITH_ABS_ERROR(0.0f, hermiteSpline.arcLengthInverse(referenceArcLengths[0]), EPSILON);
    QCOMPARE_WITH_ABS_ERROR(0.25f, hermiteSpline.arcLengthInverse(referenceArcLengths[1]), EPSILON);
    QCOMPARE_WITH_ABS_ERROR(0.5f, hermiteSpline.arcLengthInverse(referenceArcLengths[2]), EPSILON);
    QCOMPARE_WITH_ABS_ERROR(0.75f, hermiteSpline.arcLengthInverse(referenceArcLengths[3]), EPSILON);
    QCOMPARE_WITH_ABS_ERROR(1.0f, hermiteSpline.arcLengthInverse(referenceArcLengths[4]), EPSILON);
}
