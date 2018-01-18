//
//  DualQuaternionTests.cpp
//  tests/shared/src
//
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>

#include "DualQuaternionTests.h"

#include <DualQuaternion.h>
#include <GLMHelpers.h>
#include <NumericalConstants.h>
#include <StreamUtils.h>

#include <../GLMTestUtils.h>
#include <../QTestExtensions.h>

QTEST_MAIN(DualQuaternionTests)

static void quatComp(const glm::quat& q1, const glm::quat& q2) {
    QCOMPARE_WITH_ABS_ERROR(q1.x, q2.x, EPSILON);
    QCOMPARE_WITH_ABS_ERROR(q1.y, q2.y, EPSILON);
    QCOMPARE_WITH_ABS_ERROR(q1.z, q2.z, EPSILON);
    QCOMPARE_WITH_ABS_ERROR(q1.w, q2.w, EPSILON);
}

void DualQuaternionTests::ctor() {
    glm::quat real = angleAxis(PI / 2.0f, Vectors::UNIT_Y);
    glm::quat dual(0.0f, 1.0f, 2.0f, 3.0f);

    DualQuaternion dq(real, dual);
    quatComp(real, dq.real());
    quatComp(dual, dq.dual());

    glm::quat rotation = angleAxis(PI / 3.0f, Vectors::UNIT_X);
    glm::vec3 translation(1.0, 2.0f, 3.0f);
    dq = DualQuaternion(rotation, translation);
    quatComp(rotation, dq.getRotation());
    QCOMPARE_WITH_ABS_ERROR(translation, dq.getTranslation(), EPSILON);

    rotation = angleAxis(-2.0f * PI / 7.0f, Vectors::UNIT_Z);
    translation = glm::vec3(-1.0, 12.0f, 2.0f);
    glm::mat4 m = createMatFromQuatAndPos(rotation, translation);
    dq = DualQuaternion(m);
    quatComp(rotation, dq.getRotation());
    QCOMPARE_WITH_ABS_ERROR(translation, dq.getTranslation(), EPSILON);
}

void DualQuaternionTests::mult() {

    glm::quat rotation = angleAxis(PI / 3.0f, Vectors::UNIT_X);
    glm::vec3 translation(1.0, 2.0f, 3.0f);
    glm::mat4 m1 = createMatFromQuatAndPos(rotation, translation);
    DualQuaternion dq1(m1);

    rotation = angleAxis(-2.0f * PI / 7.0f, Vectors::UNIT_Z);
    translation = glm::vec3(-1.0, 12.0f, 2.0f);
    glm::mat4 m2 = createMatFromQuatAndPos(rotation, translation);
    DualQuaternion dq2(m2);

    DualQuaternion dq3 = dq1 * dq2;
    glm::mat4 m3 = m1 * m2;

    rotation = glmExtractRotation(m3);
    translation = extractTranslation(m3);

    quatComp(rotation, dq3.getRotation());
    QCOMPARE_WITH_ABS_ERROR(translation, dq3.getTranslation(), EPSILON);
}

void DualQuaternionTests::xform() {

    glm::quat rotation = angleAxis(PI / 3.0f, Vectors::UNIT_X);
    glm::vec3 translation(1.0, 2.0f, 3.0f);
    glm::mat4 m1 = createMatFromQuatAndPos(rotation, translation);
    DualQuaternion dq1(m1);

    rotation = angleAxis(-2.0f * PI / 7.0f, Vectors::UNIT_Z);
    translation = glm::vec3(-1.0, 12.0f, 2.0f);
    glm::mat4 m2 = createMatFromQuatAndPos(rotation, translation);
    DualQuaternion dq2(m2);

    DualQuaternion dq3 = dq1 * dq2;
    glm::mat4 m3 = m1 * m2;

    glm::vec3 p(1.0f, 2.0f, 3.0f);

    glm::vec3 p1 = transformPoint(m3, p);
    glm::vec3 p2 = dq3.xformPoint(p);

    QCOMPARE_WITH_ABS_ERROR(p1, p2, 0.001f);

    p1 = transformVectorFast(m3, p);
    p2 = dq3.xformVector(p);

    QCOMPARE_WITH_ABS_ERROR(p1, p2, 0.001f);
}

void DualQuaternionTests::trans() {
    glm::vec3 t1 = glm::vec3();
    DualQuaternion dq1(Quaternions::IDENTITY, t1);
    glm::vec3 t2 = glm::vec3(1.0f, 2.0f, 3.0f);
    DualQuaternion dq2(angleAxis(PI / 3.0f, Vectors::UNIT_X), t2);
    glm::vec3 t3 = glm::vec3(3.0f, 2.0f, 1.0f);
    DualQuaternion dq3(angleAxis(PI / 5.0f, Vectors::UNIT_Y), t3);

    QCOMPARE_WITH_ABS_ERROR(t1, dq1.getTranslation(), 0.001f);
    QCOMPARE_WITH_ABS_ERROR(t2, dq2.getTranslation(), 0.001f);
    QCOMPARE_WITH_ABS_ERROR(t3, dq3.getTranslation(), 0.001f);
}
