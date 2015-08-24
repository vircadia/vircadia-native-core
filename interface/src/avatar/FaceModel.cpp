//
//  FaceModel.cpp
//  interface/src/avatar
//
//  Created by Andrzej Kapolka on 9/16/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/transform.hpp>

#include "Avatar.h"
#include "FaceModel.h"
#include "Head.h"
#include "Menu.h"

FaceModel::FaceModel(Head* owningHead, RigPointer rig) :
    Model(rig, nullptr),
    _owningHead(owningHead)
{
    assert(_rig);
}

void FaceModel::simulate(float deltaTime, bool fullUpdate) {
    updateGeometry();

    Avatar* owningAvatar = static_cast<Avatar*>(_owningHead->_owningAvatar);
    glm::vec3 neckPosition;
    if (!owningAvatar->getSkeletonModel().getNeckPosition(neckPosition)) {
        neckPosition = owningAvatar->getPosition();
    }
    setTranslation(neckPosition);
    glm::quat neckParentRotation;
    if (!owningAvatar->getSkeletonModel().getNeckParentRotationFromDefaultOrientation(neckParentRotation)) {
        neckParentRotation = owningAvatar->getOrientation();
    }
    setRotation(neckParentRotation);
    setScale(glm::vec3(1.0f, 1.0f, 1.0f) * _owningHead->getScale());

    setPupilDilation(_owningHead->getPupilDilation());
    setBlendshapeCoefficients(_owningHead->getBlendshapeCoefficients());

    // FIXME - this is very expensive, we shouldn't do it if we don't have to
    //invalidCalculatedMeshBoxes();

    if (isActive()) {
        setOffset(-_geometry->getFBXGeometry().neckPivot);
        Model::simulateInternal(deltaTime);
    }
}

bool FaceModel::getEyePositions(glm::vec3& firstEyePosition, glm::vec3& secondEyePosition) const {
    if (!isActive()) {
        return false;
    }
    const FBXGeometry& geometry = _geometry->getFBXGeometry();
    return getJointPositionInWorldFrame(geometry.leftEyeJointIndex, firstEyePosition) &&
        getJointPositionInWorldFrame(geometry.rightEyeJointIndex, secondEyePosition);
}
