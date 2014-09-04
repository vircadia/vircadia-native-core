//
//  main.cpp
//  tests/octree/src
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AABoxCubeTests.h"
#include "ModelTests.h" // needs to be EntityTests.h soon
#include "OctreeTests.h"
#include "SharedUtil.h"

int main(int argc, const char* argv[]) {
    const char* VERBOSE = "--verbose";
    bool verbose = cmdOptionExists(argc, argv, VERBOSE);
    qDebug() << "OctreeTests::runAllTests()";
    //OctreeTests::runAllTests(verbose);
    //AABoxCubeTests::runAllTests(verbose);
    EntityTests::runAllTests(verbose);
    return 0;
}
