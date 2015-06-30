//
//  AABoxCubeTests.h
//  tests/octree/src
//
//  Created by Brad Hefta-Gaub on 06/04/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AABoxCubeTests_h
#define hifi_AABoxCubeTests_h

#include <QtTest/QtTest>

class AABoxCubeTests : public QObject {
    Q_OBJECT
    
private slots:
    void raycastOutHitsXMinFace ();
    void raycastOutHitsXMaxFace ();
    void raycastInHitsXMinFace ();
    
    // TODO: Add more unit tests!
    // (do we need this? Currently no tests for no-intersection or non-orthogonal rays)
};

#endif // hifi_AABoxCubeTests_h
