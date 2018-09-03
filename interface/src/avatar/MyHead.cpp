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

    return myAvatar->getWorldOrientation() * glm::quat(glm::radians(glm::vec3(_basePitch, 0.0f, 0.0f)));
}

void MyHead::simulate(float deltaTime) {
    auto player = DependencyManager::get<recording::Deck>();
    // Only use face trackers when not playing back a recording.
    if (!player->isPlaying()) {
        auto faceTracker = qApp->getActiveFaceTracker();
        const bool hasActualFaceTrackerConnected = faceTracker && !faceTracker->isMuted();
        _isFaceTrackerConnected = hasActualFaceTrackerConnected || _owningAvatar->getHasScriptedBlendshapes();
        if (_isFaceTrackerConnected) {
            if (hasActualFaceTrackerConnected) {
                _blendshapeCoefficients = faceTracker->getBlendshapeCoefficients();
            }
        }

        auto eyeTracker = DependencyManager::get<EyeTracker>();
        _isEyeTrackerConnected = eyeTracker->isTracking();
        // if eye tracker is connected we should get the data here.
    }
    Parent::simulate(deltaTime);
}
