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
        bool constrained = c->applyTo(newRotation);
        if (constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should not applyTo()" << std::endl;
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
        bool constrained = c->applyTo(newRotation);
        if (constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should not applyTo()" << std::endl;
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
        bool constrained = c->applyTo(newRotation);
        if (constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should not applyTo()" << std::endl;
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
        bool constrained = c->applyTo(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should applyTo()" << std::endl;
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
        bool constrained = c->applyTo(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should applyTo()" << std::endl;
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
        bool constrained = c->applyTo(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should applyTo()" << std::endl;
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
        bool constrained = c->applyTo(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should applyTo()" << std::endl;
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
        bool constrained = c->applyTo(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should applyTo()" << std::endl;
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
        bool constrained = c->applyTo(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should applyTo()" << std::endl;
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
        bool constrained = c->applyTo(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should applyTo()" << std::endl;
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
        bool constrained = c->applyTo(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should applyTo()" << std::endl;
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
        bool constrained = c->applyTo(newRotation);
        if (!constrained) {
            std::cout << __FILE__ << ":" << __LINE__
                << " ERROR: HingeConstraint should applyTo()" << std::endl;
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
}

void AngularConstraintTests::testConeRollerConstraint() {
}

void AngularConstraintTests::runAllTests() {
    testHingeConstraint();
}
