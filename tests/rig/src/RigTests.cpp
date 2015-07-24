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
#include <PathUtils.h>

#include "AvatarData.h"
#include "OBJReader.h" 
#include "FBXReader.h"

#include "AvatarRig.h" // We might later test Rig vs AvatarRig separately, but for now, we're concentrating on the main use case.
#include "RigTests.h"

QTEST_MAIN(RigTests)

void RigTests::initTestCase() {

    // There are two good ways we could organize this:
    // 1. Create a MyAvatar the same way that Interface does, and poke at it.
    //    We can't do that because MyAvatar (and even Avatar) are in interface, not a library, and our build system won't allow that dependency.
    // 2. Create just the minimum skeleton in the most direct way possible, using only very basic library APIs (such as fbx).
    //    I don't think we can do that because not everything we need is exposed directly from, e.g., the fst and fbx readers.
    // So here we do neither. Using as much as we can from AvatarData (which is in the avatar and further requires network and audio), and
    // duplicating whatever other code we need from (My)Avatar. Ugh.  We may refactor that later, but right now, cleaning this up is not on our critical path.

    // Joint mapping from fst. FIXME: Do we need this???
    /*auto avatar = std::make_shared<AvatarData>();
    QEventLoop loop; // Create an event loop that will quit when we get the finished signal
    QObject::connect(avatar.get(), &AvatarData::jointMappingLoaded, &loop, &QEventLoop::quit);
    avatar->setSkeletonModelURL(QUrl("https://hifi-public.s3.amazonaws.com/marketplace/contents/4a690585-3fa3-499e-9f8b-fd1226e561b1/e47e6898027aa40f1beb6adecc6a7db5.fst")); // Zach fst
    loop.exec();*/                    // Blocking all further tests until signalled.

    // Joint geometry from fbx.
#define FROM_FILE "/Users/howardstearns/howardHiFi/Zack.fbx"
#ifdef FROM_FILE
    QFile file(FROM_FILE);
    QCOMPARE(file.open(QIODevice::ReadOnly), true);
    FBXGeometry geometry = readFBX(file.readAll(), QVariantHash());
#else
    QUrl fbxUrl("https://s3.amazonaws.com/hifi-public/models/skeletons/Zack/Zack.fbx");
    QNetworkReply* reply = OBJReader().request(fbxUrl, false);  // Just a convenience hack for synchronoud http request
    auto fbxHttpCode = !reply->isFinished() ? -1 : reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QCOMPARE(fbxHttpCode, 200);
    FBXGeometry geometry = readFBX(reply->readAll(), QVariantHash());
#endif
    //QCOMPARE(geometry.joints.count(), avatar->getJointNames().count());

    QVector<JointState> jointStates;
    for (int i = 0; i < geometry.joints.size(); ++i) {
        // Note that if the geometry is stack allocated and goes away, so will the joints. Hence the heap copy here.
        FBXJoint* joint = new FBXJoint(geometry.joints[i]);
        JointState state;
        state.setFBXJoint(joint);
        jointStates.append(state);
    }

    _rig = std::make_shared<AvatarRig>();
    _rig->initJointStates(jointStates, glm::mat4(), geometry.neckJointIndex);
    std::cout << "Rig is ready " << geometry.joints.count() << " joints " << std::endl;
   }

void reportJoint(int index, JointState joint) { // Handy for debugging
    std::cout << "\n";
    std::cout << index << " " << joint.getFBXJoint().name.toUtf8().data() << "\n";
    std::cout << " pos:" << joint.getPosition() << "/" << joint.getPositionInParentFrame() << " from " << joint.getParentIndex() << "\n";
    std::cout << " rot:" << safeEulerAngles(joint.getRotation()) << "/" << safeEulerAngles(joint.getRotationInParentFrame()) << "/" << safeEulerAngles(joint.getRotationInBindFrame()) << "\n";
    std::cout << "\n";
}

void RigTests::initialPoseArmsDown() {
    for (int i = 0; i < _rig->getJointStateCount(); i++) {
        JointState joint = _rig->getJointState(i);
        reportJoint(i, joint);
    }
}
