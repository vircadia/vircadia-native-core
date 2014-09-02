//
//  main.cpp
//  tests/physics/src
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AngularConstraintTests.h"
#include "MovingPercentileTests.h"
#include "MovingMinMaxAvgTests.h"

int main(int argc, char** argv) {
    MovingMinMaxAvgTests::runAllTests();
    MovingPercentileTests::runAllTests();
    AngularConstraintTests::runAllTests();
    printf("tests complete, press enter to exit\n");
    getchar();
    return 0;
}
