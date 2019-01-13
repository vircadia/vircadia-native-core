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
#include "DetailedMotionState.h"

static glm::u8vec3 getLoadingOrbColor(Avatar::LoadingStatus loadingStatus) {

    const glm::u8vec3 NO_MODEL_COLOR(0xe3, 0xe3, 0xe3);
    const glm::u8vec3 LOAD_MODEL_COLOR(0xef, 0x93, 0xd1);
    const glm::u8vec3 LOAD_SUCCESS_COLOR(0x1f, 0xc6, 0xa6);
    const glm::u8vec3 LOAD_FAILURE_COLOR(0xc6, 0x21, 0x47);
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

btCollisionShape* OtherAvatar::createDetailedCollisionShapeForJoint(int jointIndex) {
    ShapeInfo shapeInfo;
    computeDetailedShapeInfo(shapeInfo, jointIndex);
    if (shapeInfo.getType() != SHAPE_TYPE_NONE) {
        btCollisionShape* shape = const_cast<btCollisionShape*>(ObjectMotionState::getShapeManager()->getShape(shapeInfo));
        return shape;
    }
    return nullptr;
}

btCollisionShape* OtherAvatar::createCapsuleCollisionShape() {
    ShapeInfo shapeInfo;
    computeShapeInfo(shapeInfo);
    shapeInfo.setOffset(glm::vec3(0.0f));
    if (shapeInfo.getType() != SHAPE_TYPE_NONE) {
        btCollisionShape* shape = const_cast<btCollisionShape*>(ObjectMotionState::getShapeManager()->getShape(shapeInfo));
        return shape;
    }
    return nullptr;
}

DetailedMotionState* OtherAvatar::createDetailedMotionStateForJoint(std::shared_ptr<OtherAvatar> avatar, int jointIndex) {
    auto shape = createDetailedCollisionShapeForJoint(jointIndex);
    if (shape) {
        DetailedMotionState* motionState = new DetailedMotionState(avatar, shape, jointIndex);
        motionState->setMass(computeMass());
        return motionState;
    }
    return nullptr;
}

DetailedMotionState* OtherAvatar::createCapsuleMotionState(std::shared_ptr<OtherAvatar> avatar) {
    auto shape = createCapsuleCollisionShape();
    if (shape) {
        DetailedMotionState* motionState = new DetailedMotionState(avatar, shape, -1);
        motionState->setIsAvatarCapsule(true);
        motionState->setMass(computeMass());
        return motionState;
    }
    return nullptr;
}

void OtherAvatar::resetDetailedMotionStates() {
    for (size_t i = 0; i < _detailedMotionStates.size(); i++) {
        _detailedMotionStates[i] = nullptr;
    }
    _detailedMotionStates.clear();
}

void OtherAvatar::setWorkloadRegion(uint8_t region) {
    _workloadRegion = region;
    QString printRegion = "";
    if (region == workload::Region::R1) {
        printRegion = "R1";
    } else if (region == workload::Region::R2) {
        printRegion = "R2";
    } else if (region == workload::Region::R3) {
        printRegion = "R3";
    } else {
        printRegion = "invalid";
    }
    qDebug() << "Setting workload region to " << printRegion;
    computeShapeLOD();
}

void OtherAvatar::computeShapeLOD() {
    auto newBodyLOD = (_workloadRegion < workload::Region::R3 && !isDead()) ? BodyLOD::MultiSphereShapes : BodyLOD::CapsuleShape;
    if (newBodyLOD != _bodyLOD) {
        _bodyLOD = newBodyLOD;
        if (isInPhysicsSimulation()) {
            qDebug() << "Changing to body LOD " << (_bodyLOD == BodyLOD::MultiSphereShapes ? "MultiSpheres" : "Capsule");
            _needsReinsertion = true;
        }
    }
}

bool OtherAvatar::isInPhysicsSimulation() const {
    return _motionState != nullptr && _detailedMotionStates.size() > 0;
}

bool OtherAvatar::shouldBeInPhysicsSimulation() const {
    return !(isInPhysicsSimulation() && _needsReinsertion);
}

bool OtherAvatar::needsPhysicsUpdate() const {
    constexpr uint32_t FLAGS_OF_INTEREST = Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS | Simulation::DIRTY_POSITION | Simulation::DIRTY_COLLISION_GROUP;
    return (_needsReinsertion || _motionState && (bool)(_motionState->getIncomingDirtyFlags() & FLAGS_OF_INTEREST));
}

void OtherAvatar::rebuildCollisionShape() {
    if (_motionState) {
        _motionState->addDirtyFlags(Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS);
    }
    for (size_t i = 0; i < _detailedMotionStates.size(); i++) {
        if (_detailedMotionStates[i]) {
            _detailedMotionStates[i]->addDirtyFlags(Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS);
        }
    }
}

void OtherAvatar::updateCollisionGroup(bool myAvatarCollide) {
    if (_motionState) {
        bool collides = _motionState->getCollisionGroup() == BULLET_COLLISION_GROUP_OTHER_AVATAR && myAvatarCollide;
        if (_collideWithOtherAvatars != collides) {
            if (!myAvatarCollide) {
                _collideWithOtherAvatars = false;
            }
            auto newCollisionGroup = _collideWithOtherAvatars ? BULLET_COLLISION_GROUP_OTHER_AVATAR : BULLET_COLLISION_GROUP_COLLISIONLESS;
            _motionState->setCollisionGroup(newCollisionGroup);
            _motionState->addDirtyFlags(Simulation::DIRTY_COLLISION_GROUP);
        }
    }
}

void OtherAvatar::createDetailedMotionStates(const std::shared_ptr<OtherAvatar>& avatar){
    auto& detailedMotionStates = getDetailedMotionStates();
    if (_bodyLOD == BodyLOD::MultiSphereShapes) {
        for (int i = 0; i < getJointCount(); i++) {
            auto dMotionState = createDetailedMotionStateForJoint(avatar, i);
            if (dMotionState) {
                detailedMotionStates.push_back(dMotionState);
            }
        }
    } else if (_bodyLOD == BodyLOD::CapsuleShape) {
        auto dMotionState = createCapsuleMotionState(avatar);
        if (dMotionState) {
            detailedMotionStates.push_back(dMotionState);
        }
    }
    _needsReinsertion = false;
}
