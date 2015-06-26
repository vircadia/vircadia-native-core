//
//  AngularConstraintTests.h
//  tests/physics/src
//
//  Created by Andrew Meadows on 2014.05.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AngularConstraintTests_h
#define hifi_AngularConstraintTests_h

#include <QtTest/QtTest>

class AngularConstraintTests : public QObject {
    Q_OBJECT
private slots:
    void testHingeConstraint();
    void testConeRollerConstraint();
};

// Use QCOMPARE_WITH_ABS_ERROR and define it for glm::quat
#include <glm/glm.hpp>
float getErrorDifference (const glm::quat & a, const glm::quat & b);
QTextStream & operator << (QTextStream & stream, const glm::quat & q);
#include "../QTestExtensions.h"

#endif // hifi_AngularConstraintTests_h
