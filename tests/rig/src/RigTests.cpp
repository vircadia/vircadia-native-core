//
//  RigTests.cpp
//  tests/rig/src
//
//  Created by Howard Stearns on 6/16/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include "RigTests.h"

QTEST_MAIN(RigTests)

void RigTests::initTestCase() {
    _rig = new Rig();
}

void RigTests::dummyPassTest() {
    bool x = true;
    std::cout << "dummyPassTest x=" << x << std::endl;
    QCOMPARE(x, true);
}

void RigTests::dummyFailTest() {
    bool x = false;
    std::cout << "dummyFailTest x=" << x << std::endl;
    QCOMPARE(x, true);
}
