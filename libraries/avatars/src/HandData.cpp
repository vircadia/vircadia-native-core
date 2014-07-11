//
//  HandData.cpp
//  libraries/avatars/src
//
//  Created by Stephen Birarda on 5/20/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDataStream>

#include <GeometryUtil.h>
#include <SharedUtil.h>

#include "AvatarData.h" 
#include "HandData.h"


HandData::HandData(AvatarData* owningAvatar) :
    _owningAvatarData(owningAvatar)
{
    // Start with two palms
    addNewPalm();
    addNewPalm();
}

glm::vec3 HandData::worldToLocalVector(const glm::vec3& worldVector) const {
    return glm::inverse(getBaseOrientation()) * worldVector;
}

PalmData& HandData::addNewPalm()  {
    _palms.push_back(PalmData(this));
    return _palms.back();
}

const PalmData* HandData::getPalm(int sixSenseID) const {
    // the palms are not necessarily added in left-right order, 
    // so we have to search for the right SixSenseID
    for (unsigned int i = 0; i < _palms.size(); i++) {
        const PalmData* palm = &(_palms[i]);
        if (palm->getSixenseID() == sixSenseID) {
            return palm->isActive() ? palm : NULL;
        }
    }
    return NULL;
}

void HandData::getLeftRightPalmIndices(int& leftPalmIndex, int& rightPalmIndex) const {
    leftPalmIndex = -1;
    rightPalmIndex = -1;
    for (size_t i = 0; i < _palms.size(); i++) {
        const PalmData& palm = _palms[i];
        if (palm.isActive()) {
            if (palm.getSixenseID() == LEFT_HAND_INDEX) {
                leftPalmIndex = i;
            }
            if (palm.getSixenseID() == RIGHT_HAND_INDEX) {
                rightPalmIndex = i;
            }
        }
    }
}

PalmData::PalmData(HandData* owningHandData) :
_rawRotation(0.f, 0.f, 0.f, 1.f),
_rawPosition(0.f),
_rawVelocity(0.f),
_rotationalVelocity(0.f),
_totalPenetration(0.f),
_controllerButtons(0),
_isActive(false),
_sixenseID(SIXENSEID_INVALID),
_numFramesWithoutData(0),
_owningHandData(owningHandData),
_isCollidingWithVoxel(false),
_isCollidingWithPalm(false),
_collisionlessPaddleExpiry(0) {
}

void PalmData::addToPosition(const glm::vec3& delta) {
    _rawPosition += _owningHandData->worldToLocalVector(delta);
}

bool HandData::findSpherePenetration(const glm::vec3& penetratorCenter, float penetratorRadius, glm::vec3& penetration, 
                                        const PalmData*& collidingPalm) const {
    
    for (size_t i = 0; i < _palms.size(); ++i) {
        const PalmData& palm = _palms[i];
        if (!palm.isActive()) {
            continue;
        }
        glm::vec3 palmPosition = palm.getPosition();
        const float PALM_RADIUS = 0.05f; // in world (not voxel) coordinates
        if (findSphereSpherePenetration(penetratorCenter, penetratorRadius, palmPosition, PALM_RADIUS, penetration)) {
            collidingPalm = &palm;
            return true;
        }
    }
    return false;
}

glm::quat HandData::getBaseOrientation() const {
    return _owningAvatarData->getOrientation();
}

glm::vec3 HandData::getBasePosition() const {
    return _owningAvatarData->getPosition();
}
        
glm::vec3 PalmData::getFingerDirection() const {
    const glm::vec3 LOCAL_FINGER_DIRECTION(0.0f, 0.0f, 1.0f);
    return _owningHandData->localToWorldDirection(_rawRotation * LOCAL_FINGER_DIRECTION);
}

glm::vec3 PalmData::getNormal() const {
    const glm::vec3 LOCAL_PALM_DIRECTION(0.0f, -1.0f, 0.0f);
    return _owningHandData->localToWorldDirection(_rawRotation * LOCAL_PALM_DIRECTION);
}



