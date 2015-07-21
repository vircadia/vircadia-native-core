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

#include <ByteCountCoding.h>
#include <SharedUtil.h>

#include "world.h"
#include "Application.h"
#include "InterfaceLogging.h"

#include "Util.h"

using namespace std;

void renderWorldBox(gpu::Batch& batch) {
    auto geometryCache = DependencyManager::get<GeometryCache>();

    //  Show edge of world
    static const glm::vec3 red(1.0f, 0.0f, 0.0f);
    static const glm::vec3 green(0.0f, 1.0f, 0.0f);
    static const glm::vec3 blue(0.0f, 0.0f, 1.0f);
    static const glm::vec3 grey(0.5f, 0.5f, 0.5f);

    auto transform = Transform{};
    batch.setModelTransform(transform);
    geometryCache->renderLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(TREE_SCALE, 0.0f, 0.0f), red);
    geometryCache->renderLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, TREE_SCALE, 0.0f), green);
    geometryCache->renderLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, TREE_SCALE), blue);
    geometryCache->renderLine(batch, glm::vec3(0.0f, 0.0f, TREE_SCALE), glm::vec3(TREE_SCALE, 0.0f, TREE_SCALE), grey);
    geometryCache->renderLine(batch, glm::vec3(TREE_SCALE, 0.0f, TREE_SCALE), glm::vec3(TREE_SCALE, 0.0f, 0.0f), grey);

    //  Draw meter markers along the 3 axis to help with measuring things
    const float MARKER_DISTANCE = 1.0f;
    const float MARKER_RADIUS = 0.05f;

    geometryCache->renderSphere(batch, MARKER_RADIUS, 10, 10, red);

    transform.setTranslation(glm::vec3(MARKER_DISTANCE, 0.0f, 0.0f));
    batch.setModelTransform(transform);
    geometryCache->renderSphere(batch, MARKER_RADIUS, 10, 10, red);

    transform.setTranslation(glm::vec3(0.0f, MARKER_DISTANCE, 0.0f));
    batch.setModelTransform(transform);
    geometryCache->renderSphere(batch, MARKER_RADIUS, 10, 10, green);

    transform.setTranslation(glm::vec3(0.0f, 0.0f, MARKER_DISTANCE));
    batch.setModelTransform(transform);
    geometryCache->renderSphere(batch, MARKER_RADIUS, 10, 10, blue);

    transform.setTranslation(glm::vec3(MARKER_DISTANCE, 0.0f, MARKER_DISTANCE));
    batch.setModelTransform(transform);
    geometryCache->renderSphere(batch, MARKER_RADIUS, 10, 10, grey);
}

//  Return a random vector of average length 1
const glm::vec3 randVector() {
    return glm::vec3(randFloat() - 0.5f, randFloat() - 0.5f, randFloat() - 0.5f) * 2.0f;
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
    qCDebug(interfaceapp, "QElapsedTimer::nsecElapsed() usecs: %f", (double)elapsedUsecs);

    // Test sleep functions for accuracy
    startTime.start();
    QThread::msleep(1);
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qCDebug(interfaceapp, "QThread::msleep(1) ms: %f", (double)(elapsedUsecs / 1000.0f));

    startTime.start();
    QThread::sleep(1);
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qCDebug(interfaceapp, "QThread::sleep(1) ms: %f", (double)(elapsedUsecs / 1000.0f));

    startTime.start();
    usleep(1);
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qCDebug(interfaceapp, "usleep(1) ms: %f", (double)(elapsedUsecs / 1000.0f));

    startTime.start();
    usleep(10);
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qCDebug(interfaceapp, "usleep(10) ms: %f", (double)(elapsedUsecs / 1000.0f));

    startTime.start();
    usleep(100);
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qCDebug(interfaceapp, "usleep(100) ms: %f", (double)(elapsedUsecs / 1000.0f));

    startTime.start();
    usleep(1000);
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qCDebug(interfaceapp, "usleep(1000) ms: %f", (double)(elapsedUsecs / 1000.0f));

    startTime.start();
    usleep(15000);
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qCDebug(interfaceapp, "usleep(15000) ms: %f", (double)(elapsedUsecs / 1000.0f));

    // Random number generation
    startTime.start();
    for (int i = 0; i < numTests; i++) {
        iResults[i] = rand();
    }
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qCDebug(interfaceapp, "rand() stored in array usecs: %f, first result:%d",
            (double)(elapsedUsecs / numTests), iResults[0]);

    // Random number generation using randFloat()
    startTime.start();
    for (int i = 0; i < numTests; i++) {
        fResults[i] = randFloat();
    }
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qCDebug(interfaceapp, "randFloat() stored in array usecs: %f, first result: %f",
            (double)(elapsedUsecs / numTests), (double)(fResults[0]));

    free(iResults);
    free(fResults);

    //  PowF function
    fTest = 1145323.2342f;
    startTime.start();
    for (int i = 0; i < numTests; i++) {
        fTest = powf(fTest, 0.5f);
    }
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qCDebug(interfaceapp, "powf(f, 0.5) usecs: %f", (double)(elapsedUsecs / (float) numTests));

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
    qCDebug(interfaceapp, "vector math usecs: %f [%f usecs total for %d tests], last result:%f",
            (double)(elapsedUsecs / (float) numTests), (double)elapsedUsecs, numTests, (double)distance);

    //  Vec3 test
    glm::vec3 vecA(randVector()), vecB(randVector());
    float result;

    startTime.start();
    for (int i = 0; i < numTests; i++) {
        glm::vec3 temp = vecA-vecB;
        result = glm::dot(temp,temp);
    }
    elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
    qCDebug(interfaceapp, "vec3 assign and dot() usecs: %f, last result:%f",
            (double)(elapsedUsecs / numTests), (double)result);


    quint64 BYTE_CODE_MAX_TEST_VALUE = 99999999;
    quint64 BYTE_CODE_TESTS_SKIP = 999;

    QByteArray extraJunk;
    const int EXTRA_JUNK_SIZE = 200;
    extraJunk.append((unsigned char)255);
    for (int i = 0; i < EXTRA_JUNK_SIZE; i++) {
        extraJunk.append(QString("junk"));
    }

    {
        startTime.start();
        quint64 tests = 0;
        quint64 failed = 0;
        for (quint64 value = 0; value < BYTE_CODE_MAX_TEST_VALUE; value += BYTE_CODE_TESTS_SKIP) {
            quint64 valueA = value; // usecTimestampNow();
            ByteCountCoded<quint64> codedValueA = valueA;
            QByteArray codedValueABuffer = codedValueA;
            codedValueABuffer.append(extraJunk);
            ByteCountCoded<quint64> decodedValueA;
            decodedValueA.decode(codedValueABuffer);
            quint64 valueADecoded = decodedValueA;
            tests++;
            if (valueA != valueADecoded) {
                qDebug() << "FAILED! value:" << valueA << "decoded:" << valueADecoded;
                failed++;
            }

        }
        elapsedUsecs = (float)startTime.nsecsElapsed() * NSEC_TO_USEC;
        qCDebug(interfaceapp) << "ByteCountCoded<quint64> usecs: " << elapsedUsecs
                                << "per test:" << (double) (elapsedUsecs / tests)
                                << "tests:" << tests
                                << "failed:" << failed;
    }
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

void runUnitTests() {

    quint64 LAST_TEST = 10;
    quint64 SKIP_BY = 1;
    
    for (quint64 value = 0; value <= LAST_TEST; value += SKIP_BY) {
        qDebug() << "value:" << value;

        ByteCountCoded<quint64> codedValue = value;
    
        QByteArray codedValueBuffer = codedValue;
        
        codedValueBuffer.append((unsigned char)255);
        codedValueBuffer.append(QString("junk"));
        
        qDebug() << "codedValueBuffer:";
        outputBufferBits((const unsigned char*)codedValueBuffer.constData(), codedValueBuffer.size());

        ByteCountCoded<quint64> valueDecoder;
        size_t bytesConsumed =  valueDecoder.decode(codedValueBuffer);
        quint64 valueDecoded = valueDecoder;
        qDebug() << "valueDecoded:" << valueDecoded;
        qDebug() << "bytesConsumed:" << bytesConsumed;
        

        if (value == valueDecoded) {
            qDebug() << "SUCCESS!";
        } else {
            qDebug() << "FAILED!";
        }

    }
}


