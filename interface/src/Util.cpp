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

#include <SharedUtil.h>

#include <QThread>

#include "InterfaceConfig.h"
#include "ui/TextRenderer.h"
#include "VoxelConstants.h"
#include "world.h"
#include "Application.h"

#include "Util.h"

using namespace std;

// no clue which versions are affected...
#define WORKAROUND_BROKEN_GLUT_STROKES
// see http://www.opengl.org/resources/libraries/glut/spec3/node78.html



void renderWorldBox() {
    //  Show edge of world
    float red[] = {1, 0, 0};
    float green[] = {0, 1, 0};
    float blue[] = {0, 0, 1};
    float gray[] = {0.5, 0.5, 0.5};

    glDisable(GL_LIGHTING);
    glLineWidth(1.0);
    glBegin(GL_LINES);
    glColor3fv(red);
    glVertex3f(0, 0, 0);
    glVertex3f(TREE_SCALE, 0, 0);
    glColor3fv(green);
    glVertex3f(0, 0, 0);
    glVertex3f(0, TREE_SCALE, 0);
    glColor3fv(blue);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, TREE_SCALE);
    glColor3fv(gray);
    glVertex3f(0, 0, TREE_SCALE);
    glVertex3f(TREE_SCALE, 0, TREE_SCALE);
    glVertex3f(TREE_SCALE, 0, TREE_SCALE);
    glVertex3f(TREE_SCALE, 0, 0);
    glEnd();
    //  Draw meter markers along the 3 axis to help with measuring things
    const float MARKER_DISTANCE = 1.0f;
    const float MARKER_RADIUS = 0.05f;
    glEnable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(MARKER_DISTANCE, 0, 0);
    glColor3fv(red);
    DependencyManager::get<GeometryCache>()->renderSphere(MARKER_RADIUS, 10, 10);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, MARKER_DISTANCE, 0);
    glColor3fv(green);
    DependencyManager::get<GeometryCache>()->renderSphere(MARKER_RADIUS, 10, 10);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, 0, MARKER_DISTANCE);
    glColor3fv(blue);
    DependencyManager::get<GeometryCache>()->renderSphere(MARKER_RADIUS, 10, 10);
    glPopMatrix();
    glPushMatrix();
    glColor3fv(gray);
    glTranslatef(MARKER_DISTANCE, 0, MARKER_DISTANCE);
    DependencyManager::get<GeometryCache>()->renderSphere(MARKER_RADIUS, 10, 10);
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
    glColor3fv(color);
    glRotated(double(radians * DEGREES_PER_RADIAN), 0.0, 0.0, 1.0);
    glScalef(scale / 0.1f, scale / 0.1f, 1.0f);
    textRenderer(mono)->draw(0, 0, string);
    glPopMatrix();
}

void renderCollisionOverlay(int width, int height, float magnitude, float red, float blue, float green) {
    const float MIN_VISIBLE_COLLISION = 0.01f;
    if (magnitude > MIN_VISIBLE_COLLISION) {
        glColor4f(red, blue, green, magnitude);
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2d(width, 0);
        glVertex2d(width, height);
        glVertex2d(0, height);
        glEnd();
    }
}



void renderCircle(glm::vec3 position, float radius, glm::vec3 surfaceNormal, int numSides) {
    glm::vec3 perp1 = glm::vec3(surfaceNormal.y, surfaceNormal.z, surfaceNormal.x);
    glm::vec3 perp2 = glm::vec3(surfaceNormal.z, surfaceNormal.x, surfaceNormal.y);

    glBegin(GL_LINE_STRIP);

    for (int i=0; i<numSides+1; i++) {
        float r = ((float)i / (float)numSides) * TWO_PI;
        float s = radius * sinf(r);
        float c = radius * cosf(r);
        glVertex3f
        (
            position.x + perp1.x * s + perp2.x * c,
            position.y + perp1.y * s + perp2.y * c,
            position.z + perp1.z * s + perp2.z * c
        );
    }
    glEnd();
}


void renderBevelCornersRect(int x, int y, int width, int height, int bevelDistance) {
    glBegin(GL_POLYGON);
    
    // left side
    glVertex2f(x, y + bevelDistance);
    glVertex2f(x, y + height - bevelDistance);
    
    // top side
    glVertex2f(x + bevelDistance,  y + height);
    glVertex2f(x + width - bevelDistance, y + height);
    
    // right
    glVertex2f(x + width, y + height - bevelDistance);
    glVertex2f(x + width, y + bevelDistance);
    
    // bottom
    glVertex2f(x + width - bevelDistance,  y);
    glVertex2f(x +bevelDistance, y);

    glEnd();
}



void renderOrientationDirections(glm::vec3 position, const glm::quat& orientation, float size) {
	glm::vec3 pRight	= position + orientation * IDENTITY_RIGHT * size;
	glm::vec3 pUp		= position + orientation * IDENTITY_UP    * size;
	glm::vec3 pFront	= position + orientation * IDENTITY_FRONT * size;

	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_LINE_STRIP);
	glVertex3f(position.x, position.y, position.z);
	glVertex3f(pRight.x, pRight.y, pRight.z);
	glEnd();

	glColor3f(0.0f, 1.0f, 0.0f);
	glBegin(GL_LINE_STRIP);
	glVertex3f(position.x, position.y, position.z);
	glVertex3f(pUp.x, pUp.y, pUp.z);
	glEnd();

	glColor3f(0.0f, 0.0f, 1.0f);
	glBegin(GL_LINE_STRIP);
	glVertex3f(position.x, position.y, position.z);
	glVertex3f(pFront.x, pFront.y, pFront.z);
	glEnd();
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

float loadSetting(QSettings* settings, const char* name, float defaultValue) {
    float value = settings->value(name, defaultValue).toFloat();
    if (glm::isnan(value)) {
        value = defaultValue;
    }
    return value;
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
