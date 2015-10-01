//
//  AABoxCubeTests.h
//  tests/octree/src
//
//  Created by Brad Hefta-Gaub on 06/04/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <AABox.h>
#include <AACube.h>

#include "AABoxCubeTests.h"

QTEST_MAIN(AABoxCubeTests)

void AABoxCubeTests::raycastOutHitsXMinFace() {
    // Raycast inside out
    glm::vec3 corner(0.0f, 0.0f, 0.0f);
    float size = 1.0f;
    
    AABox box(corner, size);
    glm::vec3 origin(0.5f, 0.5f, 0.5f);
    glm::vec3 direction(-1.0f, 0.0f, 0.0f);
    float distance;
    BoxFace face;
    glm::vec3 surfaceNormal;

    bool intersects = box.findRayIntersection(origin, direction, distance, face, surfaceNormal);
    
    QCOMPARE(intersects, true);
    QCOMPARE(distance, 0.5f);
    QCOMPARE(face, MIN_X_FACE);
}

void AABoxCubeTests::raycastOutHitsXMaxFace () {
    // Raycast inside out
    glm::vec3 corner(0.0f, 0.0f, 0.0f);
    float size = 1.0f;
    
    AABox box(corner, size);
    glm::vec3 origin(0.5f, 0.5f, 0.5f);
    glm::vec3 direction(1.0f, 0.0f, 0.0f);
    float distance;
    BoxFace face;
    glm::vec3 surfaceNormal;

    bool intersects = box.findRayIntersection(origin, direction, distance, face, surfaceNormal);

    QCOMPARE(intersects, true);
    QCOMPARE(distance, 0.5f);
    QCOMPARE(face, MAX_X_FACE);
}
void AABoxCubeTests::raycastInHitsXMinFace () {
    // Raycast outside in
    glm::vec3 corner(0.5f, 0.0f, 0.0f);
    float size = 0.5f;
    
    AABox box(corner, size);
    glm::vec3 origin(0.25f, 0.25f, 0.25f);
    glm::vec3 direction(1.0f, 0.0f, 0.0f);
    float distance;
    BoxFace face;
    glm::vec3 surfaceNormal;

    bool intersects = box.findRayIntersection(origin, direction, distance, face, surfaceNormal);

    QCOMPARE(intersects, true);
    QCOMPARE(distance, 0.25f);
    QCOMPARE(face, MIN_X_FACE);
}

