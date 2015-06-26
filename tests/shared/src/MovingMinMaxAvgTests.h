//
//  MovingMinMaxAvgTests.h
//  tests/shared/src
//
//  Created by Yixin Wang on 7/8/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MovingMinMaxAvgTests_h
#define hifi_MovingMinMaxAvgTests_h

#include <QtTest/QtTest>

inline float getErrorDifference (float a, float b) {
    return fabsf(a - b);
}

#include "../QTestExtensions.h"

#include "MovingMinMaxAvg.h"
#include "SharedUtil.h"

class MovingMinMaxAvgTests : public QObject {
    
private slots:
    void testQuint64 ();
    void testInt     ();
    void testFloat   ();
    
private:
    quint64 randQuint64();
};

#endif // hifi_MovingMinMaxAvgTests_h
