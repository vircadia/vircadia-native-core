//
//  OverlayConductor.cpp
//  interface/src/ui
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <OffscreenUi.h>
#include <display-plugins/CompositorHelper.h>

#include "Application.h"
#include "avatar/AvatarManager.h"
#include "InterfaceLogging.h"
#include "OverlayConductor.h"

OverlayConductor::OverlayConductor() {
}

OverlayConductor::~OverlayConductor() {
}

void OverlayConductor::update(float dt) {

    updateMode();

    switch (_mode) {
    case SITTING: {
        // when sitting, the overlay is at the origin, facing down the -z axis.
        // the camera is taken directly from the HMD.
        Transform identity;
        qApp->getApplicationCompositor().setModelTransform(identity);
        qApp->getApplicationCompositor().setCameraBaseTransform(identity);
        break;
    }
    case STANDING: {
        // when standing, the overlay is at a reference position, which is set when the overlay is
        // enabled.  The camera is taken directly from the HMD, but in world space.
        // So the sensorToWorldMatrix must be applied.
        MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        Transform t;
        t.evalFromRawMatrix(myAvatar->getSensorToWorldMatrix());
        qApp->getApplicationCompositor().setCameraBaseTransform(t);

        // detect when head moves out side of sweet spot, or looks away.
        mat4 headMat = myAvatar->getSensorToWorldMatrix() * qApp->getHMDSensorPose();
        vec3 headWorldPos = extractTranslation(headMat);
        vec3 headForward = glm::quat_cast(headMat) * glm::vec3(0.0f, 0.0f, -1.0f);
        Transform modelXform = qApp->getApplicationCompositor().getModelTransform();
        vec3 compositorWorldPos = modelXform.getTranslation();
        vec3 compositorForward = modelXform.getRotation() * glm::vec3(0.0f, 0.0f, -1.0f);
        const float MAX_COMPOSITOR_DISTANCE = 0.6f;
        const float MAX_COMPOSITOR_ANGLE = 110.0f;
        if (_enabled && (glm::distance(headWorldPos, compositorWorldPos) > MAX_COMPOSITOR_DISTANCE ||
                         glm::dot(headForward, compositorForward) < cosf(glm::radians(MAX_COMPOSITOR_ANGLE)))) {
            // fade out the overlay
            setEnabled(false);
        }
        break;
    }
    case FLAT:
        // do nothing
        break;
    }
}

void OverlayConductor::updateMode() {
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    if (myAvatar->getClearOverlayWhenDriving()) {
        float speed = glm::length(myAvatar->getVelocity());
        const float MIN_DRIVING = 0.2f;
        const float MAX_NOT_DRIVING = 0.01f;
        const quint64 REQUIRED_USECS_IN_NEW_MODE_BEFORE_INVISIBLE = 200 * 1000;
        const quint64 REQUIRED_USECS_IN_NEW_MODE_BEFORE_VISIBLE = 1000 * 1000;
        bool nowDriving = _driving; // Assume current _driving mode unless...
        if (speed > MIN_DRIVING) {  // ... we're definitely moving...
            nowDriving = true;
        } else if (speed < MAX_NOT_DRIVING) { // ... or definitely not.
            nowDriving = false;
        }
        // Check that we're in this new mode for long enough to really trigger a transition.
        if (nowDriving == _driving) {  // If there's no change in state, clear any attepted timer.
            _timeInPotentialMode = 0;
        } else if (_timeInPotentialMode == 0) { // We've just changed with no timer, so start timing now.
            _timeInPotentialMode = usecTimestampNow();
        } else if ((usecTimestampNow() - _timeInPotentialMode) > (nowDriving ? REQUIRED_USECS_IN_NEW_MODE_BEFORE_INVISIBLE : REQUIRED_USECS_IN_NEW_MODE_BEFORE_VISIBLE)) {
            _timeInPotentialMode = 0; // a real transition
            if (nowDriving) {
                _wantsOverlays = Menu::getInstance()->isOptionChecked(MenuOption::Overlays);
            } else { // reset when coming out of driving
                _mode = FLAT;  // Seems appropriate to let things reset, below, after the following.
                // All reset of, e.g., room-scale location as though by apostrophe key, without all the other adjustments.
                qApp->getActiveDisplayPlugin()->resetSensors();
                myAvatar->reset(true, false, false);
            }
            if (_wantsOverlays) {
                setEnabled(!nowDriving);
            }
            _driving = nowDriving;
        } // Else haven't accumulated enough time in new mode, but keep timing.
    }

    Mode newMode;
    if (qApp->isHMDMode()) {
        newMode = SITTING;
    } else {
        newMode = FLAT;
    }

    if (newMode != _mode) {
        switch (newMode) {
        case SITTING: {
            // enter the SITTING state
            // place the overlay at origin
            qApp->getApplicationCompositor().setModelTransform(Transform());
            break;
        }
        case STANDING: {  // STANDING mode is not currently used.
            // enter the STANDING state
            // place the overlay at the current hmd position in world space
            auto camMat = cancelOutRollAndPitch(myAvatar->getSensorToWorldMatrix() * qApp->getHMDSensorPose());
            qApp->getApplicationCompositor().setModelTransform(Transform(camMat));
            break;
        }

        case FLAT:
            // do nothing
            break;
        }
    }

    _mode = newMode;

}

void OverlayConductor::setEnabled(bool enabled) {
    if (enabled == _enabled) {
        return;
    }

    _enabled = enabled; // set the new value
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->setPinned(!_enabled);
    // if the new state is visible/enabled...
    if (_enabled && _mode == STANDING) {
        // place the overlay at the current hmd position in world space
        MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        auto camMat = cancelOutRollAndPitch(myAvatar->getSensorToWorldMatrix() * qApp->getHMDSensorPose());
        qApp->getApplicationCompositor().setModelTransform(Transform(camMat));
    } 
}

bool OverlayConductor::getEnabled() const {
    return _enabled;
}

