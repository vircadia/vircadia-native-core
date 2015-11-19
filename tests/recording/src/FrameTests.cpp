//
//  Created by Bradley Austin Davis 2015/11/05
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FrameTests.h"
#include "Constants.h"

#if 0

#include "../QTestExtensions.h"

#include <recording/Frame.h>

QTEST_MAIN(FrameTests)

void FrameTests::registerFrameTypeTest() {
    auto result = recording::Frame::registerFrameType(TEST_NAME);
    QCOMPARE(result, (recording::FrameType)1);
    auto forwardMap = recording::Frame::getFrameTypes();
    QCOMPARE(forwardMap.count(TEST_NAME), 1);
    QCOMPARE(forwardMap[TEST_NAME], result);
    QCOMPARE(forwardMap[HEADER_NAME], recording::Frame::TYPE_HEADER);
    auto backMap = recording::Frame::getFrameTypeNames();
    QCOMPARE(backMap.count(result), 1);
    QCOMPARE(backMap[result], TEST_NAME);
    QCOMPARE(backMap[recording::Frame::TYPE_HEADER], HEADER_NAME);
}

#endif
