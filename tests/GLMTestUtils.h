//
//  GLMTestUtils.h
//  tests/physics/src
//
//  Created by Seiji Emery on 6/22/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GLMTestUtils_h
#define hifi_GLMTestUtils_h

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <QTextStream>

// Implements functionality in QTestExtensions.h for glm types

// Computes the error value between two quaternions (using glm::dot)
float getErrorDifference(const glm::quat& a, const glm::quat& b) {
    return fabsf(glm::dot(a, b)) - 1.0f;
}       

inline float getErrorDifference(const glm::vec3& a, const glm::vec3& b) {
    return glm::distance(a, b);
}

inline float getErrorDifference(const glm::mat4& a, const glm::mat4& b) {
    float maxDiff = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float diff = fabs(a[i][j] - b[i][j]);
            maxDiff = std::max(diff, maxDiff);
        }
    }
    return maxDiff;
}

inline QTextStream& operator<<(QTextStream& stream, const glm::vec3& v) {
    return stream << "glm::vec3 { " << v.x << ", " << v.y << ", " << v.z << " }";
}

QTextStream& operator<<(QTextStream& stream, const glm::quat& q) {
            return stream << "glm::quat { " << q.x << ", " << q.y << ", " << q.z << ", " << q.w << " }";
} 

inline QTextStream& operator<< (QTextStream& stream, const glm::mat4& matrix) {
    stream << "[\n\t\t";
    stream.setFieldWidth(15);
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            stream << matrix[c][r];
        }
        stream << "\n\t\t";
    }
    stream.setFieldWidth(0);
    stream << "]\n\t";   // hacky as hell, but this should work...
    return stream;
}

#define QCOMPARE_QUATS(rotationA, rotationB, angle) \
    QVERIFY(fabsf(1.0f - fabsf(glm::dot(rotationA, rotationB))) < 2.0f * sinf(angle))

#endif
