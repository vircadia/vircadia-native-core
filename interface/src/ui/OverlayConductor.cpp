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

    const float MAX_COMPOSITOR_DISTANCE = 0.6f;
    const float MAX_COMPOSITOR_ANGLE = 180.0f;  // rotation check is effectively disabled
    if (glm::distance(uiPos, hmdPos) > MAX_COMPOSITOR_DISTANCE ||
        glm::dot(uiForward, hmdForward) < cosf(glm::radians(MAX_COMPOSITOR_ANGLE))) {
        return true;
    }
    return false;
}

bool OverlayConductor::avatarHasDriveInput() const {
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

bool OverlayConductor::shouldShowOverlay() const {
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

#ifdef WANT_DEBUG
    qDebug() << "AJT: wantsOverlays =" << Menu::getInstance()->isOptionChecked(MenuOption::Overlays) << ", clearOverlayWhenDriving =" << myAvatar->getClearOverlayWhenDriving() <<
        ", headOutsideOverlay =" << headOutsideOverlay() << ", hasDriveInput =" << avatarHasDriveInput();
#endif

    return Menu::getInstance()->isOptionChecked(MenuOption::Overlays) && (!myAvatar->getClearOverlayWhenDriving() || (!headOutsideOverlay() && !avatarHasDriveInput()));
}

bool OverlayConductor::shouldRecenterOnFadeOut() const {
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    return Menu::getInstance()->isOptionChecked(MenuOption::Overlays) && myAvatar->getClearOverlayWhenDriving() && headOutsideOverlay();
}

void OverlayConductor::centerUI() {
    // place the overlay at the current hmd position in sensor space
    auto camMat = cancelOutRollAndPitch(qApp->getHMDSensorPose());
    qApp->getApplicationCompositor().setModelTransform(Transform(camMat));
}

void OverlayConductor::update(float dt) {

    // centerUI if hmd mode changes
    if (qApp->isHMDMode() && !_hmdMode) {
        centerUI();
    }
    _hmdMode = qApp->isHMDMode();

    // centerUI if timer expires
    if (_fadeOutTime != 0 && usecTimestampNow() > _fadeOutTime) {
        // fade out timer expired
        _fadeOutTime = 0;
        centerUI();
    }

    bool showOverlay = shouldShowOverlay();

    if (showOverlay != getEnabled()) {
        if (showOverlay) {
            // disable fadeOut timer
            _fadeOutTime = 0;
        } else if (shouldRecenterOnFadeOut()) {
            // start fadeOut timer
            const quint64 FADE_OUT_TIME_USECS = 300 * 1000;  // 300 ms
            _fadeOutTime = usecTimestampNow() + FADE_OUT_TIME_USECS;
        }
    }

    setEnabled(showOverlay);
}

void OverlayConductor::setEnabled(bool enabled) {
    if (enabled == _enabled) {
        return;
    }

    _enabled = enabled; // set the new value
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->setPinned(!_enabled);
    // if the new state is visible/enabled...
    MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    if (_enabled && myAvatar->getClearOverlayWhenDriving() && qApp->isHMDMode()) {
        centerUI();
    }
}

bool OverlayConductor::getEnabled() const {
    return _enabled;
}

