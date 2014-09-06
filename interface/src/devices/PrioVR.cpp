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

#include <QTimer>
#include <QtDebug>

#include <FBXReader.h>
#include <PerfStat.h>

#include "Application.h"
#include "PrioVR.h"
#include "ui/TextRenderer.h"

#ifdef HAVE_PRIOVR
const unsigned int SERIAL_LIST[] = { 0x00000001, 0x00000000, 0x00000008, 0x00000009, 0x0000000A,
    0x0000000C, 0x0000000D, 0x0000000E, 0x00000004, 0x00000005, 0x00000010, 0x00000011 };
const unsigned char AXIS_LIST[] = { 9, 43, 37, 37, 37, 13, 13, 13, 52, 52, 28, 28 };
const int LIST_LENGTH = sizeof(SERIAL_LIST) / sizeof(SERIAL_LIST[0]);

const char* JOINT_NAMES[] = { "Neck", "Spine", "LeftArm", "LeftForeArm", "LeftHand", "RightArm",
    "RightForeArm", "RightHand", "LeftUpLeg", "LeftLeg", "RightUpLeg", "RightLeg" };  

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

static void setPalm(float deltaTime, int index) {
    MyAvatar* avatar = Application::getInstance()->getAvatar();
    Hand* hand = avatar->getHand();
    PalmData* palm;
    bool foundHand = false;
    for (size_t j = 0; j < hand->getNumPalms(); j++) {
        if (hand->getPalms()[j].getSixenseID() == index) {
            palm = &(hand->getPalms()[j]);
            foundHand = true;
        }
    }
    if (!foundHand) {
        PalmData newPalm(hand);
        hand->getPalms().push_back(newPalm);
        palm = &(hand->getPalms()[hand->getNumPalms() - 1]);
        palm->setSixenseID(index);
    }
    
    palm->setActive(true);
    
    // Read controller buttons and joystick into the hand
    if (!Application::getInstance()->getJoystickManager()->getJoystickStates().isEmpty()) {
        const JoystickState& state = Application::getInstance()->getJoystickManager()->getJoystickStates().at(0);
        if (state.axes.size() >= 4 && state.buttons.size() >= 4) {
            if (index == LEFT_HAND_INDEX) {
                palm->setControllerButtons(state.buttons.at(1) ? BUTTON_FWD : 0);
                palm->setTrigger(state.buttons.at(0) ? 1.0f : 0.0f);
                palm->setJoystick(state.axes.at(0), -state.axes.at(1));
                
            } else {
                palm->setControllerButtons(state.buttons.at(3) ? BUTTON_FWD : 0);
                palm->setTrigger(state.buttons.at(2) ? 1.0f : 0.0f);
                palm->setJoystick(state.axes.at(2), -state.axes.at(3)); 
            }
        }
    }
    
    // NOTE: this math is done in the worl-frame with unecessary complexity.
    // TODO: transfom this to stay in the model-frame.
    glm::vec3 position;
    glm::quat rotation;
    SkeletonModel* skeletonModel = &Application::getInstance()->getAvatar()->getSkeletonModel();
    int jointIndex;
    glm::quat inverseRotation = glm::inverse(Application::getInstance()->getAvatar()->getOrientation());
    if (index == LEFT_HAND_INDEX) {
        jointIndex = skeletonModel->getLeftHandJointIndex();
        skeletonModel->getJointRotationInWorldFrame(jointIndex, rotation);      
        rotation = inverseRotation * rotation * glm::quat(glm::vec3(0.0f, PI_OVER_TWO, 0.0f));
        
    } else {
        jointIndex = skeletonModel->getRightHandJointIndex();
        skeletonModel->getJointRotationInWorldFrame(jointIndex, rotation);
        rotation = inverseRotation * rotation * glm::quat(glm::vec3(0.0f, -PI_OVER_TWO, 0.0f));
    }
    skeletonModel->getJointPositionInWorldFrame(jointIndex, position);
    position = inverseRotation * (position - skeletonModel->getTranslation());
    
    palm->setRawRotation(rotation);
    
    //  Compute current velocity from position change
    glm::vec3 rawVelocity;
    if (deltaTime > 0.f) {
        rawVelocity = (position - palm->getRawPosition()) / deltaTime; 
    } else {
        rawVelocity = glm::vec3(0.0f);
    }
    palm->setRawVelocity(rawVelocity);
    palm->setRawPosition(position);
    
    // Store the one fingertip in the palm structure so we can track velocity
    const float FINGER_LENGTH = 0.3f;   //  meters
    const glm::vec3 FINGER_VECTOR(0.0f, 0.0f, FINGER_LENGTH);
    const glm::vec3 newTipPosition = position + rotation * FINGER_VECTOR;
    glm::vec3 oldTipPosition = palm->getTipRawPosition();
    if (deltaTime > 0.f) {
        palm->setTipVelocity((newTipPosition - oldTipPosition) / deltaTime);
    } else {
        palm->setTipVelocity(glm::vec3(0.f));
    }
    palm->setTipPosition(newTipPosition);
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
    _lastJointRotations.resize(LIST_LENGTH);
    for (int i = 0; i < LIST_LENGTH; i++) {
        _humanIKJointIndices.append(jointsDiscovered[i] ? indexOfHumanIKJoint(JOINT_NAMES[i]) : -1);
    }
#endif
}

PrioVR::~PrioVR() {
#ifdef HAVE_PRIOVR
    if (_skeletalDevice) {
        yei_stopStreaming(_skeletalDevice);
    } 
#endif
}

const int HEAD_ROTATION_INDEX = 0;

bool PrioVR::hasHeadRotation() const {
    return _humanIKJointIndices.size() > HEAD_ROTATION_INDEX && _humanIKJointIndices.at(HEAD_ROTATION_INDEX) != -1;
}

glm::quat PrioVR::getHeadRotation() const {    
    return _jointRotations.size() > HEAD_ROTATION_INDEX ? _jointRotations.at(HEAD_ROTATION_INDEX) : glm::quat();
}

glm::quat PrioVR::getTorsoRotation() const {
    const int TORSO_ROTATION_INDEX = 1;
    return _jointRotations.size() > TORSO_ROTATION_INDEX ? _jointRotations.at(TORSO_ROTATION_INDEX) : glm::quat();
}

void PrioVR::update(float deltaTime) {
#ifdef HAVE_PRIOVR
    if (!_skeletalDevice) {
        return;
    }
    PerformanceTimer perfTimer("PrioVR");
    unsigned int timestamp;
    yei_getLastStreamDataAll(_skeletalDevice, (char*)_jointRotations.data(),
        _jointRotations.size() * sizeof(glm::quat), &timestamp);

    // convert to our expected coordinate system, average with last rotations to smooth
    for (int i = 0; i < _jointRotations.size(); i++) {
        _jointRotations[i].y *= -1.0f;
        _jointRotations[i].z *= -1.0f;
        
        glm::quat lastRotation = _lastJointRotations.at(i);
        _lastJointRotations[i] = _jointRotations.at(i);
        _jointRotations[i] = safeMix(lastRotation, _jointRotations.at(i), 0.5f);
    }
    
    // convert the joysticks into palm data
    setPalm(deltaTime, LEFT_HAND_INDEX);
    setPalm(deltaTime, RIGHT_HAND_INDEX);
#endif
}

void PrioVR::reset() {
#ifdef HAVE_PRIOVR
    if (!_skeletalDevice) {
        return;
    }
    connect(Application::getInstance(), SIGNAL(renderingOverlay()), SLOT(renderCalibrationCountdown()));
    _calibrationCountdownStarted = QDateTime::currentDateTime();
#endif
}

void PrioVR::renderCalibrationCountdown() {
#ifdef HAVE_PRIOVR
    const int COUNTDOWN_SECONDS = 3;
    int secondsRemaining = COUNTDOWN_SECONDS - _calibrationCountdownStarted.secsTo(QDateTime::currentDateTime());
    if (secondsRemaining == 0) {
        yei_tareSensors(_skeletalDevice);
        Application::getInstance()->disconnect(this);
        return;
    }
    static TextRenderer textRenderer(MONO_FONT_FAMILY, 18, QFont::Bold, false, TextRenderer::OUTLINE_EFFECT, 2);
    QByteArray text = "Assume T-Pose in " + QByteArray::number(secondsRemaining) + "...";
    textRenderer.draw((Application::getInstance()->getGLWidget()->getDeviceWidth() -
        textRenderer.computeWidth(text.constData())) / 2, Application::getInstance()->getGLWidget()->getDeviceHeight() / 2,
            text);
#endif  
}
