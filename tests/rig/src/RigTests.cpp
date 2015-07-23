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
//#include "FSTReader.h"
// There are two good ways we could organize this:
// 1. Create a MyAvatar the same way that Interface does, and poke at it.
//    We can't do that because MyAvatar (and even Avatar) are in interface, not a library, and our build system won't allow that dependency.
// 2. Create just the minimum skeleton in the most direct way possible, using only very basic library APIs (such as fbx).
//    I don't think we can do that because not everything we need is exposed directly from, e.g., the fst and fbx readers.
// So here we do neither. Using as much as we can from AvatarData (which is in the avatar and further requires network and audio), and
// duplicating whatever other code we need from (My)Avatar. Ugh.  We may refactor that later, but right now, cleaning this up is not on our critical path.
#include "AvatarData.h"
#include "RigTests.h"

QTEST_MAIN(RigTests)

void RigTests::initTestCase() {
    AvatarData avatar;
    QEventLoop loop; // Create an event loop that will quit when we get the finished signal
    QObject::connect(&avatar, &AvatarData::jointsLoaded, &loop, &QEventLoop::quit);
    avatar.setSkeletonModelURL(QUrl("https://hifi-public.s3.amazonaws.com/marketplace/contents/4a690585-3fa3-499e-9f8b-fd1226e561b1/e47e6898027aa40f1beb6adecc6a7db5.fst")); // Zach
    //std::cout << "sleep start" << std::endl;
    loop.exec();                    // Nothing is going to happen on this whole run thread until we get this
    _rig = new Rig();
}

void RigTests::dummyPassTest() {
    bool x = true;
    std::cout << "dummyPassTest x=" << x << std::endl;
    QCOMPARE(x, true);
}

void RigTests::dummyFailTest() {
    bool x = false;
    std::cout << "dummyFailTest x=" << x << std::endl;
    QCOMPARE(x, true);
}
