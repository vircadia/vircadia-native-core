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

bool OverlayConductor::headOutsideOverlay() const {
    glm::mat4 hmdMat = qApp->getHMDSensorPose();
    glm::vec3 hmdPos = extractTranslation(hmdMat);
    glm::vec3 hmdForward = transformVectorFast(hmdMat, glm::vec3(0.0f, 0.0f, -1.0f));

    Transform uiTransform = qApp->getApplicationCompositor().getModelTransform();
    glm::vec3 uiPos = uiTransform.getTranslation();
    glm::vec3 uiForward = uiTransform.getRotation() * glm::vec3(0.0f, 0.0f, -1.0f);

    const float MAX_COMPOSITOR_DISTANCE = 0.99f; // If you're 1m from center of ui sphere, you're at the surface.
    const float MAX_COMPOSITOR_ANGLE = 180.0f;  // rotation check is effectively disabled
    if (glm::distance(uiPos, hmdPos) > MAX_COMPOSITOR_DISTANCE ||
        glm::dot(uiForward, hmdForward) < cosf(glm::radians(MAX_COMPOSITOR_ANGLE))) {
        return true;
    }
    return false;
}

bool OverlayConductor::updateAvatarIsAtRest() {

    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    const quint64 REST_ENABLE_TIME_USECS = 1000 * 1000; // 1 s
    const quint64 REST_DISABLE_TIME_USECS = 200 * 1000;  // 200 ms

    const float AT_REST_THRESHOLD = 0.01f;
    bool desiredAtRest = glm::length(myAvatar->getVelocity()) < AT_REST_THRESHOLD;
    if (desiredAtRest != _desiredAtRest) {
        // start timer
        _desiredAtRestTimer = usecTimestampNow() + (desiredAtRest ? REST_ENABLE_TIME_USECS : REST_DISABLE_TIME_USECS);
    }

    _desiredAtRest = desiredAtRest;

    if (_desiredAtRestTimer != 0 && usecTimestampNow() > _desiredAtRestTimer) {
        // timer expired
        // change state!
        _currentAtRest = _desiredAtRest;
        // disable timer
        _desiredAtRestTimer = 0;
    }

    return _currentAtRest;
}

bool OverlayConductor::updateAvatarHasDriveInput() {
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    const quint64 DRIVE_ENABLE_TIME_USECS = 200 * 1000;  // 200 ms
    const quint64 DRIVE_DISABLE_TIME_USECS = 1000 * 1000; // 1 s

    bool desiredDriving = myAvatar->hasDriveInput();
    if (desiredDriving != _desiredDriving) {
        // start timer
        _desiredDrivingTimer = usecTimestampNow() + (desiredDriving ? DRIVE_ENABLE_TIME_USECS : DRIVE_DISABLE_TIME_USECS);
    }

    _desiredDriving = desiredDriving;

    if (_desiredDrivingTimer != 0 && usecTimestampNow() > _desiredDrivingTimer) {
        // timer expired
        // change state!
        _currentDriving = _desiredDriving;
        // disable timer
        _desiredDrivingTimer = 0;
    }

    return _currentDriving;
}

void OverlayConductor::centerUI() {
    // place the overlay at the current hmd position in sensor space
    auto camMat = cancelOutRollAndPitch(qApp->getHMDSensorPose());
    qApp->getApplicationCompositor().setModelTransform(Transform(camMat));
}

void OverlayConductor::update(float dt) {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    bool currentVisible = !offscreenUi->getDesktop()->property("pinned").toBool();

    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    // centerUI when hmd mode is first enabled and mounted
    if (qApp->isHMDMode() && qApp->getActiveDisplayPlugin()->isDisplayVisible()) {
        if (!_hmdMode) {
            _hmdMode = true;
            centerUI();
        }
    } else {
        _hmdMode = false;
    }

    bool prevDriving = _currentDriving;
    bool isDriving = updateAvatarHasDriveInput();
    bool drivingChanged = prevDriving != isDriving;
    bool isAtRest = updateAvatarIsAtRest();
    bool shouldRecenter = false;

    if (_flags & SuppressedByDrive) {
        if (!isDriving) {
            _flags &= ~SuppressedByDrive;
             shouldRecenter = true;
        }
    } else {
        if (myAvatar->getClearOverlayWhenMoving() && drivingChanged && isDriving) {
            _flags |= SuppressedByDrive;
        }
    }

    if (_flags & SuppressedByHead) {
        if (isAtRest) {
            _flags &= ~SuppressedByHead;
            shouldRecenter = true;
        }
    } else {
        if (_hmdMode && headOutsideOverlay()) {
            _flags |= SuppressedByHead;
        }
    }


    bool targetVisible = Menu::getInstance()->isOptionChecked(MenuOption::Overlays) && (0 == (_flags & SuppressMask));
    if (targetVisible != currentVisible) {
        offscreenUi->setPinned(!targetVisible);
    }
    if (shouldRecenter && !_flags) {
        centerUI();
    }
}
