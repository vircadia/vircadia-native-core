//
//  AnimClipTests.cpp
//  tests/rig/src
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimClipTests.h"
#include "AnimClip.h"
#include "AnimationLogging.h"

#include <../QTestExtensions.h>

QTEST_MAIN(AnimClipTests)

const float EPSILON = 0.001f;

void AnimClipTests::testAccessors() {
    std::string url = "foo";
    float startFrame = 2.0f;
    float endFrame = 20.0f;
    float timeScale = 1.1f;
    float loopFlag = true;

    AnimClip clip(url, startFrame, endFrame, timeScale, loopFlag);
    QVERIFY(clip.getURL() == url);
    QVERIFY(clip.getStartFrame() == startFrame);
    QVERIFY(clip.getEndFrame() == endFrame);
    QVERIFY(clip.getTimeScale() == timeScale);
    QVERIFY(clip.getLoopFlag() == loopFlag);

    std::string url2 = "bar";
    float startFrame2 = 22.0f;
    float endFrame2 = 100.0f;
    float timeScale2 = 1.2f;
    float loopFlag2 = false;

    clip.setURL(url2);
    clip.setStartFrame(startFrame2);
    clip.setEndFrame(endFrame2);
    clip.setTimeScale(timeScale2);
    clip.setLoopFlag(loopFlag2);

    QVERIFY(clip.getURL() == url2);
    QVERIFY(clip.getStartFrame() == startFrame2);
    QVERIFY(clip.getEndFrame() == endFrame2);
    QVERIFY(clip.getTimeScale() == timeScale2);
    QVERIFY(clip.getLoopFlag() == loopFlag2);
}

static float secsToFrames(float secs) {
    const float FRAMES_PER_SECOND = 30.0f;
    return secs * FRAMES_PER_SECOND;
}

static float framesToSec(float secs) {
    const float FRAMES_PER_SECOND = 30.0f;
    return secs / FRAMES_PER_SECOND;
}

void AnimClipTests::testEvaulate() {
    std::string url = "foo";
    float startFrame = 2.0f;
    float endFrame = 22.0f;
    float timeScale = 1.0f;
    float loopFlag = true;

    AnimClip clip(url, startFrame, endFrame, timeScale, loopFlag);

    clip.evaluate(framesToSec(10.0f));
    QCOMPARE_WITH_ABS_ERROR(clip._frame, 12.0f, EPSILON);

    // does it loop?
    clip.evaluate(framesToSec(11.0f));
    QCOMPARE_WITH_ABS_ERROR(clip._frame, 3.0f, EPSILON);

    // does it pause at end?
    clip.setLoopFlag(false);
    clip.evaluate(framesToSec(20.0f));
    QCOMPARE_WITH_ABS_ERROR(clip._frame, 22.0f, EPSILON);
}

