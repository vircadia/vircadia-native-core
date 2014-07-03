//
//  AngularConstraintTests.cpp
//  tests/physics/src
//
//  Created by Andrew Meadows on 2014.05.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>

#include <AngularConstraint.h>
#include <SharedUtil.h>
#include <StreamUtils.h>

#include "AngularConstraintTests.h"


void AngularConstraintTests::testHingeConstraint() {
    float minAngle = -PI;
    float maxAngle = 0.0f;
    glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
    glm::vec3 minAngles(0.0f, -PI, 0.0f);
    glm::vec3 maxAngles(0.0f, 0.0f, 0.0f);

    AngularConstraint* c = AngularConstraint::newAngularConstraint(minAngles, maxAngles);
    if (!c) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: newAngularConstraint() should make a constraint" << std::endl;
    }

    {   // test in middle of constraint
        float angle = 0.5f * (minAngle + maxAngle);
        glm::quat rotation = glm::angleAxis(angle, yAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should not clamp()" << std::endl;
        }
        if (rotation != newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should not change rotation" << std::endl;
        }
    }
    {   // test just inside min edge of constraint
        float angle = minAngle + 10.f * EPSILON;
        glm::quat rotation = glm::angleAxis(angle, yAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should not clamp()" << std::endl;
        }
        if (rotation != newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should not change rotation" << std::endl;
        }
    }
    {   // test just inside max edge of constraint
        float angle = maxAngle - 10.f * EPSILON;
        glm::quat rotation = glm::angleAxis(angle, yAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should not clamp()" << std::endl;
        }
        if (rotation != newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should not change rotation" << std::endl;
        }
    }
    {   // test just outside min edge of constraint
        float angle = minAngle - 0.001f;
        glm::quat rotation = glm::angleAxis(angle, yAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should clamp()" << std::endl;
        }
        if (rotation == newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should change rotation" << std::endl;
        }
        glm::quat expectedRotation = glm::angleAxis(minAngle, yAxis);
        float qDot = glm::dot(expectedRotation, newRotation);
        if (fabsf(qDot - 1.0f) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint rotation = " << newRotation << " but expected " << expectedRotation << std::endl;
        }
    }
    {   // test just outside max edge of constraint
        float angle = maxAngle + 0.001f;
        glm::quat rotation = glm::angleAxis(angle, yAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should clamp()" << std::endl;
        }
        if (rotation == newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should change rotation" << std::endl;
        }
        glm::quat expectedRotation = glm::angleAxis(maxAngle, yAxis);
        float qDot = glm::dot(expectedRotation, newRotation);
        if (fabsf(qDot - 1.0f) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint rotation = " << newRotation << " but expected " << expectedRotation << std::endl;
        }
    }
    {   // test far outside min edge of constraint (wraps around to max)
        float angle = minAngle - 0.75f * (TWO_PI - (maxAngle - minAngle));
        glm::quat rotation = glm::angleAxis(angle, yAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should clamp()" << std::endl;
        }
        if (rotation == newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should change rotation" << std::endl;
        }
        glm::quat expectedRotation = glm::angleAxis(maxAngle, yAxis);
        float qDot = glm::dot(expectedRotation, newRotation);
        if (fabsf(qDot - 1.0f) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint rotation = " << newRotation << " but expected " << expectedRotation << std::endl;
        }
    }
    {   // test far outside max edge of constraint (wraps around to min)
        float angle = maxAngle + 0.75f * (TWO_PI - (maxAngle - minAngle));
        glm::quat rotation = glm::angleAxis(angle, yAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should clamp()" << std::endl;
        }
        if (rotation == newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should change rotation" << std::endl;
        }
        glm::quat expectedRotation = glm::angleAxis(minAngle, yAxis);
        float qDot = glm::dot(expectedRotation, newRotation);
        if (fabsf(qDot - 1.0f) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint rotation = " << newRotation << " but expected " << expectedRotation << std::endl;
        }
    }

    float ACCEPTABLE_ERROR = 1.0e-4f;
    {   // test nearby but off-axis rotation
        float offAngle = 0.1f;
        glm::quat offRotation(offAngle, glm::vec3(1.0f, 0.0f, 0.0f));
        float angle = 0.5f * (maxAngle + minAngle);
        glm::quat rotation = offRotation * glm::angleAxis(angle, yAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should clamp()" << std::endl;
        }
        if (rotation == newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should change rotation" << std::endl;
        }
        glm::quat expectedRotation = glm::angleAxis(angle, yAxis);
        float qDot = glm::dot(expectedRotation, newRotation);
        if (fabsf(qDot - 1.0f) > ACCEPTABLE_ERROR) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint rotation = " << newRotation << " but expected " << expectedRotation << std::endl;
        }
    }
    {   // test way off rotation > maxAngle
        float offAngle = 0.5f;
        glm::quat offRotation = glm::angleAxis(offAngle, glm::vec3(1.0f, 0.0f, 0.0f));
        float angle = maxAngle + 0.2f * (TWO_PI - (maxAngle - minAngle));
        glm::quat rotation = glm::angleAxis(angle, yAxis);
        rotation = offRotation * glm::angleAxis(angle, yAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should clamp()" << std::endl;
        }
        if (rotation == newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should change rotation" << std::endl;
        }
        glm::quat expectedRotation = glm::angleAxis(maxAngle, yAxis);
        float qDot = glm::dot(expectedRotation, newRotation);
        if (fabsf(qDot - 1.0f) > ACCEPTABLE_ERROR) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint rotation = " << newRotation << " but expected " << expectedRotation << std::endl;
        }
    }
    {   // test way off rotation < minAngle
        float offAngle = 0.5f;
        glm::quat offRotation = glm::angleAxis(offAngle, glm::vec3(1.0f, 0.0f, 0.0f));
        float angle = minAngle - 0.2f * (TWO_PI - (maxAngle - minAngle));
        glm::quat rotation = glm::angleAxis(angle, yAxis);
        rotation = offRotation * glm::angleAxis(angle, yAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should clamp()" << std::endl;
        }
        if (rotation == newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should change rotation" << std::endl;
        }
        glm::quat expectedRotation = glm::angleAxis(minAngle, yAxis);
        float qDot = glm::dot(expectedRotation, newRotation);
        if (fabsf(qDot - 1.0f) > ACCEPTABLE_ERROR) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint rotation = " << newRotation << " but expected " << expectedRotation << std::endl;
        }
    }
    {   // test way off rotation > maxAngle with wrap over to minAngle
        float offAngle = -0.5f;
        glm::quat offRotation = glm::angleAxis(offAngle, glm::vec3(1.0f, 0.0f, 0.0f));
        float angle = maxAngle + 0.6f * (TWO_PI - (maxAngle - minAngle));
        glm::quat rotation = glm::angleAxis(angle, yAxis);
        rotation = offRotation * glm::angleAxis(angle, yAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should clamp()" << std::endl;
        }
        if (rotation == newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should change rotation" << std::endl;
        }
        glm::quat expectedRotation = glm::angleAxis(minAngle, yAxis);
        float qDot = glm::dot(expectedRotation, newRotation);
        if (fabsf(qDot - 1.0f) > ACCEPTABLE_ERROR) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint rotation = " << newRotation << " but expected " << expectedRotation << std::endl;
        }
    }
    {   // test way off rotation < minAngle with wrap over to maxAngle
        float offAngle = -0.6f;
        glm::quat offRotation = glm::angleAxis(offAngle, glm::vec3(1.0f, 0.0f, 0.0f));
        float angle = minAngle - 0.7f * (TWO_PI - (maxAngle - minAngle));
        glm::quat rotation = glm::angleAxis(angle, yAxis);
        rotation = offRotation * glm::angleAxis(angle, yAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should clamp()" << std::endl;
        }
        if (rotation == newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should change rotation" << std::endl;
        }
        glm::quat expectedRotation = glm::angleAxis(maxAngle, yAxis);
        float qDot = glm::dot(expectedRotation, newRotation);
        if (fabsf(qDot - 1.0f) > ACCEPTABLE_ERROR) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint rotation = " << newRotation << " but expected " << expectedRotation << std::endl;
        }
    }
    delete c;
}

void AngularConstraintTests::testConeRollerConstraint() {
    float minAngleX = -PI / 5.0f;
    float minAngleY = -PI / 5.0f;
    float minAngleZ = -PI / 8.0f;

    float maxAngleX = PI / 4.0f;
    float maxAngleY = PI / 3.0f;
    float maxAngleZ = PI / 4.0f;

    glm::vec3 minAngles(minAngleX, minAngleY, minAngleZ);
    glm::vec3 maxAngles(maxAngleX, maxAngleY, maxAngleZ);
    AngularConstraint* c = AngularConstraint::newAngularConstraint(minAngles, maxAngles);

    float expectedConeAngle = 0.25 * (maxAngleX - minAngleX + maxAngleY - minAngleY);
    glm::vec3 middleAngles = 0.5f * (maxAngles + minAngles);
    glm::quat yaw = glm::angleAxis(middleAngles[1], glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat pitch = glm::angleAxis(middleAngles[0], glm::vec3(1.0f, 0.0f, 0.0f));
    glm::vec3 expectedConeAxis = pitch * yaw * glm::vec3(0.0f, 0.0f, 1.0f);

    glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
    glm::vec3 perpAxis = glm::normalize(xAxis - glm::dot(xAxis, expectedConeAxis) * expectedConeAxis);

    if (!c) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: newAngularConstraint() should make a constraint" << std::endl;
    }
    {   // test in middle of constraint
        glm::vec3 angles(PI/20.0f, 0.0f, PI/10.0f);
        glm::quat rotation(angles);

        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should not clamp()" << std::endl;
        }
        if (rotation != newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should not change rotation" << std::endl;
        }
    }
    float deltaAngle = 0.001f;
    {   // test just inside edge of cone 
        glm::quat rotation = glm::angleAxis(expectedConeAngle - deltaAngle, perpAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should not clamp()" << std::endl;
        }
        if (rotation != newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should not change rotation" << std::endl;
        }
    }
    {   // test just outside edge of cone
        glm::quat rotation = glm::angleAxis(expectedConeAngle + deltaAngle, perpAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should clamp()" << std::endl;
        }
        if (rotation == newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should change rotation" << std::endl;
        }
    }
    {   // test just inside min edge of roll
        glm::quat rotation = glm::angleAxis(minAngleZ + deltaAngle, expectedConeAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should not clamp()" << std::endl;
        }
        if (rotation != newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should not change rotation" << std::endl;
        }
    }
    {   // test just inside max edge of roll
        glm::quat rotation = glm::angleAxis(maxAngleZ - deltaAngle, expectedConeAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should not clamp()" << std::endl;
        }
        if (rotation != newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should not change rotation" << std::endl;
        }
    }
    {   // test just outside min edge of roll
        glm::quat rotation = glm::angleAxis(minAngleZ - deltaAngle, expectedConeAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should clamp()" << std::endl;
        }
        if (rotation == newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should change rotation" << std::endl;
        }
        glm::quat expectedRotation = glm::angleAxis(minAngleZ, expectedConeAxis);
        if (fabsf(1.0f - glm::dot(newRotation, expectedRotation)) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: rotation = " << newRotation << " but expected " << expectedRotation << std::endl;
        }
    }
    {   // test just outside max edge of roll
        glm::quat rotation = glm::angleAxis(maxAngleZ + deltaAngle, expectedConeAxis);
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should clamp()" << std::endl;
        }
        if (rotation == newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should change rotation" << std::endl;
        }
        glm::quat expectedRotation = glm::angleAxis(maxAngleZ, expectedConeAxis);
        if (fabsf(1.0f - glm::dot(newRotation, expectedRotation)) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: rotation = " << newRotation << " but expected " << expectedRotation << std::endl;
        }
    }
    deltaAngle = 0.25f * expectedConeAngle;
    {   // test far outside cone and min roll
        glm::quat roll = glm::angleAxis(minAngleZ - deltaAngle, expectedConeAxis);
        glm::quat pitchYaw = glm::angleAxis(expectedConeAngle + deltaAngle, perpAxis);
        glm::quat rotation = pitchYaw * roll;
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should clamp()" << std::endl;
        }
        if (rotation == newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should change rotation" << std::endl;
        }
        glm::quat expectedRoll = glm::angleAxis(minAngleZ, expectedConeAxis);
        glm::quat expectedPitchYaw = glm::angleAxis(expectedConeAngle, perpAxis);
        glm::quat expectedRotation = expectedPitchYaw * expectedRoll;
        if (fabsf(1.0f - glm::dot(newRotation, expectedRotation)) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: rotation = " << newRotation << " but expected " << expectedRotation << std::endl;
        }
    }
    {   // test far outside cone and max roll
        glm::quat roll = glm::angleAxis(maxAngleZ + deltaAngle, expectedConeAxis);
        glm::quat pitchYaw = glm::angleAxis(- expectedConeAngle - deltaAngle, perpAxis);
        glm::quat rotation = pitchYaw * roll;
    
        glm::quat newRotation = rotation;
        bool constrained = c->clamp(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should clamp()" << std::endl;
        }
        if (rotation == newRotation) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: ConeRollerConstraint should change rotation" << std::endl;
        }
        glm::quat expectedRoll = glm::angleAxis(maxAngleZ, expectedConeAxis);
        glm::quat expectedPitchYaw = glm::angleAxis(- expectedConeAngle, perpAxis);
        glm::quat expectedRotation = expectedPitchYaw * expectedRoll;
        if (fabsf(1.0f - glm::dot(newRotation, expectedRotation)) > EPSILON) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: rotation = " << newRotation << " but expected " << expectedRotation << std::endl;
        }
    }
    delete c;
}

void AngularConstraintTests::runAllTests() {
    testHingeConstraint();
    testConeRollerConstraint();
}
