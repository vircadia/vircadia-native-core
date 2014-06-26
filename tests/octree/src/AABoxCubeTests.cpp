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

#include <QDebug>

#include <AABox.h>
#include <AACube.h>

#include "AABoxCubeTests.h"

void AABoxCubeTests::AABoxCubeTests() {
    qDebug() << "******************************************************************************************";
    qDebug() << "AABoxCubeTests::AABoxCubeTests()";

    {    
        qDebug() << "Test 1: AABox.findRayIntersection() inside out MIN_X_FACE";

        glm::vec3 corner(0.0f, 0.0f, 0.0f);
        float size = 1.0f;
        
        AABox box(corner, size);
        glm::vec3 origin(0.5f, 0.5f, 0.5f);
        glm::vec3 direction(-1.0f, 0.0f, 0.0f);
        float distance;
        BoxFace face;

        bool intersects = box.findRayIntersection(origin, direction, distance, face);
        
        if (intersects && distance == 0.5f && face == MIN_X_FACE) {
            qDebug() << "Test 1: PASSED";
        } else {
            qDebug() << "intersects=" << intersects << "expected=" << true;
            qDebug() << "distance=" << distance << "expected=" << 0.5f;
            qDebug() << "face=" << face << "expected=" << MIN_X_FACE;
        
        }
    }

    {    
        qDebug() << "Test 2: AABox.findRayIntersection() inside out MAX_X_FACE";

        glm::vec3 corner(0.0f, 0.0f, 0.0f);
        float size = 1.0f;
        
        AABox box(corner, size);
        glm::vec3 origin(0.5f, 0.5f, 0.5f);
        glm::vec3 direction(1.0f, 0.0f, 0.0f);
        float distance;
        BoxFace face;

        bool intersects = box.findRayIntersection(origin, direction, distance, face);
        
        if (intersects && distance == 0.5f && face == MAX_X_FACE) {
            qDebug() << "Test 2: PASSED";
        } else {
            qDebug() << "intersects=" << intersects << "expected=" << true;
            qDebug() << "distance=" << distance << "expected=" << 0.5f;
            qDebug() << "face=" << face << "expected=" << MAX_X_FACE;
        
        }
    }

    {    
        qDebug() << "Test 3: AABox.findRayIntersection() outside in";

        glm::vec3 corner(0.5f, 0.0f, 0.0f);
        float size = 0.5f;
        
        AABox box(corner, size);
        glm::vec3 origin(0.25f, 0.25f, 0.25f);
        glm::vec3 direction(1.0f, 0.0f, 0.0f);
        float distance;
        BoxFace face;

        bool intersects = box.findRayIntersection(origin, direction, distance, face);
        
        if (intersects && distance == 0.25f && face == MIN_X_FACE) {
            qDebug() << "Test 3: PASSED";
        } else {
            qDebug() << "intersects=" << intersects << "expected=" << true;
            qDebug() << "distance=" << distance << "expected=" << 0.5f;
            qDebug() << "face=" << face << "expected=" << MIN_X_FACE;
        
        }
    }

    qDebug() << "******************************************************************************************";
}

void AABoxCubeTests::runAllTests() {
    AABoxCubeTests();
}
