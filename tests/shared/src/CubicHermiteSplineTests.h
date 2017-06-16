//
// CubicHermiteSplineTests.h
// tests/shared/src
//
// Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CubicHermiteSplineTests_h
#define hifi_CubicHermiteSplineTests_h

#include <QtTest/QtTest>

class CubicHermiteSplineTests : public QObject {
    Q_OBJECT
private slots:
    void testCubicHermiteSplineFunctor();
    void testCubicHermiteSplineFunctorWithArcLength();
};

#endif // hifi_TransformTests_h
