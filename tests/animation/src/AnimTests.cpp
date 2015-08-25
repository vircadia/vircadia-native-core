//
//  AnimTests.cpp
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AnimTests.h"
#include "AnimNodeLoader.h"
#include "AnimClip.h"
#include "AnimBlendLinear.h"
#include "AnimationLogging.h"
#include "AnimVariant.h"

#include <../QTestExtensions.h>

QTEST_MAIN(AnimTests)

const float EPSILON = 0.001f;

void AnimTests::initTestCase() {
    auto animationCache = DependencyManager::set<AnimationCache>();
    auto resourceCacheSharedItems = DependencyManager::set<ResourceCacheSharedItems>();
}

void AnimTests::cleanupTestCase() {
    DependencyManager::destroy<AnimationCache>();
}

void AnimTests::testAccessors() {
    std::string id = "my anim clip";
    std::string url = "https://hifi-public.s3.amazonaws.com/ozan/support/FightClubBotTest1/Animations/standard_idle.fbx";
    float startFrame = 2.0f;
    float endFrame = 20.0f;
    float timeScale = 1.1f;
    bool loopFlag = true;

    AnimClip clip(id, url, startFrame, endFrame, timeScale, loopFlag);

    QVERIFY(clip.getID() == id);
    QVERIFY(clip.getType() == AnimNode::ClipType);

    QVERIFY(clip.getURL() == url);
    QVERIFY(clip.getStartFrame() == startFrame);
    QVERIFY(clip.getEndFrame() == endFrame);
    QVERIFY(clip.getTimeScale() == timeScale);
    QVERIFY(clip.getLoopFlag() == loopFlag);

    std::string url2 = "https://hifi-public.s3.amazonaws.com/ozan/support/FightClubBotTest1/Animations/standard_walk.fbx";
    float startFrame2 = 22.0f;
    float endFrame2 = 100.0f;
    float timeScale2 = 1.2f;
    bool loopFlag2 = false;

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

static float framesToSec(float secs) {
    const float FRAMES_PER_SECOND = 30.0f;
    return secs / FRAMES_PER_SECOND;
}

void AnimTests::testEvaulate() {
    std::string id = "my clip node";
    std::string url = "https://hifi-public.s3.amazonaws.com/ozan/support/FightClubBotTest1/Animations/standard_idle.fbx";
    float startFrame = 2.0f;
    float endFrame = 22.0f;
    float timeScale = 1.0f;
    float loopFlag = true;

    auto vars = AnimVariantMap();

    AnimClip clip(id, url, startFrame, endFrame, timeScale, loopFlag);

    clip.evaluate(vars, framesToSec(10.0f));
    QCOMPARE_WITH_ABS_ERROR(clip._frame, 12.0f, EPSILON);

    // does it loop?
    clip.evaluate(vars, framesToSec(11.0f));
    QCOMPARE_WITH_ABS_ERROR(clip._frame, 3.0f, EPSILON);

    // does it pause at end?
    clip.setLoopFlag(false);
    clip.evaluate(vars, framesToSec(20.0f));
    QCOMPARE_WITH_ABS_ERROR(clip._frame, 22.0f, EPSILON);
}

void AnimTests::testLoader() {
    auto url = QUrl("https://gist.githubusercontent.com/hyperlogic/857129fe04567cbe670f/raw/8ba57a8f0a76f88b39a11f77f8d9df04af9cec95/test.json");
    AnimNodeLoader loader(url);

    const int timeout = 1000;
    QEventLoop loop;
    QTimer timer;
    timer.setInterval(timeout);
    timer.setSingleShot(true);

    AnimNode::Pointer node = nullptr;
    connect(&loader, &AnimNodeLoader::success, [&](AnimNode::Pointer nodeIn) { node = nodeIn; });

    loop.connect(&loader, SIGNAL(success(AnimNode::Pointer)), SLOT(quit()));
    loop.connect(&loader, SIGNAL(error(int, QString)), SLOT(quit()));
    loop.connect(&timer, SIGNAL(timeout()), SLOT(quit()));
    timer.start();
    loop.exec();

    QVERIFY((bool)node);

    QVERIFY(node->getID() == "blend");
    QVERIFY(node->getType() == AnimNode::BlendLinearType);

    QVERIFY((bool)node);
    QVERIFY(node->getID() == "blend");
    QVERIFY(node->getType() == AnimNode::BlendLinearType);

    auto blend = std::static_pointer_cast<AnimBlendLinear>(node);
    QVERIFY(blend->getAlpha() == 0.5f);

    QVERIFY(node->getChildCount() == 3);

    std::shared_ptr<AnimNode> nodes[3] = { node->getChild(0), node->getChild(1), node->getChild(2) };

    QVERIFY(nodes[0]->getID() == "test01");
    QVERIFY(nodes[0]->getChildCount() == 0);
    QVERIFY(nodes[1]->getID() == "test02");
    QVERIFY(nodes[1]->getChildCount() == 0);
    QVERIFY(nodes[2]->getID() == "test03");
    QVERIFY(nodes[2]->getChildCount() == 0);

    auto test01 = std::static_pointer_cast<AnimClip>(nodes[0]);
    QVERIFY(test01->getURL() == "test01.fbx");
    QVERIFY(test01->getStartFrame() == 1.0f);
    QVERIFY(test01->getEndFrame() == 20.0f);
    QVERIFY(test01->getTimeScale() == 1.0f);
    QVERIFY(test01->getLoopFlag() == false);

    auto test02 = std::static_pointer_cast<AnimClip>(nodes[1]);
    QVERIFY(test02->getURL() == "test02.fbx");
    QVERIFY(test02->getStartFrame() == 2.0f);
    QVERIFY(test02->getEndFrame() == 21.0f);
    QVERIFY(test02->getTimeScale() == 0.9f);
    QVERIFY(test02->getLoopFlag() == true);
}

void AnimTests::testVariant() {
    auto defaultVar = AnimVariant();
    auto boolVar = AnimVariant(true);
    auto intVar = AnimVariant(1);
    auto floatVar = AnimVariant(1.0f);
    auto vec3Var = AnimVariant(glm::vec3(1.0f, 2.0f, 3.0f));
    auto quatVar = AnimVariant(glm::quat(1.0f, 2.0f, 3.0f, 4.0f));
    auto mat4Var = AnimVariant(glm::mat4(glm::vec4(1.0f, 2.0f, 3.0f, 4.0f),
                                         glm::vec4(5.0f, 6.0f, 7.0f, 8.0f),
                                         glm::vec4(9.0f, 10.0f, 11.0f, 12.0f),
                                         glm::vec4(13.0f, 14.0f, 15.0f, 16.0f)));
    QVERIFY(defaultVar.isBool());
    QVERIFY(defaultVar.getBool() == false);

    QVERIFY(boolVar.isBool());
    QVERIFY(boolVar.getBool() == true);

    QVERIFY(intVar.isInt());
    QVERIFY(intVar.getInt() == 1);

    QVERIFY(floatVar.isFloat());
    QVERIFY(floatVar.getFloat() == 1.0f);

    QVERIFY(vec3Var.isVec3());
    auto v = vec3Var.getVec3();
    QVERIFY(v.x == 1.0f);
    QVERIFY(v.y == 2.0f);
    QVERIFY(v.z == 3.0f);

    QVERIFY(quatVar.isQuat());
    auto q = quatVar.getQuat();
    QVERIFY(q.w == 1.0f);
    QVERIFY(q.x == 2.0f);
    QVERIFY(q.y == 3.0f);
    QVERIFY(q.z == 4.0f);

    QVERIFY(mat4Var.isMat4());
    auto m = mat4Var.getMat4();
    QVERIFY(m[0].x == 1.0f);
    QVERIFY(m[3].w == 16.0f);
}
