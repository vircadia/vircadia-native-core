//
//  Leapmotion.cpp
//  interface/src/devices
//
//  Created by Sam Cake on 6/2/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "SharedUtil.h"
#include "Leapmotion.h"

const int PALMROOT_NUM_JOINTS = 3;
const int FINGER_NUM_JOINTS = 4;
const int HAND_NUM_JOINTS = FINGER_NUM_JOINTS*5+PALMROOT_NUM_JOINTS;

const DeviceTracker::Name Leapmotion::NAME = "Leapmotion";

// find the index of a joint from
// the side: true = right
// the finger & the bone:
//             finger in [0..4] : bone in  [0..3] a finger phalange
//             [-1]   up the hand branch : bone in [0..2] <=> [ hand, forearm, arm]
MotionTracker::Index evalJointIndex(bool isRightSide, int finger, int bone) {

    MotionTracker::Index offset = 1                                     // start after root
                                + (int(isRightSide) * HAND_NUM_JOINTS)  // then offset for side
                                + PALMROOT_NUM_JOINTS;                  // then add the arm/forearm/hand chain
    if (finger >= 0) {
        // from there go down in the correct finger and bone
        return offset + (finger * FINGER_NUM_JOINTS) + bone;
    } else {
        // or go back up for the correct root bone
        return offset - 1 - bone;
    }
}

// static
void Leapmotion::init() {
    DeviceTracker* device = DeviceTracker::getDevice(NAME);

    if (!device) {
        // create a new Leapmotion and register it
        Leapmotion* leap = new Leapmotion();
        DeviceTracker::registerDevice(NAME, leap);
    }
}

// static
Leapmotion* Leapmotion::getInstance() {
    DeviceTracker* device = DeviceTracker::getDevice(NAME);
    if (!device) {
        // create a new Leapmotion and register it
        device = new Leapmotion();
        DeviceTracker::registerDevice(NAME, device);
    }
    return dynamic_cast< Leapmotion* > (device);
}

Leapmotion::Leapmotion() :
     MotionTracker(),
    _active(false)
{
    // Create the Leapmotion joint hierarchy
    std::vector< Semantic > sides;
    sides.push_back("joint_L_");
    sides.push_back("joint_R_");

    std::vector< Semantic > rootBones;
    rootBones.push_back("elbow");
    rootBones.push_back("hand");
    rootBones.push_back("wrist");

    std::vector< Semantic > fingers;
    fingers.push_back("thumb");
    fingers.push_back("index");
    fingers.push_back("middle");
    fingers.push_back("ring");
    fingers.push_back("pinky");

    std::vector< Semantic > fingerBones;
    fingerBones.push_back("1");
    fingerBones.push_back("2");
    fingerBones.push_back("3");
    fingerBones.push_back("4");

    std::vector< Index > palms;
    for (unsigned int s = 0; s < sides.size(); s++) {
        Index rootJoint = 0;
        for (unsigned int rb = 0; rb < rootBones.size(); rb++) {
            rootJoint = addJoint(sides[s] + rootBones[rb], rootJoint);
        }

        // capture the hand index for debug
        palms.push_back(rootJoint);

        for (unsigned int f = 0; f < fingers.size(); f++) {
            for (unsigned int b = 0; b < fingerBones.size(); b++) {
                rootJoint  = addJoint(sides[s] + fingers[f] + fingerBones[b], rootJoint);
            }
        }
    }
}

Leapmotion::~Leapmotion() {
}

#ifdef HAVE_LEAPMOTION
glm::quat quatFromLeapBase(float sideSign, const Leap::Matrix& basis) {

    // fix the handness to right and always...
    glm::vec3 xAxis = glm::normalize(sideSign * glm::vec3(basis.xBasis.x, basis.xBasis.y, basis.xBasis.z));
    glm::vec3 yAxis = glm::normalize(glm::vec3(basis.yBasis.x, basis.yBasis.y, basis.yBasis.z));
    glm::vec3 zAxis = glm::normalize(glm::vec3(basis.zBasis.x, basis.zBasis.y, basis.zBasis.z));

    xAxis = glm::normalize(glm::cross(yAxis, zAxis));

    glm::quat orientation = (glm::quat_cast(glm::mat3(xAxis, yAxis, zAxis)));
    return orientation;
}

glm::vec3 vec3FromLeapVector(const Leap::Vector& vec) {
    return glm::vec3(vec.x * METERS_PER_MILLIMETER, vec.y * METERS_PER_MILLIMETER, vec.z * METERS_PER_MILLIMETER);
}

#endif

void Leapmotion::update() {
#ifdef HAVE_LEAPMOTION
    // Check that the controller is actually active
    _active = _controller.isConnected();
    if (!_active) {
        return;
    }

    // go through all the joints and increment their counter since last update
    // TODO C++11 for (auto jointIt = _jointsArray.begin(); jointIt != _jointsArray.end(); jointIt++) {
    for (JointTracker::Vector::iterator jointIt = _jointsArray.begin(); jointIt != _jointsArray.end(); jointIt++) {
        (*jointIt).tickNewFrame();
    }

    // Get the most recent frame and report some basic information
    const Leap::Frame frame = _controller.frame();
    static int64_t lastFrameID = -1;
    int64_t newFrameID = frame.id();

    // If too soon then exit
    if (lastFrameID >= newFrameID)
        return;

    glm::vec3 delta(0.f);
    glm::quat handOri;
    if (!frame.hands().isEmpty()) {
        for (int handNum = 0; handNum < frame.hands().count(); handNum++) {

            const Leap::Hand hand = frame.hands()[handNum];
            int side = (hand.isRight() ? 1 : -1);

            JointTracker* parentJointTracker = _jointsArray.data();


            int rootBranchIndex = -1;

            Leap::Arm arm = hand.arm();
            if (arm.isValid()) {
                glm::quat ori = quatFromLeapBase(float(side), arm.basis());
                glm::vec3 pos = vec3FromLeapVector(arm.elbowPosition());
                JointTracker* elbow = editJointTracker(evalJointIndex((side > 0), rootBranchIndex, 2)); // 2 is the index of the elbow joint
                elbow->editAbsFrame().setTranslation(pos);
                elbow->editAbsFrame().setRotation(ori);
                elbow->updateLocFromAbsTransform(parentJointTracker);
                elbow->activeFrame();
                parentJointTracker = elbow;

                pos = vec3FromLeapVector(arm.wristPosition());
                JointTracker* wrist = editJointTracker(evalJointIndex((side > 0), rootBranchIndex, 1)); // 1 is the index of the wrist joint
                wrist->editAbsFrame().setTranslation(pos);
                wrist->editAbsFrame().setRotation(ori);
                wrist->updateLocFromAbsTransform(parentJointTracker);
                wrist->activeFrame();
                parentJointTracker = wrist;
            }

            JointTracker* palmJoint = NULL;
            {
                glm::vec3 pos = vec3FromLeapVector(hand.palmPosition());
                glm::quat ori = quatFromLeapBase(float(side), hand.basis());

                palmJoint = editJointTracker(evalJointIndex((side > 0), rootBranchIndex, 0)); // 0 is the index of the palm joint
                palmJoint->editAbsFrame().setTranslation(pos);
                palmJoint->editAbsFrame().setRotation(ori);
                palmJoint->updateLocFromAbsTransform(parentJointTracker);
                palmJoint->activeFrame();
            }
            
            // Check if the hand has any fingers
            const Leap::FingerList fingers = hand.fingers();
            if (!fingers.isEmpty()) {
                // For every fingers in the list
                for (int i = 0; i < fingers.count(); ++i) {
                    // Reset the parent joint to the palmJoint for every finger traversal
                    parentJointTracker = palmJoint;

                    // surprisingly, Leap::Finger::Type start at 0 for thumb a until 4 for the pinky
                    Index fingerIndex = evalJointIndex((side > 0), int(fingers[i].type()), 0);

                    // let's update the finger's joints
                    for (int b = 0; b < FINGER_NUM_JOINTS; b++) {
                        Leap::Bone::Type type = Leap::Bone::Type(b + Leap::Bone::TYPE_METACARPAL);
                        Leap::Bone bone = fingers[i].bone(type);
                        JointTracker* ljointTracker = editJointTracker(fingerIndex + b);
                        if (bone.isValid()) {
                            Leap::Vector bp = bone.nextJoint();

                            ljointTracker->editAbsFrame().setTranslation(vec3FromLeapVector(bp));
                            ljointTracker->editAbsFrame().setRotation(quatFromLeapBase(float(side), bone.basis()));
                            ljointTracker->updateLocFromAbsTransform(parentJointTracker);
                            ljointTracker->activeFrame();
                        }
                        parentJointTracker = ljointTracker;
                    }
                }
            }
        }
    }

    lastFrameID = newFrameID;
#endif
}
