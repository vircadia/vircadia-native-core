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
#include <trackers/FaceTracker.h>
#include <trackers/EyeTracker.h>

#include "devices/DdeFaceTracker.h"
#include "Application.h"
#include "MyAvatar.h"

using namespace std;

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

    return myAvatar->getOrientation() * glm::quat(glm::radians(glm::vec3(_basePitch, 0.0f, 0.0f)));
}

void MyHead::simulate(float deltaTime) {
    auto player = DependencyManager::get<recording::Deck>();
    // Only use face trackers when not playing back a recording.
    if (!player->isPlaying()) {
        FaceTracker* faceTracker = qApp->getActiveFaceTracker();
        _isFaceTrackerConnected = faceTracker != nullptr && !faceTracker->isMuted();
        if (_isFaceTrackerConnected) {
            _transientBlendshapeCoefficients = faceTracker->getBlendshapeCoefficients();

            if (typeid(*faceTracker) == typeid(DdeFaceTracker)) {

                if (Menu::getInstance()->isOptionChecked(MenuOption::UseAudioForMouth)) {
                    calculateMouthShapes(deltaTime);

                    const int JAW_OPEN_BLENDSHAPE = 21;
                    const int MMMM_BLENDSHAPE = 34;
                    const int FUNNEL_BLENDSHAPE = 40;
                    const int SMILE_LEFT_BLENDSHAPE = 28;
                    const int SMILE_RIGHT_BLENDSHAPE = 29;
                    _transientBlendshapeCoefficients[JAW_OPEN_BLENDSHAPE] += _audioJawOpen;
                    _transientBlendshapeCoefficients[SMILE_LEFT_BLENDSHAPE] += _mouth4;
                    _transientBlendshapeCoefficients[SMILE_RIGHT_BLENDSHAPE] += _mouth4;
                    _transientBlendshapeCoefficients[MMMM_BLENDSHAPE] += _mouth2;
                    _transientBlendshapeCoefficients[FUNNEL_BLENDSHAPE] += _mouth3;
                }
                applyEyelidOffset(getFinalOrientationInWorldFrame());
            }
        }
        auto eyeTracker = DependencyManager::get<EyeTracker>();
        _isEyeTrackerConnected = eyeTracker->isTracking();
    }
    Parent::simulate(deltaTime);
}
