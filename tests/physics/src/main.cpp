//
//  main.cpp
//  tests/physics/src
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtTest/QtTest>
//#include <QtConcurrent>

//#include "ShapeColliderTests.h"
#include "ShapeInfoTests.h"
#include "ShapeManagerTests.h"
//#include "BulletUtilTests.h"
#include "MeshMassPropertiesTests.h"

//int main(int argc, char** argv) {
//    ShapeColliderTests::runAllTests();
//    ShapeInfoTests::runAllTests();
//    ShapeManagerTests::runAllTests();
//    BulletUtilTests::runAllTests();
//    MeshMassPropertiesTests::runAllTests();
//    return 0;
//}

//QTEST_MAIN(BulletUtilTests)

// Custom main function to run multiple unit tests. Just copy + paste this into future tests.
//#define RUN_TEST(Test) { Test test; runTest(test); }
#define RUN_TEST(Test) { runTest(new Test()); }
int main (int argc, const char ** argv) {
    QStringList args;
    for (int i = 0; i < argc; ++i)
        args.append(QString { argv[i] });
    int status = 0;
    
    auto runTest = [&status, args] (QObject * test) {
        status |= QTest::qExec(test, args);
    };
    
    // Run test classes
//    RUN_TEST(ShapeColliderTests)
    RUN_TEST(ShapeInfoTests)
    RUN_TEST(ShapeManagerTests)
//    RUN_TEST(BulletUtilTests)
    RUN_TEST(MeshMassPropertiesTests)

    return status;
}