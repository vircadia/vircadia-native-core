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
#include <GLMHelpers.h>
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
    // like a square wave with amplitudes 0.25 and 0.75:
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

void RotationConstraintTests::testDynamicSwingLimitFunction() {
    SwingTwistConstraint::SwingLimitFunction limitFunction;
    const float ACCEPTABLE_ERROR = 1.0e-6f;

    const float adjustmentDot = -0.5f;

    const float MIN_DOT = 0.5f;
    { // initialize limitFunction
        std::vector<float> minDots;
        minDots.push_back(MIN_DOT);
        limitFunction.setMinDots(minDots);
    }

    std::vector<float> referenceDots;
    { // verify limits and initialize referenceDots
        const int MIN_NUM_DOTS = 8;
        const std::vector<float>& minDots = limitFunction.getMinDots();
        QVERIFY(minDots.size() >= MIN_NUM_DOTS);

        int numDots = (int)minDots.size();
        for (int i = 0; i < numDots; ++i) {
            QCOMPARE_WITH_RELATIVE_ERROR(minDots[i], MIN_DOT, ACCEPTABLE_ERROR);
            referenceDots.push_back(minDots[i]);
        }
    }
    { // dynamically adjust limits
        const std::vector<float>& minDots = limitFunction.getMinDots();
        int numDots = (int)minDots.size();

        float deltaTheta = TWO_PI / (float)(numDots - 1);
        int indexA = 2;
        int indexB = (indexA + 1) % numDots;

        { // dynamically adjust a data point
            float theta = deltaTheta * (float)indexA;
            float interpolatedDot = limitFunction.getMinDot(theta);

            // change indexA
            limitFunction.dynamicallyAdjustMinDots(theta, adjustmentDot);
            QCOMPARE_WITH_ABS_ERROR(limitFunction.getMinDot(theta), adjustmentDot, ACCEPTABLE_ERROR); // adjustmentDot at theta
            QCOMPARE_WITH_ABS_ERROR(minDots[indexA], adjustmentDot, ACCEPTABLE_ERROR); // indexA has changed
            QCOMPARE_WITH_ABS_ERROR(minDots[indexB], referenceDots[indexB], ACCEPTABLE_ERROR); // indexB has not changed

            // change indexB
            theta = deltaTheta * (float)indexB;
            limitFunction.dynamicallyAdjustMinDots(theta, adjustmentDot);
            QCOMPARE_WITH_ABS_ERROR(limitFunction.getMinDot(theta), adjustmentDot, ACCEPTABLE_ERROR); // adjustmentDot at theta
            QCOMPARE_WITH_ABS_ERROR(minDots[indexA], referenceDots[indexA], ACCEPTABLE_ERROR); // indexA has been restored
            QCOMPARE_WITH_ABS_ERROR(minDots[indexB], adjustmentDot, ACCEPTABLE_ERROR); // indexB has changed

            // restore
            limitFunction.dynamicallyAdjustMinDots(theta, referenceDots[indexB] + 0.01f); // restore with a larger dot
            QCOMPARE_WITH_ABS_ERROR(limitFunction.getMinDot(theta), interpolatedDot, ACCEPTABLE_ERROR); // restored
            QCOMPARE_WITH_ABS_ERROR(minDots[indexA], referenceDots[indexA], ACCEPTABLE_ERROR); // indexA is restored
            QCOMPARE_WITH_ABS_ERROR(minDots[indexB], referenceDots[indexB], ACCEPTABLE_ERROR); // indexB is restored
        }
        { // dynamically adjust halfway between data points
            float theta = deltaTheta * 0.5f * (float)(indexA + indexB); // halfway between two points
            float interpolatedDot = limitFunction.getMinDot(theta);
            float deltaDot = adjustmentDot - interpolatedDot;
            limitFunction.dynamicallyAdjustMinDots(theta, adjustmentDot);
            QCOMPARE_WITH_ABS_ERROR(limitFunction.getMinDot(theta), adjustmentDot, ACCEPTABLE_ERROR); // adjustmentDot at theta
            QCOMPARE_WITH_ABS_ERROR(minDots[indexA], referenceDots[indexA] + deltaDot, ACCEPTABLE_ERROR); // indexA has changed
            QCOMPARE_WITH_ABS_ERROR(minDots[indexB], referenceDots[indexB] + deltaDot, ACCEPTABLE_ERROR); // indexB has changed

            limitFunction.dynamicallyAdjustMinDots(theta, interpolatedDot + 0.01f); // reset with something larger
            QCOMPARE_WITH_ABS_ERROR(limitFunction.getMinDot(theta), interpolatedDot, ACCEPTABLE_ERROR); // restored
            QCOMPARE_WITH_ABS_ERROR(minDots[indexA], referenceDots[indexA], ACCEPTABLE_ERROR); // indexA is restored
            QCOMPARE_WITH_ABS_ERROR(minDots[indexB], referenceDots[indexB], ACCEPTABLE_ERROR); // indexB is restored
        }
        { // dynamically adjust one-quarter between data points
            float theta = deltaTheta * ((float)indexA + 0.25f); // one quarter past A towards B
            float interpolatedDot = limitFunction.getMinDot(theta);
            limitFunction.dynamicallyAdjustMinDots(theta, adjustmentDot);
            QCOMPARE_WITH_ABS_ERROR(limitFunction.getMinDot(theta), adjustmentDot, ACCEPTABLE_ERROR); // adjustmentDot at theta
            QVERIFY(minDots[indexA] < adjustmentDot); // indexA should be less than minDot
            QVERIFY(minDots[indexB] > adjustmentDot); // indexB should be larger than minDot
            QVERIFY(minDots[indexB] < referenceDots[indexB]); // indexB should be less than what it was

            limitFunction.dynamicallyAdjustMinDots(theta, interpolatedDot + 0.01f); // reset with something larger
            QCOMPARE_WITH_ABS_ERROR(limitFunction.getMinDot(theta), interpolatedDot, ACCEPTABLE_ERROR); // restored
            QCOMPARE_WITH_ABS_ERROR(minDots[indexA], referenceDots[indexA], ACCEPTABLE_ERROR); // indexA is restored
            QCOMPARE_WITH_ABS_ERROR(minDots[indexB], referenceDots[indexB], ACCEPTABLE_ERROR); // indexB is restored
        }
        { // halfway between first two data points (boundary condition)
            indexA = 0;
            indexB = 1;
            int indexZ = minDots.size() - 1; // far boundary condition
            float theta = deltaTheta * 0.5f * (float)(indexA + indexB); // halfway between two points
            float interpolatedDot = limitFunction.getMinDot(theta);
            float deltaDot = adjustmentDot - interpolatedDot;
            limitFunction.dynamicallyAdjustMinDots(theta, adjustmentDot);
            QCOMPARE_WITH_ABS_ERROR(limitFunction.getMinDot(theta), adjustmentDot, ACCEPTABLE_ERROR); // adjustmentDot at theta
            QCOMPARE_WITH_ABS_ERROR(minDots[indexA], referenceDots[indexA] + deltaDot, ACCEPTABLE_ERROR); // indexA has changed
            QCOMPARE_WITH_ABS_ERROR(minDots[indexB], referenceDots[indexB] + deltaDot, ACCEPTABLE_ERROR); // indexB has changed
            QCOMPARE_WITH_ABS_ERROR(minDots[indexZ], referenceDots[indexZ] + deltaDot, ACCEPTABLE_ERROR); // indexZ has changed

            limitFunction.dynamicallyAdjustMinDots(theta, interpolatedDot + 0.01f); // reset with something larger
            QCOMPARE_WITH_ABS_ERROR(limitFunction.getMinDot(theta), interpolatedDot, ACCEPTABLE_ERROR); // restored
            QCOMPARE_WITH_ABS_ERROR(minDots[indexA], referenceDots[indexA], ACCEPTABLE_ERROR); // indexA is restored
            QCOMPARE_WITH_ABS_ERROR(minDots[indexB], referenceDots[indexB], ACCEPTABLE_ERROR); // indexB is restored
            QCOMPARE_WITH_ABS_ERROR(minDots[indexZ], referenceDots[indexZ], ACCEPTABLE_ERROR); // indexZ is restored
        }
        { // halfway between first two data points (boundary condition)
            indexB = minDots.size() - 1;
            indexA = indexB - 1;
            int indexZ = 0; // far boundary condition
            float theta = deltaTheta * 0.5f * (float)(indexA + indexB); // halfway between two points
            float interpolatedDot = limitFunction.getMinDot(theta);
            float deltaDot = adjustmentDot - interpolatedDot;
            limitFunction.dynamicallyAdjustMinDots(theta, adjustmentDot);
            QCOMPARE_WITH_ABS_ERROR(limitFunction.getMinDot(theta), adjustmentDot, ACCEPTABLE_ERROR); // adjustmentDot at theta
            QCOMPARE_WITH_ABS_ERROR(minDots[indexA], referenceDots[indexA] + deltaDot, ACCEPTABLE_ERROR); // indexA has changed
            QCOMPARE_WITH_ABS_ERROR(minDots[indexB], referenceDots[indexB] + deltaDot, ACCEPTABLE_ERROR); // indexB has changed
            QCOMPARE_WITH_ABS_ERROR(minDots[indexZ], referenceDots[indexZ] + deltaDot, ACCEPTABLE_ERROR); // indexZ has changed

            limitFunction.dynamicallyAdjustMinDots(theta, interpolatedDot + 0.01f); // reset with something larger
            QCOMPARE_WITH_ABS_ERROR(limitFunction.getMinDot(theta), interpolatedDot, ACCEPTABLE_ERROR); // restored
            QCOMPARE_WITH_ABS_ERROR(minDots[indexA], referenceDots[indexA], ACCEPTABLE_ERROR); // indexA is restored
            QCOMPARE_WITH_ABS_ERROR(minDots[indexB], referenceDots[indexB], ACCEPTABLE_ERROR); // indexB is restored
            QCOMPARE_WITH_ABS_ERROR(minDots[indexZ], referenceDots[indexZ], ACCEPTABLE_ERROR); // indexZ is restored
        }
    }
}

void RotationConstraintTests::testDynamicSwing() {
    const float ACCEPTABLE_ERROR = 1.0e-6f;

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
    std::vector<float> minDots;
    const float MIN_DOT = 0.5f;
    minDots.push_back(MIN_DOT);
    shoulder.setSwingLimits(minDots);

    // verify resolution of the swing limits
    const std::vector<float>& shoulderMinDots = shoulder.getMinDots();
    const int MIN_NUM_DOTS = 8;
    int numDots = shoulderMinDots.size();
    QVERIFY(numDots >= MIN_NUM_DOTS);

    // verify values of the swing limits
    QCOMPARE_WITH_ABS_ERROR(shoulderMinDots[0], shoulderMinDots[numDots - 1], ACCEPTABLE_ERROR); // endpoints should be the same
    for (int i = 0; i < numDots; ++i) {
        QCOMPARE_WITH_ABS_ERROR(shoulderMinDots[i], MIN_DOT, ACCEPTABLE_ERROR); // all values should be the same
    }

    float deltaTheta = TWO_PI / (float)(numDots - 1);
    float theta = 1.5f * deltaTheta;
    glm::vec3 swingAxis(cosf(theta), 0.0f, sinf(theta));
    float deltaSwing = 0.1f;

    { // compute rotation that should NOT be constrained
        float swingAngle = acosf(MIN_DOT) - deltaSwing;
        glm::quat swingRotation = glm::angleAxis(swingAngle, swingAxis);
        glm::quat totalRotation = swingRotation * referenceRotation;

        // verify rotation is NOT constrained
        glm::quat constrainedRotation = totalRotation;
        QVERIFY(!shoulder.apply(constrainedRotation));
    }

    { // compute a rotation that should be barely constrained
        float swingAngle = acosf(MIN_DOT) + deltaSwing;
        glm::quat swingRotation = glm::angleAxis(swingAngle, swingAxis);
        glm::quat totalRotation = swingRotation * referenceRotation;

        // verify rotation is constrained
        glm::quat constrainedRotation = totalRotation;
        QVERIFY(shoulder.apply(constrainedRotation));
    }

    { // make a dynamic adjustment to the swing limits
        const float SMALLER_MIN_DOT = -0.5f;
        float swingAngle = acosf(SMALLER_MIN_DOT);
        glm::quat swingRotation = glm::angleAxis(swingAngle, swingAxis);
        glm::quat badRotation = swingRotation * referenceRotation;

        { // verify rotation is constrained
            glm::quat constrainedRotation = badRotation;
            QVERIFY(shoulder.apply(constrainedRotation));

            // now poke the SMALLER_MIN_DOT into the swing limits
            shoulder.dynamicallyAdjustLimits(badRotation);

            // verify that if rotation is constrained then it is only by a little bit
            constrainedRotation = badRotation;
            bool constrained = shoulder.apply(constrainedRotation);
            if (constrained) {
                // Note: Q1 = dQ * Q0  -->  dQ = Q1 * Q0^
                glm::quat dQ = constrainedRotation * glm::inverse(badRotation);
                const float acceptableClampAngle = 0.01f;
                float deltaAngle = glm::angle(dQ);
                QVERIFY(deltaAngle < acceptableClampAngle);
            }
        }

        { // verify that other swing axes still use the old non-adjusted limits
            float deltaTheta = TWO_PI / (float)(numDots - 1);
            float otherTheta = 3.5f * deltaTheta;
            glm::vec3 otherSwingAxis(cosf(otherTheta), 0.0f, sinf(otherTheta));

            { // inside rotations should be unconstrained
                float goodAngle = acosf(MIN_DOT) - deltaSwing;
                glm::quat goodRotation = glm::angleAxis(goodAngle, otherSwingAxis) * referenceRotation;
                QVERIFY(!shoulder.apply(goodRotation));
            }
            { // outside rotations should be constrained
                float badAngle = acosf(MIN_DOT) + deltaSwing;
                glm::quat otherBadRotation = glm::angleAxis(badAngle, otherSwingAxis) * referenceRotation;
                QVERIFY(shoulder.apply(otherBadRotation));

                float constrainedAngle = glm::angle(otherBadRotation);
                QCOMPARE_WITH_ABS_ERROR(constrainedAngle, acosf(MIN_DOT), 0.1f * deltaSwing);
            }
        }

        { // clear dynamic adjustment
            float goodAngle = acosf(MIN_DOT) - deltaSwing;
            glm::quat goodRotation = glm::angleAxis(goodAngle, swingAxis) * referenceRotation;

            // when we update with a goodRotation the dynamic adjustment is cleared
            shoulder.dynamicallyAdjustLimits(goodRotation);

            // verify that the old badRotation, which was not constrained dynamically, is now constrained
            glm::quat constrainedRotation = badRotation;
            QVERIFY(shoulder.apply(constrainedRotation));

            // and the good rotation should not be constrained
            constrainedRotation = goodRotation;
            QVERIFY(!shoulder.apply(constrainedRotation));
        }
    }
}

void RotationConstraintTests::testDynamicTwist() {
    // referenceRotation is the default rotation
    float referenceAngle = 1.23f;
    glm::vec3 referenceAxis = glm::normalize(glm::vec3(1.0f, 2.0f, -3.0f));
    glm::quat referenceRotation = glm::angleAxis(referenceAngle, referenceAxis);

    // the angle limits of the constriant about the hinge axis
    const float minTwistAngle = -PI / 2.0f;
    const float maxTwistAngle = PI / 2.0f;

    // build the constraint
    SwingTwistConstraint shoulder;
    shoulder.setReferenceRotation(referenceRotation);
    shoulder.setTwistLimits(minTwistAngle, maxTwistAngle);

    glm::vec3 twistAxis = Vectors::UNIT_Y;
    float deltaTwist = 0.1f;

    { // compute min rotation that should NOT be constrained
        float twistAngle = minTwistAngle + deltaTwist;
        glm::quat twistRotation = glm::angleAxis(twistAngle, twistAxis);
        glm::quat totalRotation = twistRotation * referenceRotation;

        // verify rotation is NOT constrained
        glm::quat constrainedRotation = totalRotation;
        QVERIFY(!shoulder.apply(constrainedRotation));
    }
    { // compute max rotation that should NOT be constrained
        float twistAngle = maxTwistAngle - deltaTwist;
        glm::quat twistRotation = glm::angleAxis(twistAngle, twistAxis);
        glm::quat totalRotation = twistRotation * referenceRotation;

        // verify rotation is NOT constrained
        glm::quat constrainedRotation = totalRotation;
        QVERIFY(!shoulder.apply(constrainedRotation));
    }
    { // compute a min rotation that should be barely constrained
        float twistAngle = minTwistAngle - deltaTwist;
        glm::quat twistRotation = glm::angleAxis(twistAngle, twistAxis);
        glm::quat totalRotation = twistRotation * referenceRotation;

        // verify rotation is constrained
        glm::quat constrainedRotation = totalRotation;
        QVERIFY(shoulder.apply(constrainedRotation));

        // adjust the constraint and verify rotation is NOT constrained
        shoulder.dynamicallyAdjustLimits(totalRotation);
        constrainedRotation = totalRotation;
        bool constrained = shoulder.apply(constrainedRotation);
        if (constrained) {
            // or, if it is constrained then the adjustment is very small
            // Note: Q1 = dQ * Q0  -->  dQ = Q1 * Q0^
            glm::quat dQ = constrainedRotation * glm::inverse(totalRotation);
            const float acceptableClampAngle = 0.01f;
            float deltaAngle = glm::angle(dQ);
            QVERIFY(deltaAngle < acceptableClampAngle);
        }

        // clear the adjustment using a null rotation
        shoulder.dynamicallyAdjustLimits(glm::quat());

        // verify that rotation is constrained again
        constrainedRotation = totalRotation;
        QVERIFY(shoulder.apply(constrainedRotation));
    }
    { // compute a min rotation that should be barely constrained
        float twistAngle = maxTwistAngle + deltaTwist;
        glm::quat twistRotation = glm::angleAxis(twistAngle, twistAxis);
        glm::quat totalRotation = twistRotation * referenceRotation;

        // verify rotation is constrained
        glm::quat constrainedRotation = totalRotation;
        QVERIFY(shoulder.apply(constrainedRotation));

        // adjust the constraint and verify rotation is NOT constrained
        shoulder.dynamicallyAdjustLimits(totalRotation);
        constrainedRotation = totalRotation;
        bool constrained = shoulder.apply(constrainedRotation);
        if (constrained) {
            // or, if it is constrained then the adjustment is very small
            // Note: Q1 = dQ * Q0  -->  dQ = Q1 * Q0^
            glm::quat dQ = constrainedRotation * glm::inverse(totalRotation);
            const float acceptableClampAngle = 0.01f;
            float deltaAngle = glm::angle(dQ);
            QVERIFY(deltaAngle < acceptableClampAngle);
        }

        // clear the adjustment using a null rotation
        shoulder.dynamicallyAdjustLimits(glm::quat());

        // verify that rotation is constrained again
        constrainedRotation = totalRotation;
        QVERIFY(shoulder.apply(constrainedRotation));
    }
}
