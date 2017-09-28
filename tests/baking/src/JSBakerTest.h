//
//  JSBakerTest.h
//  tests/networking/src
//
//  Created by Utkarsh Gautam on 9/26/17.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_JSBakerTest_h
#define hifi_JSBakerTest_h

#include <QtTest/QtTest>
#include <JSBaker.h>

class JSBakerTest : public QObject {
    Q_OBJECT

private slots:
    void setTestCases();
    void testJSBaking();

private:
    std::vector<std::pair<QByteArray, QByteArray>> _testCases;
};

#endif
