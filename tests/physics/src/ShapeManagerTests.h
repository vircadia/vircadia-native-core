//
//  ShapeManagerTests.h
//  tests/physics/src
//
//  Created by Andrew Meadows on 2014.10.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ShapeManagerTests_h
#define hifi_ShapeManagerTests_h

#include <QtTest/QtTest>

class ShapeManagerTests : public QObject {
    Q_OBJECT

private slots:
    void testShapeAccounting();
    void addManyShapes();
    void addBoxShape();
    void addSphereShape();
    void addCylinderShape();
    void addCapsuleShape();
    void addCompoundShape();
};

#endif // hifi_ShapeManagerTests_h
