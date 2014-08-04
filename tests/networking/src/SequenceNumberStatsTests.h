//
//  AudioRingBufferTests.h
//  tests/networking/src
//
//  Created by Yixin Wang on 6/24/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SequenceNumberStatsTests_h
#define hifi_SequenceNumberStatsTests_h

#include "SequenceNumberStatsTests.h"
#include "SequenceNumberStats.h"

namespace SequenceNumberStatsTests {

    void runAllTests();

    void rolloverTest();
    void earlyLateTest();
    void duplicateTest();
    void pruneTest();
    void recursiveTest();
};

#endif // hifi_SequenceNumberStatsTests_h
