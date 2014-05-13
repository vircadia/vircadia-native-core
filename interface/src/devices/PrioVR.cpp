//
//  PrioVR.cpp
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 5/12/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtDebug>

#include <FBXReader.h>

#include "PrioVR.h"

const unsigned int SERIAL_LIST[] = { 0x00000001, 0x00000000, 0x00000008, 0x00000009, 0x0000000A,
    0x0000000C, 0x0000000D, 0x0000000E, 0x00000004, 0x00000005, 0x00000010, 0x00000011 };
const unsigned char AXIS_LIST[] = { 9, 43, 37, 37, 37, 13, 13, 13, 52, 52, 28, 28 };
const int LIST_LENGTH = sizeof(SERIAL_LIST) / sizeof(SERIAL_LIST[0]);

const char* JOINT_NAMES[] = { "Head", "Spine", "LeftArm", "LeftForeArm", "LeftHand", "RightArm",
    "RightForeArm", "RightHand", "LeftUpLeg", "LeftLeg", "RightUpLeft", "RightLeg" };  

#ifdef HAVE_PRIOVR
static int indexOfHumanIKJoint(const char* jointName) {
    for (int i = 0;; i++) {
        QByteArray humanIKJoint = HUMANIK_JOINTS[i];
        if (humanIKJoint.isEmpty()) {
            return -1;
        }
        if (humanIKJoint == jointName) {
            return i;
        }
    }
}
#endif

PrioVR::PrioVR() {
#ifdef HAVE_PRIOVR
    char jointsDiscovered[LIST_LENGTH];
    _skeletalDevice = yei_setUpPrioVRSensors(0x00000000, const_cast<unsigned int*>(SERIAL_LIST),
        const_cast<unsigned char*>(AXIS_LIST), jointsDiscovered, LIST_LENGTH, YEI_TIMESTAMP_SYSTEM);
    if (!_skeletalDevice) {
        return;
    }
    _jointRotations.resize(LIST_LENGTH);
    for (int i = 0; i < LIST_LENGTH; i++) {
        _humanIKJointIndices.append(jointsDiscovered[i] ? indexOfHumanIKJoint(JOINT_NAMES[i]) : -1);
    }
    yei_tareSensors(_skeletalDevice);
#endif
}

PrioVR::~PrioVR() {
#ifdef HAVE_PRIOVR
    if (_skeletalDevice) {
        yei_stopStreaming(_skeletalDevice);
    } 
#endif
}

void PrioVR::update() {
#ifdef HAVE_PRIOVR
    if (!_skeletalDevice) {
        return;
    }
    unsigned int timestamp;
    yei_getLastStreamDataAll(_skeletalDevice, (char*)_jointRotations.data(),
        _jointRotations.size() * sizeof(glm::quat), &timestamp);
#endif
}
