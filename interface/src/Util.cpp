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

#include "Util.h"

#include <iostream>
#include <cstring>
#include <time.h>
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/detail/func_common.hpp>

#include <QElapsedTimer>
#include <QThread>

#include <ByteCountCoding.h>
#include <GeometryCache.h>
#include <OctreeConstants.h>
#include <SharedUtil.h>

#include "InterfaceLogging.h"
#include "world.h"

using namespace std;

void renderWorldBox(RenderArgs* args, gpu::Batch& batch) {
    auto geometryCache = DependencyManager::get<GeometryCache>();

    //  Show center of world
    static const glm::vec3 RED(1.0f, 0.0f, 0.0f);
    static const glm::vec3 GREEN(0.0f, 1.0f, 0.0f);
    static const glm::vec3 BLUE(0.0f, 0.0f, 1.0f);
    static const glm::vec3 GREY(0.5f, 0.5f, 0.5f);
    static const glm::vec4 GREY4(0.5f, 0.5f, 0.5f, 1.0f);

    static const glm::vec4 DASHED_RED(1.0f, 0.0f, 0.0f, 1.0f);
    static const glm::vec4 DASHED_GREEN(0.0f, 1.0f, 0.0f, 1.0f);
    static const glm::vec4 DASHED_BLUE(0.0f, 0.0f, 1.0f, 1.0f);
    static const float DASH_LENGTH = 1.0f;
    static const float GAP_LENGTH = 1.0f;
    auto transform = Transform{};
    static std::array<int, 18> geometryIds;
    static std::once_flag initGeometryIds;
    std::call_once(initGeometryIds, [&] {
        for (size_t i = 0; i < geometryIds.size(); ++i) {
            geometryIds[i] = geometryCache->allocateID();
        }
    });

    batch.setModelTransform(transform);

    geometryCache->renderLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(HALF_TREE_SCALE, 0.0f, 0.0f), RED, geometryIds[0]);
    geometryCache->renderDashedLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-HALF_TREE_SCALE, 0.0f, 0.0f), DASHED_RED,
                                    DASH_LENGTH, GAP_LENGTH, geometryIds[1]);

    geometryCache->renderLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, HALF_TREE_SCALE, 0.0f), GREEN, geometryIds[2]);
    geometryCache->renderDashedLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -HALF_TREE_SCALE, 0.0f), DASHED_GREEN,
                                    DASH_LENGTH, GAP_LENGTH, geometryIds[3]);

    geometryCache->renderLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, HALF_TREE_SCALE), BLUE, geometryIds[4]);
    geometryCache->renderDashedLine(batch, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -HALF_TREE_SCALE), DASHED_BLUE,
                                    DASH_LENGTH, GAP_LENGTH, geometryIds[5]);

    // X center boundaries
    geometryCache->renderLine(batch, glm::vec3(-HALF_TREE_SCALE, -HALF_TREE_SCALE, 0.0f),
                              glm::vec3(HALF_TREE_SCALE, -HALF_TREE_SCALE, 0.0f), GREY,
                              geometryIds[6]);
    geometryCache->renderLine(batch, glm::vec3(-HALF_TREE_SCALE, -HALF_TREE_SCALE, 0.0f),
                              glm::vec3(-HALF_TREE_SCALE, HALF_TREE_SCALE, 0.0f), GREY,
                              geometryIds[7]);
    geometryCache->renderLine(batch, glm::vec3(-HALF_TREE_SCALE, HALF_TREE_SCALE, 0.0f),
                              glm::vec3(HALF_TREE_SCALE, HALF_TREE_SCALE, 0.0f), GREY,
                              geometryIds[8]);
    geometryCache->renderLine(batch, glm::vec3(HALF_TREE_SCALE, -HALF_TREE_SCALE, 0.0f),
                              glm::vec3(HALF_TREE_SCALE, HALF_TREE_SCALE, 0.0f), GREY,
                              geometryIds[9]);

    // Z center boundaries
    geometryCache->renderLine(batch, glm::vec3(0.0f, -HALF_TREE_SCALE, -HALF_TREE_SCALE),
                              glm::vec3(0.0f, -HALF_TREE_SCALE, HALF_TREE_SCALE), GREY,
                              geometryIds[10]);
    geometryCache->renderLine(batch, glm::vec3(0.0f, -HALF_TREE_SCALE, -HALF_TREE_SCALE),
                              glm::vec3(0.0f, HALF_TREE_SCALE, -HALF_TREE_SCALE), GREY,
                              geometryIds[11]);
    geometryCache->renderLine(batch, glm::vec3(0.0f, HALF_TREE_SCALE, -HALF_TREE_SCALE),
                              glm::vec3(0.0f, HALF_TREE_SCALE, HALF_TREE_SCALE), GREY,
                              geometryIds[12]);
    geometryCache->renderLine(batch, glm::vec3(0.0f, -HALF_TREE_SCALE, HALF_TREE_SCALE),
                              glm::vec3(0.0f, HALF_TREE_SCALE, HALF_TREE_SCALE), GREY,
                              geometryIds[13]);

    // Center boundaries
    geometryCache->renderLine(batch, glm::vec3(-HALF_TREE_SCALE, 0.0f, -HALF_TREE_SCALE),
                              glm::vec3(-HALF_TREE_SCALE, 0.0f, HALF_TREE_SCALE), GREY,
                              geometryIds[14]);
    geometryCache->renderLine(batch, glm::vec3(-HALF_TREE_SCALE, 0.0f, -HALF_TREE_SCALE),
                              glm::vec3(HALF_TREE_SCALE, 0.0f, -HALF_TREE_SCALE), GREY,
                              geometryIds[15]);
    geometryCache->renderLine(batch, glm::vec3(HALF_TREE_SCALE, 0.0f, -HALF_TREE_SCALE),
                              glm::vec3(HALF_TREE_SCALE, 0.0f, HALF_TREE_SCALE), GREY,
                              geometryIds[16]);
    geometryCache->renderLine(batch, glm::vec3(-HALF_TREE_SCALE, 0.0f, HALF_TREE_SCALE),
                              glm::vec3(HALF_TREE_SCALE, 0.0f, HALF_TREE_SCALE), GREY,
                              geometryIds[17]);

    
    geometryCache->renderWireCubeInstance(args, batch, GREY4);

    //  Draw meter markers along the 3 axis to help with measuring things
    const float MARKER_DISTANCE = 1.0f;
    const float MARKER_RADIUS = 0.05f;

    transform = Transform().setScale(MARKER_RADIUS);
    batch.setModelTransform(transform);
    geometryCache->renderSolidSphereInstance(args, batch, RED);

    transform = Transform().setTranslation(glm::vec3(MARKER_DISTANCE, 0.0f, 0.0f)).setScale(MARKER_RADIUS);
    batch.setModelTransform(transform);
    geometryCache->renderSolidSphereInstance(args, batch, RED);

    transform = Transform().setTranslation(glm::vec3(0.0f, MARKER_DISTANCE, 0.0f)).setScale(MARKER_RADIUS);
    batch.setModelTransform(transform);
    geometryCache->renderSolidSphereInstance(args, batch, GREEN);

    transform = Transform().setTranslation(glm::vec3(0.0f, 0.0f, MARKER_DISTANCE)).setScale(MARKER_RADIUS);
    batch.setModelTransform(transform);
    geometryCache->renderSolidSphereInstance(args, batch, BLUE);

    transform = Transform().setTranslation(glm::vec3(MARKER_DISTANCE, 0.0f, MARKER_DISTANCE)).setScale(MARKER_RADIUS);
    batch.setModelTransform(transform);
    geometryCache->renderSolidSphereInstance(args, batch, GREY);
}

//  Do some basic timing tests and report the results
void runTimingTests() {
    //  How long does it take to make a call to get the time?
    const int numTimingTests = 3;
    QElapsedTimer startTime;
    float elapsedNSecs;
    float elapsedUSecs;

    qCDebug(interfaceapp, "numTimingTests: %d", numTimingTests);

    startTime.start();
    elapsedNSecs = (float)startTime.nsecsElapsed();
    qCDebug(interfaceapp, "QElapsedTimer::nsecElapsed() ns: %f", (double)elapsedNSecs / numTimingTests);

    // Test sleep functions for accuracy
    startTime.start();
    for (int i = 0; i < numTimingTests; i++) {
        QThread::msleep(1);
    }
    elapsedNSecs = (float)startTime.nsecsElapsed();
    qCDebug(interfaceapp, "QThread::msleep(1) ms: %f", (double)(elapsedNSecs / NSECS_PER_MSEC / numTimingTests));

    startTime.start();
    for (int i = 0; i < numTimingTests; i++) {
        QThread::sleep(1);
    }
    elapsedNSecs = (float)startTime.nsecsElapsed();
    qCDebug(interfaceapp, "QThread::sleep(1) s: %f", (double)(elapsedNSecs / NSECS_PER_MSEC / MSECS_PER_SECOND / numTimingTests));

    const int numUsecTests = 1000;
    startTime.start();
    for (int i = 0; i < numUsecTests; i++) {
        usleep(1);
    }
    elapsedNSecs = (float)startTime.nsecsElapsed();
    qCDebug(interfaceapp, "usleep(1) (1000x) us: %f", (double)(elapsedNSecs / NSECS_PER_USEC / numUsecTests));

    startTime.start();
    for (int i = 0; i < numUsecTests; i++) {
        usleep(10);
    }
    elapsedNSecs = (float)startTime.nsecsElapsed();
    qCDebug(interfaceapp, "usleep(10) (1000x) us: %f", (double)(elapsedNSecs / NSECS_PER_USEC / numUsecTests));

    startTime.start();
    for (int i = 0; i < numUsecTests; i++) {
        usleep(100);
    }
    elapsedNSecs = (float)startTime.nsecsElapsed();
    qCDebug(interfaceapp, "usleep(100) (1000x) us: %f", (double)(elapsedNSecs / NSECS_PER_USEC / numUsecTests));

    startTime.start();
    for (int i = 0; i < numTimingTests; i++) {
        usleep(1000);
    }
    elapsedNSecs = (float)startTime.nsecsElapsed();
    qCDebug(interfaceapp, "usleep(1000) us: %f", (double)(elapsedNSecs / NSECS_PER_USEC / numTimingTests));

    startTime.start();
    for (int i = 0; i < numTimingTests; i++) {
        usleep(1001);
    }
    elapsedNSecs = (float)startTime.nsecsElapsed();
    qCDebug(interfaceapp, "usleep(1001) us: %f", (double)(elapsedNSecs / NSECS_PER_USEC / numTimingTests));

    startTime.start();
    for (int i = 0; i < numTimingTests; i++) {
        usleep(1500);
    }
    elapsedNSecs = (float)startTime.nsecsElapsed();
    qCDebug(interfaceapp, "usleep(1500) us: %f", (double)(elapsedNSecs / NSECS_PER_USEC / numTimingTests));

    startTime.start();
    usleep(15000);
    elapsedNSecs = (float)startTime.nsecsElapsed();
    qCDebug(interfaceapp, "usleep(15000) (1x) us: %f", (double)(elapsedNSecs / NSECS_PER_USEC));

    const int numTests = 1000000;
    int* iResults = (int*)malloc(sizeof(int) * numTests);
    float fTest = 1.0;
    float* fResults = (float*)malloc(sizeof(float) * numTests);

    // Random number generation
    startTime.start();
    for (int i = 0; i < numTests; i++) {
        iResults[i] = rand();
    }
    elapsedUSecs = (float)startTime.nsecsElapsed() / NSECS_PER_USEC;
    qCDebug(interfaceapp, "rand() stored in array usecs: %f, first result:%d",
            (double)(elapsedUSecs / numTests), iResults[0]);

    // Random number generation using randFloat()
    startTime.start();
    for (int i = 0; i < numTests; i++) {
        fResults[i] = randFloat();
    }
    elapsedUSecs = (float)startTime.nsecsElapsed() / NSECS_PER_USEC;
    qCDebug(interfaceapp, "randFloat() stored in array usecs: %f, first result: %f",
            (double)(elapsedUSecs / numTests), (double)(fResults[0]));

    free(iResults);
    free(fResults);

    //  PowF function
    fTest = 1145323.2342f;
    startTime.start();
    for (int i = 0; i < numTests; i++) {
        fTest = powf(fTest, 0.5f);
    }
    elapsedUSecs = (float)startTime.nsecsElapsed() / NSECS_PER_USEC;
    qCDebug(interfaceapp, "powf(f, 0.5) usecs: %f", (double)(elapsedUSecs / (float) numTests));

    //  Vector Math
    float distance;
    glm::vec3 pointA(randVector()), pointB(randVector());
    startTime.start();
    for (int i = 0; i < numTests; i++) {
        //glm::vec3 temp = pointA - pointB;
        //float distanceSquared = glm::dot(temp, temp);
        distance = glm::distance(pointA, pointB);
    }
    elapsedUSecs = (float)startTime.nsecsElapsed() / NSECS_PER_USEC;
    qCDebug(interfaceapp, "vector math usecs: %f [%f usecs total for %d tests], last result:%f",
            (double)(elapsedUSecs / (float) numTests), (double)elapsedUSecs, numTests, (double)distance);

    //  Vec3 test
    glm::vec3 vecA(randVector()), vecB(randVector());
    float result;

    startTime.start();
    for (int i = 0; i < numTests; i++) {
        glm::vec3 temp = vecA-vecB;
        result = glm::dot(temp,temp);
    }
    elapsedUSecs = (float)startTime.nsecsElapsed() / NSECS_PER_USEC;
    qCDebug(interfaceapp, "vec3 assign and dot() usecs: %f, last result:%f",
            (double)(elapsedUSecs / numTests), (double)result);


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
        elapsedUSecs = (float)startTime.nsecsElapsed() / NSECS_PER_USEC;
        qCDebug(interfaceapp) << "ByteCountCoded<quint64> usecs: " << elapsedUSecs
                                << "per test:" << (double) (elapsedUSecs / tests)
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


