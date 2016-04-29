//
//  GeometryUtilTests.h
//  tests/shared/src
//
//  Created by Andrew Meadows on 2014.05.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GeometryUtilTests_h
#define hifi_GeometryUtilTests_h

#include <QtTest/QtTest>
#include <glm/glm.hpp>

class GeometryUtilTests : public QObject {
    Q_OBJECT
private slots:
    void testConeSphereAngle();
    void testLocalRayRectangleIntersection();
    void testWorldRayRectangleIntersection();
    void testTwistSwingDecomposition();
    void testSphereCapsulePenetration();
};

float getErrorDifference(const float& a, const float& b);
float getErrorDifference(const glm::vec3& a, const glm::vec3& b);

#endif // hifi_GeometryUtilTests_h
