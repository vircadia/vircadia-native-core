//
//  main.cpp
//  tests/octree/src
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelTests.h"
#include "OctreeTests.h"
#include "AABoxCubeTests.h"

int main(int argc, char** argv) {
    OctreeTests::runAllTests();
    AABoxCubeTests::runAllTests();
    ModelTests::runAllTests(true);
    return 0;
}
