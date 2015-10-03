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
#include "InterfaceLogging.h"
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

