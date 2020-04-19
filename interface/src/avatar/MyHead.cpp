//
//  Created by Bradley Austin Davis on 2017/04/27
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MyHead.h"

#include <glm/gtx/quaternion.hpp>
#include <gpu/Batch.h>

#include <NodeList.h>
#include <recording/Deck.h>
#include <Rig.h>
#include <BlendshapeConstants.h>

#include "Application.h"
#include "MyAvatar.h"

using namespace std;

static controller::Action blendshapeActions[] = {
    controller::Action::EYEBLINK_L,
    controller::Action::EYEBLINK_R,
    controller::Action::EYESQUINT_L,
    controller::Action::EYESQUINT_R,
    controller::Action::EYEDOWN_L,
    controller::Action::EYEDOWN_R,
    controller::Action::EYEIN_L,
    controller::Action::EYEIN_R,
    controller::Action::EYEOPEN_L,
    controller::Action::EYEOPEN_R,
    controller::Action::EYEOUT_L,
    controller::Action::EYEOUT_R,
    controller::Action::EYEUP_L,
    controller::Action::EYEUP_R,
    controller::Action::BROWSD_L,
    controller::Action::BROWSD_R,
    controller::Action::BROWSU_C,
    controller::Action::BROWSU_L,
    controller::Action::BROWSU_R,
    controller::Action::JAWFWD,
    controller::Action::JAWLEFT,
    controller::Action::JAWOPEN,
    controller::Action::JAWRIGHT,
    controller::Action::MOUTHLEFT,
    controller::Action::MOUTHRIGHT,
    controller::Action::MOUTHFROWN_L,
    controller::Action::MOUTHFROWN_R,
    controller::Action::MOUTHSMILE_L,
    controller::Action::MOUTHSMILE_R,
    controller::Action::MOUTHDIMPLE_L,
    controller::Action::MOUTHDIMPLE_R,
    controller::Action::LIPSSTRETCH_L,
    controller::Action::LIPSSTRETCH_R,
    controller::Action::LIPSUPPERCLOSE,
    controller::Action::LIPSLOWERCLOSE,
    controller::Action::LIPSFUNNEL,
    controller::Action::LIPSPUCKER,
    controller::Action::PUFF,
    controller::Action::CHEEKSQUINT_L,
    controller::Action::CHEEKSQUINT_R,
    controller::Action::MOUTHCLOSE,
    controller::Action::MOUTHUPPERUP_L,
    controller::Action::MOUTHUPPERUP_R,
    controller::Action::MOUTHLOWERDOWN_L,
    controller::Action::MOUTHLOWERDOWN_R,
    controller::Action::MOUTHPRESS_L,
    controller::Action::MOUTHPRESS_R,
    controller::Action::MOUTHSHRUGLOWER,
    controller::Action::MOUTHSHRUGUPPER,
    controller::Action::NOSESNEER_L,
    controller::Action::NOSESNEER_R,
    controller::Action::TONGUEOUT,
    controller::Action::USERBLENDSHAPE0,
    controller::Action::USERBLENDSHAPE1,
    controller::Action::USERBLENDSHAPE2,
    controller::Action::USERBLENDSHAPE3,
    controller::Action::USERBLENDSHAPE4,
    controller::Action::USERBLENDSHAPE5,
    controller::Action::USERBLENDSHAPE6,
    controller::Action::USERBLENDSHAPE7,
    controller::Action::USERBLENDSHAPE8,
    controller::Action::USERBLENDSHAPE9
};

MyHead::MyHead(MyAvatar* owningAvatar) : Head(owningAvatar) {
}

glm::quat MyHead::getHeadOrientation() const {
    // NOTE: Head::getHeadOrientation() is not used for orienting the camera "view" while in Oculus mode, so
    // you may wonder why this code is here. This method will be called while in Oculus mode to determine how
    // to change the driving direction while in Oculus mode. It is used to support driving toward where your
    // head is looking. Note that in oculus mode, your actual camera view and where your head is looking is not
    // always the same.

    MyAvatar* myAvatar = static_cast<MyAvatar*>(_owningAvatar);
    auto headPose = myAvatar->getControllerPoseInWorldFrame(controller::Action::HEAD);
    if (headPose.isValid()) {
        return headPose.rotation * Quaternions::Y_180;
    }

    return myAvatar->getWorldOrientation() * glm::quat(glm::radians(glm::vec3(_basePitch, 0.0f, 0.0f)));
}

void MyHead::simulate(float deltaTime) {
    auto player = DependencyManager::get<recording::Deck>();
    // Only use face trackers when not playing back a recording.
    if (!player->isPlaying()) {

        auto userInputMapper = DependencyManager::get<UserInputMapper>();

        // if input system has control over blink blendshapes
        bool eyeLidsTracked =
            userInputMapper->getActionStateValid(controller::Action::EYEBLINK_L) ||
            userInputMapper->getActionStateValid(controller::Action::EYEBLINK_R);

        // if input system has control over the brows.
        bool browsTracked =
            userInputMapper->getActionStateValid(controller::Action::BROWSD_L) ||
            userInputMapper->getActionStateValid(controller::Action::BROWSD_R) ||
            userInputMapper->getActionStateValid(controller::Action::BROWSU_L) ||
            userInputMapper->getActionStateValid(controller::Action::BROWSU_R) ||
            userInputMapper->getActionStateValid(controller::Action::BROWSU_C);

        // if input system has control of mouth
        bool mouthTracked =
            userInputMapper->getActionStateValid(controller::Action::JAWOPEN) ||
            userInputMapper->getActionStateValid(controller::Action::LIPSUPPERCLOSE) ||
            userInputMapper->getActionStateValid(controller::Action::LIPSLOWERCLOSE) ||
            userInputMapper->getActionStateValid(controller::Action::LIPSFUNNEL) ||
            userInputMapper->getActionStateValid(controller::Action::MOUTHSMILE_L) ||
            userInputMapper->getActionStateValid(controller::Action::MOUTHSMILE_R);

        MyAvatar* myAvatar = static_cast<MyAvatar*>(_owningAvatar);
        bool eyesTracked =
            myAvatar->getControllerPoseInSensorFrame(controller::Action::LEFT_EYE).valid &&
            myAvatar->getControllerPoseInSensorFrame(controller::Action::RIGHT_EYE).valid;

        int leftEyeJointIndex = myAvatar->getJointIndex("LeftEye");
        int rightEyeJointIndex = myAvatar->getJointIndex("RightEye");
        bool eyeJointsOverridden = myAvatar->getIsJointOverridden(leftEyeJointIndex) || myAvatar->getIsJointOverridden(rightEyeJointIndex);

        bool anyInputTracked = false;
        for (int i = 0; i < (int)Blendshapes::BlendshapeCount; i++) {
            anyInputTracked = anyInputTracked || userInputMapper->getActionStateValid(blendshapeActions[i]);
        }

        setHasInputDrivenBlendshapes(anyInputTracked);

        // suppress any procedural blendshape animation if they overlap with driven input.
        setSuppressProceduralAnimationFlag(HeadData::BlinkProceduralBlendshapeAnimation, eyeLidsTracked);
        setSuppressProceduralAnimationFlag(HeadData::LidAdjustmentProceduralBlendshapeAnimation, eyeLidsTracked || browsTracked);
        setSuppressProceduralAnimationFlag(HeadData::AudioProceduralBlendshapeAnimation, mouthTracked);
        setSuppressProceduralAnimationFlag(HeadData::SaccadeProceduralEyeJointAnimation, eyesTracked || eyeJointsOverridden);

        if (anyInputTracked) {
            for (int i = 0; i < (int)Blendshapes::BlendshapeCount; i++) {
                _blendshapeCoefficients[i] = userInputMapper->getActionState(blendshapeActions[i]);
            }
        }
    }
    Parent::simulate(deltaTime);
}
