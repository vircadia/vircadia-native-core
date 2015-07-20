//
//  OverlayConductor.cpp
//  interface/src/ui
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"
#include "avatar/AvatarManager.h"

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
        Transform t;
        t.evalFromRawMatrix(qApp->getHMDSensorPose());
        qApp->getApplicationCompositor().setCameraTransform(t);
        break;
    }
    case STANDING: {
        // when standing, the overlay is at a reference position, which is set when the overlay is
        // enabled.  The camera is taken directly from the HMD in world space.
        MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        Transform t;
        t.evalFromRawMatrix(myAvatar->getSensorToWorldMatrix() * qApp->getHMDSensorPose());
        qApp->getApplicationCompositor().setCameraTransform(t);
        break;
    }
    case FLAT:
        // do nothing
        break;
    }

    // TODO: detect when head moves out side of sweet spot.
    // TODO: set reference position when HMD is on, etc are changed.

    // process alpha fade animations
    qApp->getApplicationCompositor().update(dt);
}

void OverlayConductor::updateMode() {

    Mode newMode;
    if (qApp->isHMDMode()) {
        MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
        if (myAvatar->getStandingHMDSensorMode()) {
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
            Transform identity;
            qApp->getApplicationCompositor().setModelTransform(identity);
            break;
        }
        case STANDING: {
            // enter the STANDING state
            // place the overlay at the current hmd position in world space
            MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
            auto camMat = cancelOutRollAndPitch(myAvatar->getSensorToWorldMatrix() * qApp->getHMDSensorPose());
            Transform t;
            t.setTranslation(extractTranslation(camMat));
            t.setRotation(glm::quat_cast(camMat));
            qApp->getApplicationCompositor().setModelTransform(t);
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

    if (_enabled) {
        // alpha fadeOut the overlay mesh.
        qApp->getApplicationCompositor().fadeOut();

        // disable mouse clicks from script
        qApp->getOverlays().disable();

        // disable QML events
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        offscreenUi->getRootItem()->setEnabled(false);

        _enabled = false;
    } else {
        // alpha fadeIn the overlay mesh.
        qApp->getApplicationCompositor().fadeIn();

        // enable mouse clicks from script
        qApp->getOverlays().enable();

        // enable QML events
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        offscreenUi->getRootItem()->setEnabled(true);

        if (_mode == STANDING) {
            // place the overlay at the current hmd position in world space
            MyAvatar* myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
            auto camMat = cancelOutRollAndPitch(myAvatar->getSensorToWorldMatrix() * qApp->getHMDSensorPose());
            Transform t;
            t.setTranslation(extractTranslation(camMat));
            t.setRotation(glm::quat_cast(camMat));
            qApp->getApplicationCompositor().setModelTransform(t);
        }

        _enabled = true;
    }
}

bool OverlayConductor::getEnabled() const {
    return _enabled;
}

