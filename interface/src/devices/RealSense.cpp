//
//  RealSense.cpp
//  interface/src/devices
//
//  Created by Thijs Wenker on 12/10/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RealSense.h"
#include "Menu.h"
#include "SharedUtil.h"

const int PALMROOT_NUM_JOINTS = 2;
const int FINGER_NUM_JOINTS = 4;

const DeviceTracker::Name RealSense::NAME = "RealSense";

// find the index of a joint from
// the side: true = right
// the finger & the bone:
//             finger in [0..4] : bone in  [0..3] a finger phalange
//             [-1]   up the hand branch : bone in [0..2] <=> [ hand, forearm, arm]
MotionTracker::Index evalRealSenseJointIndex(bool isRightSide, int finger, int bone) {

    MotionTracker::Index offset = 1                           // start after root
        + (int(isRightSide) * PXCHandData::NUMBER_OF_JOINTS)  // then offset for side
        + PALMROOT_NUM_JOINTS;                                // then add the arm/forearm/hand chain
    if (finger >= 0) {
        // from there go down in the correct finger and bone
        return offset + (finger * FINGER_NUM_JOINTS) + bone;
    } else {
        // or go back up for the correct root bone
        return offset - 1 - bone;
    }
}

// static
void RealSense::init() {
    DeviceTracker* device = DeviceTracker::getDevice(NAME);
    if (!device) {
        // create a new RealSense and register it
        RealSense* realSense = new RealSense();
        DeviceTracker::registerDevice(NAME, realSense);
    }
}

// static
RealSense* RealSense::getInstance() {
    DeviceTracker* device = DeviceTracker::getDevice(NAME);
    if (!device) {
        // create a new RealSense and register it
        RealSense* realSense = new RealSense();
        DeviceTracker::registerDevice(NAME, realSense);
    }
    return dynamic_cast< RealSense* > (device);
}

RealSense::RealSense() :
     MotionTracker(),
    _active(false),
    _handData(NULL)
{
    _session = PXCSession_Create();
    initSession(false, NULL);

    // Create the RealSense joint hierarchy
    std::vector< Semantic > sides;
    sides.push_back("joint_L_");
    sides.push_back("joint_R_");

    std::vector< Semantic > rootBones;
    rootBones.push_back("wrist");
    rootBones.push_back("hand");

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
                rootJoint = addJoint(sides[s] + fingers[f] + fingerBones[b], rootJoint);
            }
        }
    }
}

RealSense::~RealSense() {
    _manager->Release();
}

void RealSense::initSession(bool playback, QString filename) {
    _active = false;
    _properlyInitialized = false;
    if (_handData != NULL) {
        _handData->Release();
        _handController->Release();
        _session->Release();
    }
    _manager = _session->CreateSenseManager();
    if (playback) {
        _manager->QueryCaptureManager()->SetFileName(filename.toStdWString().c_str(), false);
    }
    _manager->QueryCaptureManager()->SetRealtime(!playback);
    _manager->EnableHand(0);
    _handController = _manager->QueryHand();

    if (_manager->Init() == PXC_STATUS_NO_ERROR) {
        _handData = _handController->CreateOutput();

        PXCCapture::Device *device = _manager->QueryCaptureManager()->QueryDevice();
        PXCCapture::DeviceInfo dinfo;
        _manager->QueryCaptureManager()->QueryDevice()->QueryDeviceInfo(&dinfo);
        if (dinfo.model == PXCCapture::DEVICE_MODEL_IVCAM)
        {
            device->SetDepthConfidenceThreshold(1);
            device->SetMirrorMode(PXCCapture::Device::MIRROR_MODE_DISABLED);
            device->SetIVCAMFilterOption(6);
        }
        _properlyInitialized = true;
    }
}

#ifdef HAVE_RSSDK
glm::quat quatFromPXCPoint4DF32(const PXCPoint4DF32& basis) {
    return glm::quat(basis.w, basis.x, basis.y, basis.z);
}

glm::vec3 vec3FromPXCPoint3DF32(const PXCPoint3DF32& vec) {
    return glm::vec3(vec.x, vec.y, vec.z);
}
#endif

void RealSense::update() {
#ifdef HAVE_RSSDK
    bool wasActive = _active;
    _active = _manager->IsConnected() && _properlyInitialized;
    if (_active || wasActive) {
        // Go through all the joints and increment their counter since last update.
        // Increment all counters once after controller first becomes inactive so that each joint reports itself as inactive.
        // TODO C++11 for (auto jointIt = _jointsArray.begin(); jointIt != _jointsArray.end(); jointIt++) {
        for (JointTracker::Vector::iterator jointIt = _jointsArray.begin(); jointIt != _jointsArray.end(); jointIt++) {
            (*jointIt).tickNewFrame();
        }
    }

    if (!_active) {
        return;
    }

    pxcStatus sts = _manager->AcquireFrame(true);
    _handData->Update();
    PXCHandData::JointData nodes[2][PXCHandData::NUMBER_OF_JOINTS] = {};
    PXCHandData::ExtremityData extremitiesPointsNodes[2][PXCHandData::NUMBER_OF_EXTREMITIES] = {};
    for (pxcI32 i = 0; i < _handData->QueryNumberOfHands(); i++) {
        PXCHandData::IHand* handData;
        if (_handData->QueryHandData(PXCHandData::ACCESS_ORDER_BY_TIME, i, handData) == PXC_STATUS_NO_ERROR) {
            int rightSide = handData->QueryBodySide() == PXCHandData::BODY_SIDE_RIGHT;
            PXCHandData::JointData jointData;
            JointTracker* parentJointTracker = _jointsArray.data();
            //Iterate Joints
            int rootBranchIndex = -1;
            JointTracker* palmJoint = NULL;
            for (int j = 0; j < PXCHandData::NUMBER_OF_JOINTS; j++) {
                handData->QueryTrackedJoint((PXCHandData::JointType)j, jointData);
                nodes[i][j] = jointData;
                if (j == PXCHandData::JOINT_WRIST) {
                    JointTracker* wrist = editJointTracker(evalRealSenseJointIndex(rightSide, rootBranchIndex, 1)); // 1 is the index of the wrist joint
                    wrist->editAbsFrame().setTranslation(vec3FromPXCPoint3DF32(jointData.positionWorld));
                    wrist->editAbsFrame().setRotation(quatFromPXCPoint4DF32(jointData.globalOrientation));
                    wrist->updateLocFromAbsTransform(parentJointTracker);
                    wrist->activeFrame();
                    parentJointTracker = wrist;
                    continue;
                } else if (j == PXCHandData::JOINT_CENTER) {
                    palmJoint = editJointTracker(evalRealSenseJointIndex(rightSide, rootBranchIndex, 0)); // 0 is the index of the palm joint
                    palmJoint->editAbsFrame().setTranslation(vec3FromPXCPoint3DF32(jointData.positionWorld));
                    palmJoint->editAbsFrame().setRotation(quatFromPXCPoint4DF32(jointData.globalOrientation));
                    palmJoint->updateLocFromAbsTransform(parentJointTracker);
                    palmJoint->activeFrame();
                    parentJointTracker = palmJoint;
                    continue;
                }
                int finger_index = j - PALMROOT_NUM_JOINTS;
                int finger = finger_index / FINGER_NUM_JOINTS;
                int finger_bone = finger_index % FINGER_NUM_JOINTS;
                JointTracker* ljointTracker = editJointTracker(evalRealSenseJointIndex(rightSide, finger, finger_bone));
                if (jointData.confidence > 0) {
                    ljointTracker->editAbsFrame().setTranslation(vec3FromPXCPoint3DF32(jointData.positionWorld));
                    ljointTracker->editAbsFrame().setRotation(quatFromPXCPoint4DF32(jointData.globalOrientation));
                    ljointTracker->updateLocFromAbsTransform(parentJointTracker);
                    ljointTracker->activeFrame();
                }
                if (finger_bone == (FINGER_NUM_JOINTS - 1)) {
                    parentJointTracker = palmJoint;
                    continue;
                }
                parentJointTracker = ljointTracker;
            }
        }
    }
    _manager->ReleaseFrame();
#endif // HAVE_RSSDK
}

void RealSense::loadRSSDKFile() {
    QString fileNameString = QFileDialog::getOpenFileName(Application::getInstance()->getGLWidget(), tr("Open RSSDK clip"),
        NULL,
        tr("RSSDK Recordings (*.rssdk)"));
    if (!fileNameString.isEmpty()) {
        initSession(true, fileNameString);
    }
}
