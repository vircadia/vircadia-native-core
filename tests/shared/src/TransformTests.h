//
// TransformTests.h
// tests/shared/src
//
// Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TransformTests_h
#define hifi_TransformTests_h

#include <QtTest/QtTest>

class TransformTests : public QObject {
    Q_OBJECT
private slots:
    void getMatrix();
    void getInverseMatrix();
};

#endif // hifi_TransformTests_h
