//
//  AABoxTests.h
//  tests/shared/src
//
//  Created by Andrew Meadows on 2016.02.19
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AABoxTests_h
#define hifi_AABoxTests_h

#include <QtTest/QtTest>
#include <glm/glm.hpp>

#include <AABox.h>

class AABoxTests : public QObject {
    Q_OBJECT
private slots:
    void testCtorsAndSetters();
    void testContainsPoint();
    void testTouchesSphere();
    void testScale();
    void testFindSpherePenetration();
};

#endif // hifi_AABoxTests_h
