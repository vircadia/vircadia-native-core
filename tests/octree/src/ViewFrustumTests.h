//
//  ViewFrustumTests.h
//  tests/octree/src
//
//  Created by Andrew Meadows on 2016.02.19
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ViewFruxtumTests_h
#define hifi_ViewFruxtumTests_h

#include <QtTest/QtTest>

class ViewFrustumTests : public QObject {
    Q_OBJECT

private slots:
    void testInit();
    void testCubeFrustumIntersection();
    void testCubeKeyholeIntersection();
    void testPointIntersectsFrustum();
    void testSphereIntersectsFrustum();
    void testBoxIntersectsFrustum();
    void testSphereIntersectsKeyhole();
    void testCubeIntersectsKeyhole();
    void testBoxIntersectsKeyhole();
};

#endif // hifi_ViewFruxtumTests_h
