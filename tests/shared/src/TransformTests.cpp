//
// TransformTests.cpp
// tests/shared/src
//
// Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TransformTests.h"

#include <algorithm>

#include <SharedLogging.h>

#include "../QTestExtensions.h"
#include <QtCore/QDebug>
#include <Transform.h>
#include <StreamUtils.h>
#include <glm/glm.hpp>

//using namespace glm;

const vec3 xAxis(1.0f, 0.0f, 0.0f);
const vec3 yAxis(0.0f, 1.0f, 0.0f);
const vec3 zAxis(0.0f, 0.0f, 1.0f);
const quat rot90 = glm::angleAxis((float)M_PI / 2.0f, yAxis);

QTEST_MAIN(TransformTests)

const float EPSILON = 0.001f;

void TransformTests::getMatrix() {

    const vec3 t(0.0f, 0.0f, 10.0f);

    // create a matrix that is composed of a PI/2 rotation followed by a small z translation
    const mat4 m(vec4(rot90 * xAxis, 0.0f),
                 vec4(rot90 * yAxis, 0.0f),
                 vec4(rot90 * zAxis, 0.0f),
                 vec4(vec4(t, 1.0f)));

    // postScale by a mirror about the x axis.
    const mat4 mirrorX(vec4(-1.0f, 0.0f, 0.0f, 0.0f),
                       vec4( 0.0f, 1.0f, 0.0f, 0.0f),
                       vec4( 0.0f, 0.0f, 1.0f, 0.0f),
                       vec4( 0.0f, 0.0f, 0.0f, 1.0f));
    const mat4 result_a = m * mirrorX;

    Transform xform;
    xform.setRotation(rot90);
    xform.setTranslation(t);
    xform.postScale(vec3(-1.0f, 1.0f, 1.0f));
    mat4 result_b;
    xform.getMatrix(result_b);

    QCOMPARE_WITH_ABS_ERROR(result_a, result_b, EPSILON);
}

void TransformTests::getInverseMatrix() {

    const vec3 t(0.0f, 0.0f, 10.0f);

    // create a matrix that is composed of a PI/2 rotation followed by a small z translation
    const mat4 m(vec4(rot90 * xAxis, 0.0f),
                 vec4(rot90 * yAxis, 0.0f),
                 vec4(rot90 * zAxis, 0.0f),
                 vec4(vec4(t, 1.0f)));

    // mirror about the x axis.
    const mat4 mirrorX(vec4(-1.0f, 0.0f, 0.0f, 0.0f),
                       vec4( 0.0f, 1.0f, 0.0f, 0.0f),
                       vec4( 0.0f, 0.0f, 1.0f, 0.0f),
                       vec4( 0.0f, 0.0f, 0.0f, 1.0f));
    const mat4 result_a = glm::inverse(m * mirrorX);

    Transform xform;
    xform.setTranslation(t);
    xform.setRotation(rot90);
    xform.postScale(vec3(-1.0f, 1.0f, 1.0f));

    mat4 result_b;
    xform.getInverseMatrix(result_b);

    // don't check elements directly, instead compare each axis transformed by the matrix.
    auto xa = transformPoint(result_a, xAxis);
    auto ya = transformPoint(result_a, yAxis);
    auto za = transformPoint(result_a, zAxis);

    auto xb = transformPoint(result_b, xAxis);
    auto yb = transformPoint(result_b, yAxis);
    auto zb = transformPoint(result_b, zAxis);

    QCOMPARE_WITH_ABS_ERROR(xa, xb, EPSILON);
    QCOMPARE_WITH_ABS_ERROR(ya, yb, EPSILON);
    QCOMPARE_WITH_ABS_ERROR(za, zb, EPSILON);
}
