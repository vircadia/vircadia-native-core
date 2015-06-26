//
//  BulletUtilTests.h
//  tests/physics/src
//
//  Created by Andrew Meadows on 2014.11.02
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BulletUtilTests_h
#define hifi_BulletUtilTests_h

#include <QtTest/QtTest>
// Add additional qtest functionality (the include order is important!)
#include "GlmTestUtils.h"
#include "../QTestExtensions.h"

class BulletUtilTests : public QObject {
    Q_OBJECT

private slots:
    void fromBulletToGLM();
    void fromGLMToBullet();
//    void fooTest ();
};

#endif // hifi_BulletUtilTests_h
