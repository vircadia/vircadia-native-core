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

inline float fuzzyCompare (const glm::vec3 & a, const glm::vec3 & b) {
    return glm::distance(a, b);
}
inline QTextStream & operator << (QTextStream & stream, const glm::vec3 & v) {
    return stream << "glm::vec3 { " << v.x << ", " << v.y << ", " << v.z << " }";
}
inline btScalar fuzzyCompare (const btScalar & a, const btScalar & b) {
    return fabs(a - b);
}
// uh... how do we compare matrices?
// Guess we'll just do this element-wise for the time being
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




// These hook into this (and must be defined first...)
#include "../QTestExtensions.hpp"


#endif // hifi_MeshMassPropertiesTests_h
