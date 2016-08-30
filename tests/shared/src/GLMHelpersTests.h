//
//  GLMHelpersTests.h
//  tests/shared/src
//
//  Created by Anthony thibault on 2015.12.29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GLMHelpersTests_h
#define hifi_GLMHelpersTests_h

#include <QtTest/QtTest>
#include <GLMHelpers.h>

class GLMHelpersTests : public QObject {
    Q_OBJECT
private slots:
    void testEulerDecomposition();
    void testSixByteOrientationCompression();
};

float getErrorDifference(const float& a, const float& b);
float getErrorDifference(const glm::vec3& a, const glm::vec3& b);

#endif // hifi_GLMHelpersTest_h
