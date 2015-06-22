//
//  MeshMassPropertiesTests.h
//  tests/physics/src
//
//  Created by Virendra Singh on 2015.03.02
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MeshMassPropertiesTests_h
#define hifi_MeshMassPropertiesTests_h

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <BulletUtil.h>

#include <QtTest/QtTest>
#include <QtGlobal>

class MeshMassPropertiesTests : public QObject {
    Q_OBJECT
    
private slots:
    void testParallelAxisTheorem();
    void testTetrahedron();
    void testOpenTetrahedonMesh();
    void testClosedTetrahedronMesh();
    void testBoxAsMesh();
};

// Define comparison + printing functions for the data types we need

inline float fuzzyCompare(const glm::vec3 & a, const glm::vec3 & b) {
    return glm::distance(a, b);
}
inline QTextStream & operator << (QTextStream & stream, const glm::vec3 & v) {
    return stream << "glm::vec3 { " << v.x << ", " << v.y << ", " << v.z << " }";
}
inline btScalar fuzzyCompare(const btScalar & a, const btScalar & b) {
    return fabs(a - b);
}

// Matrices are compared element-wise -- if the error value for any element > epsilon, then fail
inline btScalar fuzzyCompare (const btMatrix3x3 & a, const btMatrix3x3 & b) {
    btScalar totalDiff = 0;
    btScalar maxDiff   = 0;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            btScalar diff = fabs(a[i][j] - b[i][j]);
            totalDiff += diff;
            maxDiff    = qMax(diff, maxDiff);
        }
    }
//    return totalDiff;
    return maxDiff;
}

inline QTextStream & operator << (QTextStream & stream, const btMatrix3x3 & matrix) {
    stream << "[\n\t\t";
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            stream << "    " << matrix[i][j];
        }
        stream << "\n\t\t";
    }
    stream << "]\n\t";   // hacky as hell, but this should work...
    return stream;
}

inline btScalar fuzzyCompare(const btVector3 & a, const btVector3 & b)
{
    return (a - b).length();
}
inline QTextStream & operator << (QTextStream & stream, const btVector3 & v) {
    return stream << "btVector3 { " << v.x() << ", " << v.y() << ", " << v.z() << " }";
}

// Produces a relative error test for btMatrix3x3 usable with QCOMPARE_WITH_LAMBDA
inline auto errorTest (const btMatrix3x3 & actual, const btMatrix3x3 & expected, const btScalar acceptableRelativeError)
-> std::function<bool ()>
{
    return [&actual, &expected, acceptableRelativeError] () {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                auto err = (actual[i][j] - expected[i][j]) / expected[i][j];
                if (fabsf(err) > acceptableRelativeError)
                    return false;
            }
        }
        return true;
    };
}

#define QCOMPARE_WITH_RELATIVE_ERROR(actual, expected, relativeError) \
    QCOMPARE_WITH_LAMBDA(actual, expected, errorTest(actual, expected, relativeError))


// These hook into this (and must be defined first...)
#include "../QTestExtensions.hpp"


#endif // hifi_MeshMassPropertiesTests_h
