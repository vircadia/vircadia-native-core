//
// DualQuaternionTests.h
// tests/shared/src
//
// Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DualQuaternionTests_h
#define hifi_DualQuaternionTests_h

#include <QtTest/QtTest>

class DualQuaternionTests : public QObject {
    Q_OBJECT
private slots:
    void ctor();
    void mult();
    void xform();
    void trans();
};

#endif // hifi_DualQuaternionTests_h
