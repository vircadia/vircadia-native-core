//
//  RotationConstraintTests.h
//  tests/rig/src
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RotationConstraintTests_h
#define hifi_RotationConstraintTests_h

#include <QtTest/QtTest>

class RotationConstraintTests : public QObject {
    Q_OBJECT

private slots:
    void testElbowConstraint();
    void testSwingTwistConstraint();
    void testDynamicSwingLimitFunction();
    void testDynamicSwing();
    void testDynamicTwist();
};

#endif // hifi_RotationConstraintTests_h
