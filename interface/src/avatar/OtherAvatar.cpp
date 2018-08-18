//
//  Created by Bradley Austin Davis on 2017/04/27
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OtherAvatar.h"
#include "Application.h"

#include "AvatarMotionState.h"

OtherAvatar::OtherAvatar(QThread* thread) : Avatar(thread) {
    // give the pointer to our head to inherited _headData variable from AvatarData
    _headData = new Head(this);
    _skeletonModel = std::make_shared<SkeletonModel>(this, nullptr);
    _skeletonModel->setLoadingPriority(OTHERAVATAR_LOADING_PRIORITY);
    connect(_skeletonModel.get(), &Model::setURLFinished, this, &Avatar::setModelURLFinished);
    connect(_skeletonModel.get(), &Model::rigReady, this, &Avatar::rigReady);
    connect(_skeletonModel.get(), &Model::rigReset, this, &Avatar::rigReset);

    // add the purple orb
    createOrb();
}

OtherAvatar::~OtherAvatar() {
    removeOrb();
}

void OtherAvatar::removeOrb() {
    if (qApp->getOverlays().isAddedOverlay(_otherAvatarOrbMeshPlaceholderID)) {
        qApp->getOverlays().deleteOverlay(_otherAvatarOrbMeshPlaceholderID);
    }
}

void OtherAvatar::updateOrbPosition() {
    if (_otherAvatarOrbMeshPlaceholder != nullptr) {
        _otherAvatarOrbMeshPlaceholder->setWorldPosition(getHead()->getPosition());
    }
}

void OtherAvatar::createOrb() {
    if (_otherAvatarOrbMeshPlaceholderID == UNKNOWN_OVERLAY_ID ||
        !qApp->getOverlays().isAddedOverlay(_otherAvatarOrbMeshPlaceholderID)) {
        _otherAvatarOrbMeshPlaceholder = std::make_shared<Sphere3DOverlay>();
        _otherAvatarOrbMeshPlaceholder->setAlpha(1.0f);
        _otherAvatarOrbMeshPlaceholder->setColor({ 0xFF, 0x00, 0xFF });
        _otherAvatarOrbMeshPlaceholder->setIsSolid(false);
        _otherAvatarOrbMeshPlaceholder->setPulseMin(0.5);
        _otherAvatarOrbMeshPlaceholder->setPulseMax(1.0);
        _otherAvatarOrbMeshPlaceholder->setColorPulse(1.0);
        _otherAvatarOrbMeshPlaceholder->setIgnorePickIntersection(true);
        _otherAvatarOrbMeshPlaceholder->setDrawInFront(false);
        _otherAvatarOrbMeshPlaceholderID = qApp->getOverlays().addOverlay(_otherAvatarOrbMeshPlaceholder);
        // Position focus
        _otherAvatarOrbMeshPlaceholder->setWorldOrientation(glm::quat(0.0f, 0.0f, 0.0f, 1.0));
        _otherAvatarOrbMeshPlaceholder->setWorldPosition(getHead()->getPosition());
        _otherAvatarOrbMeshPlaceholder->setDimensions(glm::vec3(0.5f, 0.5f, 0.5f));
        _otherAvatarOrbMeshPlaceholder->setVisible(true);
    }
}

void OtherAvatar::setSpaceIndex(int32_t index) {
    assert(_spaceIndex == -1);
    _spaceIndex = index;
}

void OtherAvatar::updateSpaceProxy(workload::Transaction& transaction) const {
    if (_spaceIndex > -1) {
        float approximateBoundingRadius = glm::length(getTargetScale());
        workload::Sphere sphere(getWorldPosition(), approximateBoundingRadius);
        transaction.update(_spaceIndex, sphere);
    }
}

int OtherAvatar::parseDataFromBuffer(const QByteArray& buffer) {
    int32_t bytesRead = Avatar::parseDataFromBuffer(buffer);
    if (_moving && _motionState) {
        _motionState->addDirtyFlags(Simulation::DIRTY_POSITION);
    }
    return bytesRead;
}

void OtherAvatar::setWorkloadRegion(uint8_t region) {
    _workloadRegion = region;
}

bool OtherAvatar::shouldBeInPhysicsSimulation() const {
    return (_workloadRegion < workload::Region::R3 && !isDead());
}

void OtherAvatar::rebuildCollisionShape() {
    if (_motionState) {
        _motionState->addDirtyFlags(Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS);
    }
}
