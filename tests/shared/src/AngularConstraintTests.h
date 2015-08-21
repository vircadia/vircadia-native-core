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

#include <glm/glm.hpp>
#include <QtTest/QtTest>

class AngularConstraintTests : public QObject {
    Q_OBJECT
private slots:
    void testHingeConstraint();
    void testConeRollerConstraint();
};

float getErrorDifference(const glm::quat& a, const glm::quat& b);

#endif // hifi_AngularConstraintTests_h
