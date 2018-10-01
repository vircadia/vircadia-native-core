//
//  OverlayConductor.cpp
//  interface/src/ui
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OverlayConductor.h"

#include <OffscreenUi.h>
#include <display-plugins/CompositorHelper.h>

#include "Application.h"
#include "avatar/AvatarManager.h"
#include "InterfaceLogging.h"

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

    const float MAX_COMPOSITOR_DISTANCE = 0.99f;  // If you're 1m from center of ui sphere, you're at the surface.
    const float MAX_COMPOSITOR_ANGLE = 180.0f;    // rotation check is effectively disabled
    if (glm::distance(uiPos, hmdPos) > MAX_COMPOSITOR_DISTANCE ||
        glm::dot(uiForward, hmdForward) < cosf(glm::radians(MAX_COMPOSITOR_ANGLE))) {
        return true;
    }
    return false;
}

bool OverlayConductor::updateAvatarIsAtRest() {
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    const quint64 REST_ENABLE_TIME_USECS = 1000 * 1000;  // 1 s
    const quint64 REST_DISABLE_TIME_USECS = 200 * 1000;  // 200 ms

    const float AT_REST_THRESHOLD = 0.01f;
    bool desiredAtRest = glm::length(myAvatar->getWorldVelocity()) < AT_REST_THRESHOLD;
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

void OverlayConductor::centerUI() {
    // place the overlay at the current hmd position in sensor space
    auto camMat = cancelOutRollAndPitch(qApp->getHMDSensorPose());
    qApp->getApplicationCompositor().setModelTransform(Transform(camMat));
}

void OverlayConductor::update(float dt) {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    if (!offscreenUi) {
        return;
    }
    auto desktop = offscreenUi->getDesktop();
    if (!desktop) {
        return;
    }
    bool currentVisible = !desktop->property("pinned").toBool();

    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    // centerUI when hmd mode is first enabled and mounted
    if (qApp->isHMDMode() && qApp->getActiveDisplayPlugin()->isDisplayVisible()) {
        if (!_hmdMode) {
            _hmdMode = true;
            centerUI();
        }
    } else {
        _hmdMode = false;
    }

    bool shouldRecenter = false;

    if (_suppressedByHead) {
        if (updateAvatarIsAtRest()) {
            _suppressedByHead = false;
            shouldRecenter = true;
        }
    } else {
        if (_hmdMode && headOutsideOverlay()) {
            _suppressedByHead = true;
        }
    }

    bool targetVisible = Menu::getInstance()->isOptionChecked(MenuOption::Overlays) && !_suppressedByHead;
    if (targetVisible != currentVisible) {
        offscreenUi->setPinned(!targetVisible);
    }
    if (shouldRecenter && !_suppressedByHead) {
        centerUI();
    }
}
