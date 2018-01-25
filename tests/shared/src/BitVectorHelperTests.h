//
// BitVectorHelperTests.h
// tests/shared/src
//
// Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BitVectorHelperTests_h
#define hifi_BitVectorHelperTests_h

#include <QtTest/QtTest>

class BitVectorHelperTests : public QObject {
    Q_OBJECT
private slots:
    void sizeTest();
    void readWriteTest();
};

#endif // hifi_BitVectorHelperTests_h
