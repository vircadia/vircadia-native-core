//
//  main.cpp
//  tests/networking/src
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SequenceNumberStatsTests.h"
#include <stdio.h>

int main(int argc, char** argv) {
    SequenceNumberStatsTests::runAllTests();
    printf("tests passed! press enter to exit");
    getchar();
    return 0;
}
