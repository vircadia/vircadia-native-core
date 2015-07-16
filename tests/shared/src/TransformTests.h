//
// TransformTests.h
// tests/shared/src
//
// Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TransformTests_h
#define hifi_TransformTests_h

#include <QtTest/QtTest>
#include <glm/glm.hpp>
#include <algorithm>

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

#include <../QTestExtensions.h>

class TransformTests : public QObject {
    Q_OBJECT
private slots:
    void getMatrix();
    void getInverseMatrix();
};

#endif // hifi_TransformTests_h
