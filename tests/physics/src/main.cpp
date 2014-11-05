//
//  main.cpp
//  tests/physics/src
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

//#include "ShapeColliderTests.h"
//#include "VerletShapeTests.h"
#include "ShapeInfoTests.h"
#include "ShapeManagerTests.h"
#include "BulletUtilTests.h"

int main(int argc, char** argv) {
    //ShapeColliderTests::runAllTests();
    //VerletShapeTests::runAllTests();
    ShapeInfoTests::runAllTests();
    ShapeManagerTests::runAllTests();
    BulletUtilTests::runAllTests();
    return 0;
}
