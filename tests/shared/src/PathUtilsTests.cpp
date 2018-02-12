//
//  Created by Bradley Austin Davis on 2017/11/08
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "PathUtilsTests.h"

#include <QtTest/QtTest>

#include <PathUtils.h>

QTEST_MAIN(PathUtilsTests)

void PathUtilsTests::testPathUtils() {
    QString result = PathUtils::qmlBaseUrl();
    QVERIFY(result.startsWith("qrc:///"));
    QVERIFY(result.endsWith("/"));
}

