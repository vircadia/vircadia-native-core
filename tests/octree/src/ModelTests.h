//
//  EntityTests.h
//  tests/octree/src
//
//  Created by Brad Hefta-Gaub on 06/04/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityTests_h
#define hifi_EntityTests_h

#include <QtTest/QtTest>

//
// TODO: These are waaay out of date with the current codebase, and should be reimplemented at some point.
//

class EntityTests : public QObject {
    Q_OBJECT

private slots:
    void testsNotImplemented () {
        qDebug() << "fixme: ModelTests are currently broken and need to be reimplemented";
    }
//    void entityTreeTests(bool verbose = false);
//    void runAllTests(bool verbose = false);
};

#endif // hifi_EntityTests_h
