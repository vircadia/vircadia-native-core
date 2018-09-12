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

static xColor getLoadingOrbColor(Avatar::LoadingStatus loadingStatus) {

    const xColor NO_MODEL_COLOR(0xe3, 0xe3, 0xe3);
    const xColor LOAD_MODEL_COLOR(0xef, 0x93, 0xd1);
    const xColor LOAD_SUCCESS_COLOR(0x1f, 0xc6, 0xa6);
    const xColor LOAD_FAILURE_COLOR(0xc6, 0x21, 0x47);
    switch (loadingStatus) {
    case Avatar::LoadingStatus::NoModel:
        return NO_MODEL_COLOR;
    case Avatar::LoadingStatus::LoadModel:
        return LOAD_MODEL_COLOR;
    case Avatar::LoadingStatus::LoadSuccess:
        return LOAD_SUCCESS_COLOR;
    case Avatar::LoadingStatus::LoadFailure:
    default:
        return LOAD_FAILURE_COLOR;
    }
}

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
    if (!_otherAvatarOrbMeshPlaceholderID.isNull()) {
        qApp->getOverlays().deleteOverlay(_otherAvatarOrbMeshPlaceholderID);
        _otherAvatarOrbMeshPlaceholderID = UNKNOWN_OVERLAY_ID;
    }
}

void OtherAvatar::updateOrbPosition() {
    if (_otherAvatarOrbMeshPlaceholder != nullptr) {
        _otherAvatarOrbMeshPlaceholder->setWorldPosition(getHead()->getPosition());
        if (_otherAvatarOrbMeshPlaceholderID.isNull()) {
            _otherAvatarOrbMeshPlaceholderID = qApp->getOverlays().addOverlay(_otherAvatarOrbMeshPlaceholder);
        }
    }
}

void OtherAvatar::createOrb() {
    if (_otherAvatarOrbMeshPlaceholderID.isNull()) {
        _otherAvatarOrbMeshPlaceholder = std::make_shared<Sphere3DOverlay>();
        _otherAvatarOrbMeshPlaceholder->setAlpha(1.0f);
        _otherAvatarOrbMeshPlaceholder->setColor(getLoadingOrbColor(_loadingStatus));
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

void OtherAvatar::indicateLoadingStatus(LoadingStatus loadingStatus) {
    Avatar::indicateLoadingStatus(loadingStatus);
    if (_otherAvatarOrbMeshPlaceholder) {
        _otherAvatarOrbMeshPlaceholder->setColor(getLoadingOrbColor(_loadingStatus));
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
