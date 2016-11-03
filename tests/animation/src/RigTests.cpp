//
//  RigTests.cpp
//  tests/rig/src
//
//  Created by Howard Stearns on 6/16/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* FIXME/TBD:
 
 The following lower level functionality might be separated out into a separate class, covered by a separate test case class:
 - With no input, initial pose is standing, arms at side
 - Some single animation produces correct results at a given keyframe time.
 - Some single animation produces correct results at a given interpolated time.
 - Blend between two animations, started at separate times, produces correct result at a given interpolated time.
 - Head orientation can be overridden to produce change that doesn't come from the playing animation.
 - Hand position/orientation can be overridden to produce change that doesn't come from the playing animation.
 - Hand position/orientation can be overrridden to produce elbow change that doesn't come from the playing animation.
 - Respect scaling? (e.g., so that MyAvatar.increase/decreaseSize can alter rig, such that anti-scating and footfalls-on-stairs  works)

 Higher level functionality:
 - start/stopAnimation adds the animation to that which is playing, blending/fading as needed.
 - thrust causes walk role animation to be used.
 - turning causes turn role animation to be used. (two tests, correctly symmetric left & right)
 - walk/turn do not skate (footfalls match over-ground velocity)
 - (Later?) walk up stairs / hills have proper footfall for terrain
 - absence of above causes return to idle role animation to be used
 - (later?) The lower-level head/hand placements respect previous state. E.g., actual hand movement can be slower than requested.
 - (later) The lower-level head/hand placements can move whole skeleton. E.g., turning head past a limit may turn whole body. Reaching up can move shoulders and hips.
 
 Backward-compatability operations. We should think of this behavior as deprecated:
 - clearJointData return to standing. TBD: presumably with idle and all other animations NOT playing, until explicitly reenabled with a new TBD method?
 - setJointData applies the given data. Same TBD.
 These can be defined true or false, but the tests document the behavior and tells us if something's changed:
 - An external change to the original skeleton IS/ISN'T seen by the rig.
 - An external change to the original skeleton's head orientation IS/ISN'T seen by the rig.
 - An external change to the original skeleton's hand orientiation IS/ISN'T seen by the rig.
 */

#include <iostream>

#include "FBXReader.h"
#include "OBJReader.h" 

#include <Rig.h>
#include "RigTests.h"

static void reportJoint(RigPointer rig, int index) { // Handy for debugging
    std::cout << "\n";
    std::cout << index << " " << rig->getAnimSkeleton()->getJointName(index).toUtf8().data() << "\n";
    glm::vec3 pos;
    rig->getJointPosition(index, pos);
    glm::quat rot;
    rig->getJointRotation(index, rot);
    std::cout << " pos:" << pos << "\n";
    std::cout << " rot:" << safeEulerAngles(rot) << "\n";
    std::cout << "\n";
}
static void reportByName(RigPointer rig, const QString& name) {
    int jointIndex = rig->indexOfJoint(name);
    reportJoint(rig, jointIndex);
}
static void reportAll(RigPointer rig) {
    for (int i = 0; i < rig->getJointStateCount(); i++) {
        reportJoint(rig, i);
    }
}
static void reportSome(RigPointer rig) {
    QString names[] = {"Head", "Neck", "RightShoulder", "RightArm", "RightForeArm", "RightHand", "Spine2", "Spine1", "Spine", "Hips", "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase", "RightToe_End"};
    for (auto name : names) {
        reportByName(rig, name);
    }
}

QTEST_MAIN(RigTests)

void RigTests::initTestCase() {
//#define FROM_FILE "/Users/howardstearns/howardHiFi/Zack.fbx"
#ifdef FROM_FILE
    QFile file(FROM_FILE);
    QCOMPARE(file.open(QIODevice::ReadOnly), true);
    FBXGeometry* geometry = readFBX(file.readAll(), QVariantHash());
#else
    QUrl fbxUrl("https://s3.amazonaws.com/hifi-public/models/skeletons/Zack/Zack.fbx");
    QNetworkReply* reply = OBJReader().request(fbxUrl, false);  // Just a convenience hack for synchronoud http request
    auto fbxHttpCode = !reply->isFinished() ? -1 : reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QCOMPARE(fbxHttpCode, 200);
    FBXGeometry* geometry = readFBX(reply->readAll(), QVariantHash());
#endif
    QVERIFY((bool)geometry);

    _rig = std::make_shared<Rig>();
    _rig->initJointStates(*geometry, glm::mat4());
    std::cout << "Rig is ready " << geometry->joints.count() << " joints " << std::endl;
    reportAll(_rig);
}

void RigTests::initialPoseArmsDown() {
    reportSome(_rig);
}
