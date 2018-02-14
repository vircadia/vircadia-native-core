//
//  SpaceTests.cpp
//  tests/physics/src
//
//  Created by Andrew Meadows on 2017.01.26
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SpaceTests.h"

#include <iostream>

#include <workload/Space.h>
#include <StreamUtils.h>
#include <SharedUtil.h>


const float INV_SQRT_3 = 1.0f / sqrtf(3.0f);

QTEST_MAIN(SpaceTests)

void SpaceTests::testOverlaps() {
    workload::Space space;
    using Changes = std::vector<workload::Space::Change>;
    using Views = std::vector<workload::Space::View>;

    glm::vec3 viewCenter(0.0f, 0.0f, 0.0f);
    float near = 1.0f;
    float mid = 2.0f;
    float far = 3.0f;

    Views views;
    views.push_back(workload::Space::View(viewCenter, near, mid, far));
    space.setViews(views);

    int32_t proxyId = 0;
    const float DELTA = 0.001f;
    float proxyRadius = 0.5f;
    glm::vec3 proxyPosition = viewCenter + glm::vec3(0.0f, 0.0f, far + proxyRadius + DELTA);
    workload::Space::Sphere proxySphere(proxyPosition, proxyRadius);

    { // create very_far proxy
        proxyId = space.createProxy(proxySphere);
        QVERIFY(space.getNumObjects() == 1);

        Changes changes;
        space.categorizeAndGetChanges(changes);
        QVERIFY(changes.size() == 0);
    }

    { // move proxy far
        float newRadius = 1.0f;
        glm::vec3 newPosition = viewCenter + glm::vec3(0.0f, 0.0f, far + newRadius - DELTA);
        workload::Space::Sphere newSphere(newPosition, newRadius);
        space.updateProxy(proxyId, newSphere);
        Changes changes;
        space.categorizeAndGetChanges(changes);
        QVERIFY(changes.size() == 1);
        QVERIFY(changes[0].proxyId == proxyId);
        QVERIFY(changes[0].region == workload::Space::REGION_FAR);
        QVERIFY(changes[0].prevRegion == workload::Space::REGION_UNKNOWN);
    }

    { // move proxy mid
        float newRadius = 1.0f;
        glm::vec3 newPosition = viewCenter + glm::vec3(0.0f, 0.0f, mid + newRadius - DELTA);
        workload::Space::Sphere newSphere(newPosition, newRadius);
        space.updateProxy(proxyId, newSphere);
        Changes changes;
        space.categorizeAndGetChanges(changes);
        QVERIFY(changes.size() == 1);
        QVERIFY(changes[0].proxyId == proxyId);
        QVERIFY(changes[0].region == workload::Space::REGION_MIDDLE);
        QVERIFY(changes[0].prevRegion == workload::Space::REGION_FAR);
    }

    { // move proxy near
        float newRadius = 1.0f;
        glm::vec3 newPosition = viewCenter + glm::vec3(0.0f, 0.0f, near + newRadius - DELTA);
        workload::Space::Sphere newSphere(newPosition, newRadius);
        space.updateProxy(proxyId, newSphere);
        Changes changes;
        space.categorizeAndGetChanges(changes);
        QVERIFY(changes.size() == 1);
        QVERIFY(changes[0].proxyId == proxyId);
        QVERIFY(changes[0].region == workload::Space::REGION_NEAR);
        QVERIFY(changes[0].prevRegion == workload::Space::REGION_MIDDLE);
    }

    { // delete proxy
        // NOTE: atm deleting a proxy doesn't result in a "Change"
        space.deleteProxy(proxyId);
        Changes changes;
        space.categorizeAndGetChanges(changes);
        QVERIFY(changes.size() == 0);
        QVERIFY(space.getNumObjects() == 0);
    }
}

#ifdef MANUAL_TEST

const float WORLD_WIDTH = 1000.0f;
const float MIN_RADIUS = 1.0f;
const float MAX_RADIUS = 100.0f;

float randomFloat() {
    return 2.0f * ((float)rand() / (float)RAND_MAX) - 1.0f;
}

glm::vec3 randomVec3() {
    glm::vec3 v(randomFloat(), randomFloat(), randomFloat());
    return v;
}

void generateSpheres(uint32_t numProxies, std::vector<workload::Space::Sphere>& spheres) {
    spheres.reserve(numProxies);
    for (uint32_t i = 0; i < numProxies; ++i) {
        workload::Space::Sphere sphere(
                WORLD_WIDTH * randomFloat(),
                WORLD_WIDTH * randomFloat(),
                WORLD_WIDTH * randomFloat(),
                (MIN_RADIUS + (MAX_RADIUS - MIN_RADIUS)) * randomFloat());
        spheres.push_back(sphere);
    }
}

void generatePositions(uint32_t numProxies, std::vector<glm::vec3>& positions) {
    positions.reserve(numProxies);
    for (uint32_t i = 0; i < numProxies; ++i) {
        positions.push_back(WORLD_WIDTH * randomVec3());
    }
}

void generateRadiuses(uint32_t numRadiuses, std::vector<float>& radiuses) {
    radiuses.reserve(numRadiuses);
    for (uint32_t i = 0; i < numRadiuses; ++i) {
        radiuses.push_back(MIN_RADIUS + (MAX_RADIUS - MIN_RADIUS) * randomFloat());
    }
}

void SpaceTests::benchmark() {
    uint32_t numProxies[] = { 100, 1000, 10000, 100000 };
    uint32_t numTests = 4;
    std::vector<uint64_t> timeToAddAll;
    std::vector<uint64_t> timeToRemoveAll;
    std::vector<uint64_t> timeToMoveView;
    std::vector<uint64_t> timeToMoveProxies;
    for (uint32_t i = 0; i < numTests; ++i) {

        workload::Space space;

        { // build the views
            std::vector<glm::vec3> viewPositions;
            viewPositions.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
            viewPositions.push_back(glm::vec3(0.0f, 0.0f, 0.1f * WORLD_WIDTH));
            float radius0 = 0.25f * WORLD_WIDTH;
            float radius1 = 0.50f * WORLD_WIDTH;
            float radius2 = 0.75f * WORLD_WIDTH;
            std::vector<workload::Space::View> views;
            views.push_back(workload::Space::View(viewPositions[0], radius0, radius1, radius2));
            views.push_back(workload::Space::View(viewPositions[1], radius0, radius1, radius2));
            space.setViews(views);
        }

        // build the proxies
        uint32_t n = numProxies[i];
        std::vector<workload::Space::Sphere> proxySpheres;
        generateSpheres(n, proxySpheres);
        std::vector<int32_t> proxyKeys;
        proxyKeys.reserve(n);

        // measure time to put proxies in the space
        uint64_t startTime = usecTimestampNow();
        for (uint32_t j = 0; j < n; ++j) {
            int32_t key = space.createProxy(proxySpheres[j]);
            proxyKeys.push_back(key);
        }
        uint64_t usec = usecTimestampNow() - startTime;
        timeToAddAll.push_back(usec);

        {
            // build the views
            std::vector<glm::vec3> viewPositions;
            viewPositions.push_back(glm::vec3(1.0f, 2.0f, 3.0f));
            viewPositions.push_back(glm::vec3(1.0f, 2.0f, 3.0f + 0.1f * WORLD_WIDTH));
            float radius0 = 0.25f * WORLD_WIDTH;
            float radius1 = 0.50f * WORLD_WIDTH;
            float radius2 = 0.75f * WORLD_WIDTH;
            std::vector<workload::Space::View> views;
            views.push_back(workload::Space::View(viewPositions[0], radius0, radius1, radius2));
            views.push_back(workload::Space::View(viewPositions[1], radius0, radius1, radius2));
            space.setViews(views);
        }

        // measure time to categorizeAndGetChanges everything
        std::vector<workload::Space::Change> changes;
        startTime = usecTimestampNow();
        space.categorizeAndGetChanges(changes);
        usec = usecTimestampNow() - startTime;
        timeToMoveView.push_back(usec);

        // move every 10th proxy around
        const float proxySpeed = 1.0f;
        std::vector<workload::Space::Sphere> newSpheres;
        uint32_t numMovingProxies = n / 10;
        uint32_t jstep = n / numMovingProxies;
        uint32_t maxJ = numMovingProxies * jstep - 1;
        glm::vec3 direction;
        for (uint32_t j = 0; j < maxJ - jstep; j += jstep) {
            glm::vec3 position = (glm::vec3)proxySpheres[j];
            glm::vec3 destination = (glm::vec3)proxySpheres[j + jstep];
            direction = glm::normalize(destination - position);
            glm::vec3 newPosition = position + proxySpeed * direction;
            newSpheres.push_back(workload::Space::Sphere(newPosition, proxySpheres[j].w));
        }
        glm::vec3 position = (glm::vec3)proxySpheres[maxJ = jstep];
        glm::vec3 destination = (glm::vec3)proxySpheres[0];
        direction = glm::normalize(destination - position);
        direction = position + proxySpeed * direction;
        newSpheres.push_back(workload::Space::Sphere(direction, proxySpheres[0].w));
        uint32_t k = 0;
        startTime = usecTimestampNow();
        for (uint32_t j = 0; j < maxJ; j += jstep) {
            space.updateProxy(proxyKeys[j], newSpheres[k++]);
        }
        changes.clear();
        space.categorizeAndGetChanges(changes);
        usec = usecTimestampNow() - startTime;
        timeToMoveProxies.push_back(usec);

        // measure time to remove proxies from space
        startTime = usecTimestampNow();
        for (uint32_t j = 0; j < n; ++j) {
            space.deleteProxy(proxyKeys[j]);
        }
        usec = usecTimestampNow() - startTime;
        timeToRemoveAll.push_back(usec);
    }

    std::cout << "[numProxies, timeToAddAll] = [" << std::endl;
    for (uint32_t i = 0; i < timeToAddAll.size(); ++i) {
        uint32_t n = numProxies[i];
        std::cout << "    " << n << ", " << timeToAddAll[i] << std::endl;
    }
    std::cout << "];" << std::endl;

    std::cout << "[numProxies, timeToMoveView] = [" << std::endl;
    for (uint32_t i = 0; i < timeToMoveView.size(); ++i) {
        uint32_t n = numProxies[i];
        std::cout << "    " << n << ", " << timeToMoveView[i] << std::endl;
    }
    std::cout << "];" << std::endl;

    std::cout << "[numProxies, timeToMoveProxies] = [" << std::endl;
    for (uint32_t i = 0; i < timeToMoveProxies.size(); ++i) {
        uint32_t n = numProxies[i];
        std::cout << "    " << n << "/10, " << timeToMoveProxies[i] << std::endl;
    }
    std::cout << "];" << std::endl;

    std::cout << "[numProxies, timeToRemoveAll] = [" << std::endl;
    for (uint32_t i = 0; i < timeToRemoveAll.size(); ++i) {
        uint32_t n = numProxies[i];
        std::cout << "    " << n << ", " << timeToRemoveAll[i] << std::endl;
    }
    std::cout << "];" << std::endl;
}

#endif // MANUAL_TEST
