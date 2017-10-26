//
//  ShapeInfoTests.h
//  tests/physics/src
//
//  Created by Andrew Meadows on 2014.11.02
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ShapeInfoTests_h
#define hifi_ShapeInfoTests_h

#include <QtTest/QtTest>

//// Add additional qtest functionality (the include order is important!)
//#include "BulletTestUtils.h"
//#include "../QTestExtensions.h"

class ShapeInfoTests : public QObject {
    Q_OBJECT
private slots:
    void testHashFunctions();
    void testBoxShape();
    void testSphereShape();
    void testCylinderShape();
    void testCapsuleShape();
};

#endif // hifi_ShapeInfoTests_h
