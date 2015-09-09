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

void AnimTests::testClipInternalState() {
    std::string id = "my anim clip";
    std::string url = "https://hifi-public.s3.amazonaws.com/ozan/support/FightClubBotTest1/Animations/standard_idle.fbx";
    float startFrame = 2.0f;
    float endFrame = 20.0f;
    float timeScale = 1.1f;
    bool loopFlag = true;

    AnimClip clip(id, url, startFrame, endFrame, timeScale, loopFlag);

    QVERIFY(clip.getID() == id);
    QVERIFY(clip.getType() == AnimNode::Type::Clip);

    QVERIFY(clip._url == url);
    QVERIFY(clip._startFrame == startFrame);
    QVERIFY(clip._endFrame == endFrame);
    QVERIFY(clip._timeScale == timeScale);
    QVERIFY(clip._loopFlag == loopFlag);
}

static float framesToSec(float secs) {
    const float FRAMES_PER_SECOND = 30.0f;
    return secs / FRAMES_PER_SECOND;
}

void AnimTests::testClipEvaulate() {
    std::string id = "myClipNode";
    std::string url = "https://hifi-public.s3.amazonaws.com/ozan/support/FightClubBotTest1/Animations/standard_idle.fbx";
    float startFrame = 2.0f;
    float endFrame = 22.0f;
    float timeScale = 1.0f;
    float loopFlag = true;

    auto vars = AnimVariantMap();
    vars.set("FalseVar", false);

    AnimClip clip(id, url, startFrame, endFrame, timeScale, loopFlag);

    AnimNode::Triggers triggers;
    clip.evaluate(vars, framesToSec(10.0f), triggers);
    QCOMPARE_WITH_ABS_ERROR(clip._frame, 12.0f, EPSILON);

    // does it loop?
    triggers.clear();
    clip.evaluate(vars, framesToSec(11.0f), triggers);
    QCOMPARE_WITH_ABS_ERROR(clip._frame, 3.0f, EPSILON);

    // did we receive a loop trigger?
    QVERIFY(std::find(triggers.begin(), triggers.end(), "myClipNodeOnLoop") != triggers.end());

    // does it pause at end?
    triggers.clear();
    clip.setLoopFlagVar("FalseVar");
    clip.evaluate(vars, framesToSec(20.0f), triggers);
    QCOMPARE_WITH_ABS_ERROR(clip._frame, 22.0f, EPSILON);

    // did we receive a done trigger?
    QVERIFY(std::find(triggers.begin(), triggers.end(), "myClipNodeOnDone") != triggers.end());
}

void AnimTests::testClipEvaulateWithVars() {
    std::string id = "myClipNode";
    std::string url = "https://hifi-public.s3.amazonaws.com/ozan/support/FightClubBotTest1/Animations/standard_idle.fbx";
    float startFrame = 2.0f;
    float endFrame = 22.0f;
    float timeScale = 1.0f;
    float loopFlag = true;

    float startFrame2 = 22.0f;
    float endFrame2 = 100.0f;
    float timeScale2 = 1.2f;
    bool loopFlag2 = false;

    auto vars = AnimVariantMap();
    vars.set("startFrame2", startFrame2);
    vars.set("endFrame2", endFrame2);
    vars.set("timeScale2", timeScale2);
    vars.set("loopFlag2", loopFlag2);

    AnimClip clip(id, url, startFrame, endFrame, timeScale, loopFlag);
    clip.setStartFrameVar("startFrame2");
    clip.setEndFrameVar("endFrame2");
    clip.setTimeScaleVar("timeScale2");
    clip.setLoopFlagVar("loopFlag2");

    AnimNode::Triggers triggers;
    clip.evaluate(vars, framesToSec(0.1f), triggers);

    // verify that the values from the AnimVariantMap made it into the clipNode's
    // internal state
    QVERIFY(clip._startFrame == startFrame2);
    QVERIFY(clip._endFrame == endFrame2);
    QVERIFY(clip._timeScale == timeScale2);
    QVERIFY(clip._loopFlag == loopFlag2);
}

void AnimTests::testLoader() {
    auto url = QUrl("https://gist.githubusercontent.com/hyperlogic/857129fe04567cbe670f/raw/8ba57a8f0a76f88b39a11f77f8d9df04af9cec95/test.json");
    // NOTE: This will warn about missing "test01.fbx", "test02.fbx", etc. if the resource loading code doesn't handle relative pathnames!
    // However, the test will proceed.
    AnimNodeLoader loader(url);

    const int timeout = 1000;
    QEventLoop loop;

    AnimNode::Pointer node = nullptr;
    connect(&loader, &AnimNodeLoader::success, [&](AnimNode::Pointer nodeIn) { node = nodeIn; });

    loop.connect(&loader, SIGNAL(success(AnimNode::Pointer)), SLOT(quit()));
    loop.connect(&loader, SIGNAL(error(int, QString)), SLOT(quit()));
    QTimer::singleShot(timeout, &loop, SLOT(quit()));

    loop.exec();

    QVERIFY((bool)node);

    QVERIFY(node->getID() == "blend");
    QVERIFY(node->getType() == AnimNode::Type::BlendLinear);

    QVERIFY((bool)node);
    QVERIFY(node->getID() == "blend");
    QVERIFY(node->getType() == AnimNode::Type::BlendLinear);

    auto blend = std::static_pointer_cast<AnimBlendLinear>(node);
    QVERIFY(blend->_alpha == 0.5f);

    QVERIFY(node->getChildCount() == 3);

    std::shared_ptr<AnimNode> nodes[3] = { node->getChild(0), node->getChild(1), node->getChild(2) };

    QVERIFY(nodes[0]->getID() == "test01");
    QVERIFY(nodes[0]->getChildCount() == 0);
    QVERIFY(nodes[1]->getID() == "test02");
    QVERIFY(nodes[1]->getChildCount() == 0);
    QVERIFY(nodes[2]->getID() == "test03");
    QVERIFY(nodes[2]->getChildCount() == 0);

    auto test01 = std::static_pointer_cast<AnimClip>(nodes[0]);
    QVERIFY(test01->_url == "test01.fbx");
    QVERIFY(test01->_startFrame == 1.0f);
    QVERIFY(test01->_endFrame == 20.0f);
    QVERIFY(test01->_timeScale == 1.0f);
    QVERIFY(test01->_loopFlag == false);

    auto test02 = std::static_pointer_cast<AnimClip>(nodes[1]);
    QVERIFY(test02->_url == "test02.fbx");
    QVERIFY(test02->_startFrame == 2.0f);
    QVERIFY(test02->_endFrame == 21.0f);
    QVERIFY(test02->_timeScale == 0.9f);
    QVERIFY(test02->_loopFlag == true);
}

void AnimTests::testVariant() {
    auto defaultVar = AnimVariant();
    auto boolVarTrue = AnimVariant(true);
    auto boolVarFalse = AnimVariant(false);
    auto intVarZero = AnimVariant(0);
    auto intVarOne = AnimVariant(1);
    auto intVarNegative = AnimVariant(-1);
    auto floatVarZero = AnimVariant(0.0f);
    auto floatVarOne = AnimVariant(1.0f);
    auto floatVarNegative = AnimVariant(-1.0f);
    auto vec3Var = AnimVariant(glm::vec3(1.0f, -2.0f, 3.0f));
    auto quatVar = AnimVariant(glm::quat(1.0f, 2.0f, -3.0f, 4.0f));
    auto mat4Var = AnimVariant(glm::mat4(glm::vec4(1.0f, 2.0f, 3.0f, 4.0f),
                                         glm::vec4(5.0f, 6.0f, -7.0f, 8.0f),
                                         glm::vec4(9.0f, 10.0f, 11.0f, 12.0f),
                                         glm::vec4(13.0f, 14.0f, 15.0f, 16.0f)));
    QVERIFY(defaultVar.isBool());
    QVERIFY(defaultVar.getBool() == false);

    QVERIFY(boolVarTrue.isBool());
    QVERIFY(boolVarTrue.getBool() == true);
    QVERIFY(boolVarFalse.isBool());
    QVERIFY(boolVarFalse.getBool() == false);

    QVERIFY(intVarZero.isInt());
    QVERIFY(intVarZero.getInt() == 0);
    QVERIFY(intVarOne.isInt());
    QVERIFY(intVarOne.getInt() == 1);
    QVERIFY(intVarNegative.isInt());
    QVERIFY(intVarNegative.getInt() == -1);

    QVERIFY(floatVarZero.isFloat());
    QVERIFY(floatVarZero.getFloat() == 0.0f);
    QVERIFY(floatVarOne.isFloat());
    QVERIFY(floatVarOne.getFloat() == 1.0f);
    QVERIFY(floatVarNegative.isFloat());
    QVERIFY(floatVarNegative.getFloat() == -1.0f);

    QVERIFY(vec3Var.isVec3());
    auto v = vec3Var.getVec3();
    QVERIFY(v.x == 1.0f);
    QVERIFY(v.y == -2.0f);
    QVERIFY(v.z == 3.0f);

    QVERIFY(quatVar.isQuat());
    auto q = quatVar.getQuat();
    QVERIFY(q.w == 1.0f);
    QVERIFY(q.x == 2.0f);
    QVERIFY(q.y == -3.0f);
    QVERIFY(q.z == 4.0f);

    QVERIFY(mat4Var.isMat4());
    auto m = mat4Var.getMat4();
    QVERIFY(m[0].x == 1.0f);
    QVERIFY(m[1].z == -7.0f);
    QVERIFY(m[3].w == 16.0f);
}
