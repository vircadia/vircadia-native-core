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

bool OverlayConductor::shouldCenterUI() const {

    glm::mat4 hmdMat = qApp->getHMDSensorPose();
    glm::vec3 hmdPos = extractTranslation(hmdMat);
    glm::vec3 hmdForward = transformVectorFast(hmdMat, glm::vec3(0.0f, 0.0f, -1.0f));

    Transform uiTransform = qApp->getApplicationCompositor().getModelTransform();
    glm::vec3 uiPos = uiTransform.getTranslation();
    glm::vec3 uiForward = uiTransform.getRotation() * glm::vec3(0.0f, 0.0f, -1.0f);

    const float MAX_COMPOSITOR_DISTANCE = 0.6f;
    const float MAX_COMPOSITOR_ANGLE = 180.0f;  // rotation check is effectively disabled
    if (glm::distance(uiPos, hmdPos) > MAX_COMPOSITOR_DISTANCE ||
        glm::dot(uiForward, hmdForward) < cosf(glm::radians(MAX_COMPOSITOR_ANGLE))) {
        return true;
    }
    return false;
}

void OverlayConductor::update(float dt) {

    updateMode();

    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

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

        const quint64 REQUIRED_USECS_IN_NEW_MODE_BEFORE_INVISIBLE = 200 * 1000;
        const quint64 REQUIRED_USECS_IN_NEW_MODE_BEFORE_VISIBLE = 1000 * 1000;

        // fade in or out the overlay, based on driving.
        bool nowDriving = myAvatar->hasDriveInput();
        // Check that we're in this new mode for long enough to really trigger a transition.
        if (nowDriving == _driving) {  // If there's no change in state, clear any attepted timer.
            _timeInPotentialMode = 0;
        } else if (_timeInPotentialMode == 0) {  // We've just changed with no timer, so start timing now.
            _timeInPotentialMode = usecTimestampNow();
        } else if ((usecTimestampNow() - _timeInPotentialMode) > (nowDriving ? REQUIRED_USECS_IN_NEW_MODE_BEFORE_INVISIBLE : REQUIRED_USECS_IN_NEW_MODE_BEFORE_VISIBLE)) {
            _timeInPotentialMode = 0; // a real transition
            bool wantsOverlays = Menu::getInstance()->isOptionChecked(MenuOption::Overlays);
            setEnabled(!nowDriving && wantsOverlays);
            _driving = nowDriving;
        }

        // center the UI
        if (shouldCenterUI()) {
            Transform hmdTransform(cancelOutRollAndPitch(qApp->getHMDSensorPose()));
            qApp->getApplicationCompositor().setModelTransform(hmdTransform);
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

    Mode newMode;
    if (qApp->isHMDMode()) {
        if (myAvatar->getClearOverlayWhenDriving()) {
            newMode = STANDING;
        } else {
            newMode = SITTING;
        }
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
        case STANDING: {
            // enter the STANDING state
            Transform hmdTransform(cancelOutRollAndPitch(qApp->getHMDSensorPose()));
            qApp->getApplicationCompositor().setModelTransform(hmdTransform);
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

