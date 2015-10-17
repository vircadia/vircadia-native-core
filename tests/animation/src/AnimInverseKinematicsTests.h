//
// AnimInverseKinematicsTests.h
//
// Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimInverseKinematicsTests_h
#define hifi_AnimInverseKinematicsTests_h

#include <QtTest/QtTest>
#include <glm/glm.hpp>

inline float getErrorDifference(float a, float b) {
    return fabs(a - b);
}

class AnimInverseKinematicsTests : public QObject {
    Q_OBJECT
private slots:
    void testSingleChain();
    void testBar();
};

#endif // hifi_AnimInverseKinematicsTests_h
