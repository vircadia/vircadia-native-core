//
//  RotationConstraintTests.cpp
//  tests/rig/src
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RotationConstraintTests.h"

#include <glm/glm.hpp>

#include <ElbowConstraint.h>
#include <NumericalConstants.h>
#include <SwingTwistConstraint.h>

#include "../QTestExtensions.h"


QTEST_MAIN(RotationConstraintTests)

void RotationConstraintTests::testElbowConstraint() {
    // referenceRotation is the default rotation
    float referenceAngle = 1.23f;
    glm::vec3 referenceAxis = glm::normalize(glm::vec3(1.0f, 2.0f, -3.0f));
    glm::quat referenceRotation = glm::angleAxis(referenceAngle, referenceAxis);

    // NOTE: hingeAxis is in the "referenceFrame"
    glm::vec3 hingeAxis = glm::vec3(1.0f, 0.0f, 0.0f);

    // the angle limits of the constriant about the hinge axis
    float minAngle = -PI / 4.0f;
    float maxAngle = PI / 3.0f;

    // build the constraint
    ElbowConstraint elbow;
    elbow.setReferenceRotation(referenceRotation);
    elbow.setHingeAxis(hingeAxis);
    elbow.setAngleLimits(minAngle, maxAngle);

    float smallAngle = PI / 100.0f;

    { // test reference rotation -- should be unconstrained
        glm::quat inputRotation = referenceRotation;
        glm::quat outputRotation = inputRotation;
        bool updated = elbow.apply(outputRotation);
        QVERIFY(updated == false);
        glm::quat expectedRotation = referenceRotation;
        QCOMPARE_WITH_ABS_ERROR(expectedRotation, outputRotation, EPSILON);
    }

    { // test several simple rotations that are INSIDE the limits --  should be unconstrained
        int numChecks = 10;
        float startAngle = minAngle + smallAngle;
        float endAngle = maxAngle - smallAngle;
        float deltaAngle = (endAngle - startAngle) / (float)(numChecks - 1);
    
        for (float angle = startAngle; angle < endAngle + 0.5f * deltaAngle; angle += deltaAngle) {
            glm::quat inputRotation = glm::angleAxis(angle, hingeAxis) * referenceRotation;
            glm::quat outputRotation = inputRotation;
            bool updated = elbow.apply(outputRotation);
            QVERIFY(updated == false);
            QCOMPARE_WITH_ABS_ERROR(inputRotation, outputRotation, EPSILON);
        }
    }

    { // test simple rotation just OUTSIDE minAngle --  should be constrained
        float angle = minAngle - smallAngle;
        glm::quat inputRotation = glm::angleAxis(angle, hingeAxis) * referenceRotation;
        glm::quat outputRotation = inputRotation;
        bool updated = elbow.apply(outputRotation);
        QVERIFY(updated == true);
        glm::quat expectedRotation = glm::angleAxis(minAngle, hingeAxis) * referenceRotation;
        QCOMPARE_WITH_ABS_ERROR(expectedRotation, outputRotation, EPSILON);
    }

    { // test simple rotation just OUTSIDE maxAngle --  should be constrained
        float angle = maxAngle + smallAngle;
        glm::quat inputRotation = glm::angleAxis(angle, hingeAxis) * referenceRotation;
        glm::quat outputRotation = inputRotation;
        bool updated = elbow.apply(outputRotation);
        QVERIFY(updated == true);
        glm::quat expectedRotation = glm::angleAxis(maxAngle, hingeAxis) * referenceRotation;
        QCOMPARE_WITH_ABS_ERROR(expectedRotation, outputRotation, EPSILON);
    }

    { // test simple twist rotation that has no hinge component --  should be constrained
        glm::vec3 someVector(7.0f, -5.0f, 2.0f);
        glm::vec3 twistVector = glm::normalize(glm::cross(hingeAxis, someVector));
        float someAngle = 0.789f;
        glm::quat inputRotation = glm::angleAxis(someAngle, twistVector) * referenceRotation;
        glm::quat outputRotation = inputRotation;
        bool updated = elbow.apply(outputRotation);
        QVERIFY(updated == true);
        glm::quat expectedRotation = referenceRotation;
        QCOMPARE_WITH_ABS_ERROR(expectedRotation, outputRotation, EPSILON);
    }
}

void RotationConstraintTests::testSwingTwistConstraint() {
    // referenceRotation is the default rotation
    float referenceAngle = 1.23f;
    glm::vec3 referenceAxis = glm::normalize(glm::vec3(1.0f, 2.0f, -3.0f));
    glm::quat referenceRotation = glm::angleAxis(referenceAngle, referenceAxis);

    // the angle limits of the constriant about the hinge axis
    float minTwistAngle = -PI / 2.0f;
    float maxTwistAngle = PI / 2.0f;

    // build the constraint
    SwingTwistConstraint shoulder;
    shoulder.setReferenceRotation(referenceRotation);
    shoulder.setTwistLimits(minTwistAngle, maxTwistAngle);
    float lowDot = 0.25f;
    float highDot = 0.75f;
    // The swing constriants are more interesting: a vector of minimum dot products 
    // as a function of theta around the twist axis.  Our test function will be shaped
    // like the square wave with amplitudes 0.25 and 0.75:
    //
    //          |
    //     0.75 -               o---o---o---o
    //          |              /             '
    //          |             /               '
    //          |            /                 '
    //     0.25 o---o---o---o                   o
    //          |
    //          +-------+-------+-------+-------+---
    //          0     pi/2     pi     3pi/2    2pi

    int numDots = 8;
    std::vector<float> minDots;
    int dotIndex = 0;
    while (dotIndex < numDots / 2) {
        ++dotIndex;
        minDots.push_back(lowDot);
    }
    while (dotIndex < numDots) {
        minDots.push_back(highDot);
        ++dotIndex;
    }
    shoulder.setSwingLimits(minDots);
    const SwingTwistConstraint::SwingLimitFunction& shoulderSwingLimitFunction = shoulder.getSwingLimitFunction();

    { // test interpolation of SwingLimitFunction
        const float ACCEPTABLE_ERROR = 1.0e-5f;
        float theta = 0.0f;
        float minDot = shoulderSwingLimitFunction.getMinDot(theta);
        float expectedMinDot = lowDot;
        QCOMPARE_WITH_RELATIVE_ERROR(minDot, expectedMinDot, ACCEPTABLE_ERROR);

        theta = PI;
        minDot = shoulderSwingLimitFunction.getMinDot(theta);
        expectedMinDot = highDot;
        QCOMPARE_WITH_RELATIVE_ERROR(minDot, expectedMinDot, ACCEPTABLE_ERROR);

        // test interpolation on upward slope
        theta = PI * (7.0f / 8.0f);
        minDot = shoulderSwingLimitFunction.getMinDot(theta);
        expectedMinDot = 0.5f * (highDot + lowDot);
        QCOMPARE_WITH_RELATIVE_ERROR(minDot, expectedMinDot, ACCEPTABLE_ERROR);

        // test interpolation on downward slope
        theta = PI * (15.0f / 8.0f);
        minDot = shoulderSwingLimitFunction.getMinDot(theta);
        expectedMinDot = 0.5f * (highDot + lowDot);
    }

    float smallAngle = PI / 100.0f;

    // Note: the twist is always about the yAxis
    glm::vec3 yAxis(0.0f, 1.0f, 0.0f);

    { // test INSIDE both twist and swing
        int numSwingAxes = 7;
        float deltaTheta = TWO_PI / numSwingAxes;

        int numTwists = 2;
        float startTwist = minTwistAngle + smallAngle;
        float endTwist = maxTwistAngle - smallAngle;
        float deltaTwist = (endTwist - startTwist) / (float)(numTwists - 1);
        float twist = startTwist;

        for (int i = 0; i < numTwists; ++i) {
            glm::quat twistRotation = glm::angleAxis(twist, yAxis);

            for (float theta = 0.0f; theta < TWO_PI; theta += deltaTheta) {
                float swing = acosf(shoulderSwingLimitFunction.getMinDot(theta)) - smallAngle;
                glm::vec3 swingAxis(cosf(theta), 0.0f, -sinf(theta));
                glm::quat swingRotation = glm::angleAxis(swing, swingAxis);

                glm::quat inputRotation = swingRotation * twistRotation * referenceRotation;
                glm::quat outputRotation = inputRotation;

                shoulder.clearHistory();
                bool updated = shoulder.apply(outputRotation);
                QVERIFY(updated == false);
                QCOMPARE_WITH_ABS_ERROR(inputRotation, outputRotation, EPSILON);
            }
            twist += deltaTwist;
        }
    }

    { // test INSIDE twist but OUTSIDE swing
        int numSwingAxes = 7;
        float deltaTheta = TWO_PI / numSwingAxes;

        int numTwists = 2;
        float startTwist = minTwistAngle + smallAngle;
        float endTwist = maxTwistAngle - smallAngle;
        float deltaTwist = (endTwist - startTwist) / (float)(numTwists - 1);
        float twist = startTwist;

        for (int i = 0; i < numTwists; ++i) {
            glm::quat twistRotation = glm::angleAxis(twist, yAxis);

            for (float theta = 0.0f; theta < TWO_PI; theta += deltaTheta) {
                float maxSwingAngle = acosf(shoulderSwingLimitFunction.getMinDot(theta));
                float swing = maxSwingAngle + smallAngle;
                glm::vec3 swingAxis(cosf(theta), 0.0f, -sinf(theta));
                glm::quat swingRotation = glm::angleAxis(swing, swingAxis);

                glm::quat inputRotation = swingRotation * twistRotation * referenceRotation;
                glm::quat outputRotation = inputRotation;

                shoulder.clearHistory();
                bool updated = shoulder.apply(outputRotation);
                QVERIFY(updated == true);

                glm::quat expectedSwingRotation = glm::angleAxis(maxSwingAngle, swingAxis);
                glm::quat expectedRotation = expectedSwingRotation * twistRotation * referenceRotation;
                QCOMPARE_WITH_ABS_ERROR(expectedRotation, outputRotation, EPSILON);
            }
            twist += deltaTwist;
        }
    }

    { // test OUTSIDE twist but INSIDE swing
        int numSwingAxes = 6;
        float deltaTheta = TWO_PI / numSwingAxes;

        int numTwists = 2;
        float startTwist = minTwistAngle - smallAngle;
        float endTwist = maxTwistAngle + smallAngle;
        float deltaTwist = (endTwist - startTwist) / (float)(numTwists - 1);
        float twist = startTwist;

        for (int i = 0; i < numTwists; ++i) {
            glm::quat twistRotation = glm::angleAxis(twist, yAxis);
            float clampedTwistAngle = glm::clamp(twist, minTwistAngle, maxTwistAngle);

            for (float theta = 0.0f; theta < TWO_PI; theta += deltaTheta) {
                float maxSwingAngle = acosf(shoulderSwingLimitFunction.getMinDot(theta));
                float swing = maxSwingAngle - smallAngle;
                glm::vec3 swingAxis(cosf(theta), 0.0f, -sinf(theta));
                glm::quat swingRotation = glm::angleAxis(swing, swingAxis);

                glm::quat inputRotation = swingRotation * twistRotation * referenceRotation;
                glm::quat outputRotation = inputRotation;

                shoulder.clearHistory();
                bool updated = shoulder.apply(outputRotation);
                QVERIFY(updated == true);

                glm::quat expectedTwistRotation = glm::angleAxis(clampedTwistAngle, yAxis);
                glm::quat expectedRotation = swingRotation * expectedTwistRotation * referenceRotation;
                QCOMPARE_WITH_ABS_ERROR(expectedRotation, outputRotation, EPSILON);
            }
            twist += deltaTwist;
        }
    }

    { // test OUTSIDE both twist and swing
        int numSwingAxes = 5;
        float deltaTheta = TWO_PI / numSwingAxes;

        int numTwists = 2;
        float startTwist = minTwistAngle - smallAngle;
        float endTwist = maxTwistAngle + smallAngle;
        float deltaTwist = (endTwist - startTwist) / (float)(numTwists - 1);
        float twist = startTwist;

        for (int i = 0; i < numTwists; ++i) {
            glm::quat twistRotation = glm::angleAxis(twist, yAxis);
            float clampedTwistAngle = glm::clamp(twist, minTwistAngle, maxTwistAngle);

            for (float theta = 0.0f; theta < TWO_PI; theta += deltaTheta) {
                float maxSwingAngle = acosf(shoulderSwingLimitFunction.getMinDot(theta));
                float swing = maxSwingAngle + smallAngle;
                glm::vec3 swingAxis(cosf(theta), 0.0f, -sinf(theta));
                glm::quat swingRotation = glm::angleAxis(swing, swingAxis);

                glm::quat inputRotation = swingRotation * twistRotation * referenceRotation;
                glm::quat outputRotation = inputRotation;

                shoulder.clearHistory();
                bool updated = shoulder.apply(outputRotation);
                QVERIFY(updated == true);

                glm::quat expectedTwistRotation = glm::angleAxis(clampedTwistAngle, yAxis);
                glm::quat expectedSwingRotation = glm::angleAxis(maxSwingAngle, swingAxis);
                glm::quat expectedRotation = expectedSwingRotation * expectedTwistRotation * referenceRotation;
                QCOMPARE_WITH_ABS_ERROR(expectedRotation, outputRotation, EPSILON);
            }
            twist += deltaTwist;
        }
    }
}

