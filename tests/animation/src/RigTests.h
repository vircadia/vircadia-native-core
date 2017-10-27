//
//  RigTests.h
//  tests/rig/src
//
//  Created by Howard Stearns on 6/16/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RigTests_h
#define hifi_RigTests_h

#include <QtTest/QtTest>
#include <Rig.h>

//#include "../QTestExtensions.h"


// The QTest terminology is not consistent with itself or with industry:
// The whole directory, and the rig-tests target, doesn't seem to be a QTest concept, an corresponds roughly to a toplevel suite of suites.
// The directory can contain any number of classes like this one. (Don't forget to wipe your build dir, and rerun cmake when you add one.):
//    QTest doc (http://doc.qt.io/qt-5/qtest-overview.html) calls this a "test case".
//    The output of QTest's 'ctest' runner calls this a "test" when run in the whole directory (e.g., when reporting success/failure counts).
// The test case (like this class) can contain any number of test slots:
//    QTest doc calls these "test functions"
//    When you run a single test case executable (e.g., "rig-RigTests"), the (unlabeled) count includes these test functions and also the before method, which is auto generated as initTestCase.

// To build and run via make:
// make help | grep tests # shows all test targets, including all-tests and rig-tests.
// make all-tests # will compile and then die as soon as any test case dies, even if its not in your directory
// make rig-tests # will compile and run `ctest .` in the tests/rig directory, running all the test cases found there.
//   Alas, only summary output is shown on stdout. The real results, including any stdout that your code does, is in tests/rig/Testing/Temporary/LastTest.log, or...
// tests/rig/rig-RigTests (or the executable corresponding to any test case you define here) will run just that case and give output directly.
//
// To build and run via Xcode:
// On some machines, xcode can't find cmake on the path it uses. I did, effectively: sudo ln -s `which cmake` /usr/bin
// Note the above make instructions.
// all-tests, rig-tests, and rig-RigTests are all targets:
//   The first two of these show no output at all, but if there's a failure you can see it by clicking on the red failure in the "issue navigator" (or by externally viewing the .log above).
//   The last (or any other individual test case executable) does show output in the Xcode output display.

class RigTests : public QObject {
    Q_OBJECT

 private slots:
    void initTestCase();
    void initialPoseArmsDown();

 private:
    Rig _rig;
};

#endif // hifi_RigTests_h
