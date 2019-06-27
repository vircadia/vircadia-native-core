//
//  BlendshapePackingTests.h
//  tests/shared/src
//
//  Created by Ken Cooke on 6/24/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BlendshapePackingTests_h
#define hifi_BlendshapePackingTests_h

#include <QtTest/QtTest>

class BlendshapePackingTests : public QObject {
    Q_OBJECT
private slots:
    void testAVX2();
};

#endif // hifi_BlendshapePackingTests_h
