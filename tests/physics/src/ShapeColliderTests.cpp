//
//  ShapeColliderTests.cpp
//  physics-tests
//
//  Created by Andrew Meadows on 2014.02.21
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

//#include <stdio.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <CollisionInfo.h>
#include <ShapeCollider.h>
#include <SharedUtil.h>
#include <SphereShape.h>

#include "PhysicsTestUtil.h"
#include "ShapeColliderTests.h"

const glm::vec3 origin(0.f);

void ShapeColliderTests::sphereSphere() {
    CollisionInfo collision;

    float radius = 2.f;
    SphereShape sphereA(radius, origin);
    SphereShape sphereB(radius, origin);

    glm::vec3 translation(0.5f * radius, 0.f, 0.f);
    glm::quat rotation;

    // these spheres should be touching
    bool touching = ShapeCollider::sphereSphere(&sphereA, &sphereB, rotation, translation, collision);
    if (!touching) {
        std::cout << __FILE__ << ":" << __LINE__
            << " ERROR: sphereA and sphereB do not touch" << std::endl;
    }

    // TODO: verify the collision info is good...
    // penetration should point from A into B
}

/*
void ShapeColliderTests::test2() {
}
*/

void ShapeColliderTests::runAllTests() {
    ShapeColliderTests::sphereSphere();
//    ShapeColliderTests::test2();
}
