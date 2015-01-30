//
//  Util.cpp
//  interface/src
//
//  Created by Philip Rosedale on 8/24/12.
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include <cstring>
#include <time.h>
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/detail/func_common.hpp>

#include <QThread>

#include <SharedUtil.h>
#include <TextRenderer.h>

#include "InterfaceConfig.h"
#include "world.h"
#include "Application.h"

#include "Util.h"

using namespace std;

void renderWorldBox() {
    auto geometryCache = DependencyManager::get<GeometryCache>();

    //  Show edge of world
    glm::vec3 red(1.0f, 0.0f, 0.0f);
    glm::vec3 green(0.0f, 1.0f, 0.0f);
    glm::vec3 blue(0.0f, 0.0f, 1.0f);
    glm::vec3 grey(0.5f, 0.5f, 0.5f);

    glDisable(GL_LIGHTING);
    glLineWidth(1.0);
    geometryCache->renderLine(glm::vec3(0, 0, 0), glm::vec3(TREE_SCALE, 0, 0), red);
    geometryCache->renderLine(glm::vec3(0, 0, 0), glm::vec3(0, TREE_SCALE, 0), green);
    geometryCache->renderLine(glm::vec3(0, 0, 0), glm::vec3(0, 0, TREE_SCALE), blue);
    geometryCache->renderLine(glm::vec3(0, 0, TREE_SCALE), glm::vec3(TREE_SCALE, 0, TREE_SCALE), grey);
    geometryCache->renderLine(glm::vec3(TREE_SCALE, 0, TREE_SCALE), glm::vec3(TREE_SCALE, 0, 0), grey);
    
    
    //  Draw meter markers along the 3 axis to help with measuring things
    const float MARKER_DISTANCE = 1.0f;
    const float MARKER_RADIUS = 0.05f;
    glEnable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(MARKER_DISTANCE, 0, 0);
    geometryCache->renderSphere(MARKER_RADIUS, 10, 10, red);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, MARKER_DISTANCE, 0);
    geometryCache->renderSphere(MARKER_RADIUS, 10, 10, green);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, 0, MARKER_DISTANCE);
    geometryCache->renderSphere(MARKER_RADIUS, 10, 10, blue);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(MARKER_DISTANCE, 0, MARKER_DISTANCE);
    geometryCache->renderSphere(MARKER_RADIUS, 10, 10, grey);
    glPopMatrix();

}

//  Return a random vector of average length 1
const glm::vec3 randVector() {
    return glm::vec3(randFloat() - 0.5f, randFloat() - 0.5f, randFloat() - 0.5f) * 2.0f;
}

static TextRenderer* textRenderer(int mono) {
    static TextRenderer* monoRenderer = TextRenderer::getInstance(MONO_FONT_FAMILY); 
    static TextRenderer* proportionalRenderer = TextRenderer::getInstance(SANS_FONT_FAMILY,
        -1, -1, false, TextRenderer::SHADOW_EFFECT);
    static TextRenderer* inconsolataRenderer = TextRenderer::getInstance(INCONSOLATA_FONT_FAMILY, -1, INCONSOLATA_FONT_WEIGHT, 
        false);
    switch (mono) {
        case 1:
            return monoRenderer;
        case 2:
            return inconsolataRenderer;
        case 0:
        default:
            return proportionalRenderer;
    }
}

int widthText(float scale, int mono, char const* string) {
    return textRenderer(mono)->computeWidth(string) * (scale / 0.10);
}

void drawText(int x, int y, float scale, float radians, int mono,
              char const* string, const float* color) {
    //
    //  Draws text on screen as stroked so it can be resized
    //
    glPushMatrix();
    glTranslatef(static_cast<float>(x), static_cast<float>(y), 0.0f);


    glRotated(double(radians * DEGREES_PER_RADIAN), 0.0, 0.0, 1.0);
    glScalef(scale / 0.1f, scale / 0.1f, 1.0f);

    glm::vec4 colorV4 = {color[0], color[1], color[3], 1.0f };
    textRenderer(mono)->draw(0, 0, string, colorV4);
    glPopMatrix();
}

void renderCollisionOverlay(int width, int height, float magnitude, float red, float blue, float green) {
    const float MIN_VISIBLE_COLLISION = 0.01f;
    if (magnitude > MIN_VISIBLE_COLLISION) {
        DependencyManager::get<GeometryCache>()->renderQuad(0, 0, width, height, glm::vec4(red, blue, green, magnitude));
    }
}

//  Do some basic timing tests and report the results
void runTimingTests() {
    //  How long does it take to make a call to get the time?
    const int numTests = 1000000;
    int* iResults = (int*)malloc(sizeof(int) * numTests);
    float fTest = 1.0;
    float* fResults = (float*)malloc(sizeof(float) * numTests);
    QElapsedTimer startTime;
    startTime.start();
    float elapsedUsecs;
    
    float NSEC_TO_USEC = 1.0f / 1000.0f;
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qDebug("QElapsedTimer::nsecElapsed() usecs: %f", elapsedUsecs);
    
    // Test sleep functions for accuracy
    startTime.start();
    QThread::msleep(1);
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qDebug("QThread::msleep(1) ms: %f", elapsedUsecs / 1000.0f);

    startTime.start();
    QThread::sleep(1);
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qDebug("QThread::sleep(1) ms: %f", elapsedUsecs / 1000.0f);

    startTime.start();
    usleep(1);
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qDebug("usleep(1) ms: %f", elapsedUsecs / 1000.0f);

    startTime.start();
    usleep(10);
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qDebug("usleep(10) ms: %f", elapsedUsecs / 1000.0f);

    startTime.start();
    usleep(100);
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qDebug("usleep(100) ms: %f", elapsedUsecs / 1000.0f);

    startTime.start();
    usleep(1000);
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qDebug("usleep(1000) ms: %f", elapsedUsecs / 1000.0f);

    startTime.start();
    usleep(15000);
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qDebug("usleep(15000) ms: %f", elapsedUsecs / 1000.0f);

    // Random number generation
    startTime.start();
    for (int i = 0; i < numTests; i++) {
        iResults[i] = rand();
    }
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qDebug("rand() stored in array usecs: %f, first result:%d", elapsedUsecs / (float) numTests, iResults[0]);

    // Random number generation using randFloat()
    startTime.start();
    for (int i = 0; i < numTests; i++) {
        fResults[i] = randFloat();
    }
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qDebug("randFloat() stored in array usecs: %f, first result: %f", elapsedUsecs / (float) numTests, fResults[0]);

    free(iResults);
    free(fResults);

    //  PowF function
    fTest = 1145323.2342f;
    startTime.start();
    for (int i = 0; i < numTests; i++) {
        fTest = powf(fTest, 0.5f);
    }
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qDebug("powf(f, 0.5) usecs: %f", elapsedUsecs / (float) numTests);

    //  Vector Math
    float distance;
    glm::vec3 pointA(randVector()), pointB(randVector());
    startTime.start();
    for (int i = 0; i < numTests; i++) {
        //glm::vec3 temp = pointA - pointB;
        //float distanceSquared = glm::dot(temp, temp);
        distance = glm::distance(pointA, pointB);
    }
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qDebug("vector math usecs: %f [%f usecs total for %d tests], last result:%f",
           elapsedUsecs / (float) numTests, elapsedUsecs, numTests, distance);

    //  Vec3 test
    glm::vec3 vecA(randVector()), vecB(randVector());
    float result;
    
    startTime.start();
    for (int i = 0; i < numTests; i++) {
        glm::vec3 temp = vecA-vecB;
        result = glm::dot(temp,temp);
    }
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qDebug("vec3 assign and dot() usecs: %f, last result:%f", elapsedUsecs / (float) numTests, result);
}

bool rayIntersectsSphere(const glm::vec3& rayStarting, const glm::vec3& rayNormalizedDirection,
        const glm::vec3& sphereCenter, float sphereRadius, float& distance) {
    glm::vec3 relativeOrigin = rayStarting - sphereCenter;

    // compute the b, c terms of the quadratic equation (a is dot(direction, direction), which is one)
    float b = 2.0f * glm::dot(rayNormalizedDirection, relativeOrigin);
    float c = glm::dot(relativeOrigin, relativeOrigin) - sphereRadius * sphereRadius;

    // compute the radicand of the quadratic.  if less than zero, there's no intersection
    float radicand = b * b - 4.0f * c;
    if (radicand < 0.0f) {
        return false;
    }

    // compute the first solution of the quadratic
    float root = sqrtf(radicand);
    float firstSolution = -b - root;
    if (firstSolution > 0.0f) {
        distance = firstSolution / 2.0f;
        return true; // origin is outside the sphere
    }

    // now try the second solution
    float secondSolution = -b + root;
    if (secondSolution > 0.0f) {
        distance = 0.0f;
        return true; // origin is inside the sphere
    }

    return false;
}

bool pointInSphere(glm::vec3& point, glm::vec3& sphereCenter, double sphereRadius) {
    glm::vec3 diff = point - sphereCenter;
    double mag = sqrt(glm::dot(diff, diff));
    if (mag <= sphereRadius) {
        return true;
    }
    return false;
}
