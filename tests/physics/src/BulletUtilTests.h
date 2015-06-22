//
//  BulletUtilTests.h
//  tests/physics/src
//
//  Created by Andrew Meadows on 2014.11.02
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BulletUtilTests_h
#define hifi_BulletUtilTests_h

#include <QtTest/QtTest>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class BulletUtilTests : public QObject {
    Q_OBJECT

private slots:
    void fromBulletToGLM();
    void fromGLMToBullet();
    void fooTest ();
};

// Define comparison + printing functions for the data types we need

inline float fuzzyCompare (const glm::vec3 & a, const glm::vec3 & b) {
    return glm::distance(a, b);
}
inline QTextStream & operator << (QTextStream & stream, const glm::vec3 & v) {
    return stream << "glm::vec3 { " << v.x << ", " << v.y << ", " << v.z << " }";
}

// These hook into this (and must be defined first...)
#include "../QTestExtensions.hpp"

#endif // hifi_BulletUtilTests_h
