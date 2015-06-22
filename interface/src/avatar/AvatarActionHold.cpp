//
//  AvatarActionHold.cpp
//  interface/src/avatar/
//
//  Created by Seth Alves 2015-6-9
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "avatar/MyAvatar.h"
#include "avatar/AvatarManager.h"

#include "AvatarActionHold.h"

AvatarActionHold::AvatarActionHold(EntityActionType type, QUuid id, EntityItemPointer ownerEntity) :
    ObjectActionSpring(type, id, ownerEntity) {
    #if WANT_DEBUG
    qDebug() << "AvatarActionHold::AvatarActionHold";
    #endif
}

AvatarActionHold::~AvatarActionHold() {
    #if WANT_DEBUG
    qDebug() << "AvatarActionHold::~AvatarActionHold";
    #endif
}

void AvatarActionHold::updateActionWorker(float deltaTimeStep) {
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    if (!tryLockForRead()) {
        // don't risk hanging the thread running the physics simulation
        return;
    }

    glm::vec3 palmPosition;
    if (_hand == "right") {
        palmPosition = myAvatar->getRightPalmPosition();
    } else {
        palmPosition = myAvatar->getLeftPalmPosition();
    }

    auto rotation = myAvatar->getWorldAlignedOrientation();
    auto offset = rotation * _relativePosition;
    auto position = palmPosition + offset;
    rotation *= _relativeRotation;
    unlock();

    if (!tryLockForWrite()) {
        return;
    }
    _positionalTarget = position;
    _rotationalTarget = rotation;
    unlock();

    ObjectActionSpring::updateActionWorker(deltaTimeStep);
}


bool AvatarActionHold::updateArguments(QVariantMap arguments) {
    bool rPOk = true;
    glm::vec3 relativePosition =
        EntityActionInterface::extractVec3Argument("hold", arguments, "relativePosition", rPOk, false);
    bool rROk = true;
    glm::quat relativeRotation =
        EntityActionInterface::extractQuatArgument("hold", arguments, "relativeRotation", rROk, false);
    bool tSOk = true;
    float timeScale =
        EntityActionInterface::extractFloatArgument("hold", arguments, "timeScale", tSOk, false);
    bool hOk = true;
    QString hand =
        EntityActionInterface::extractStringArgument("hold", arguments, "hand", hOk, false);

    lockForWrite();
    if (rPOk) {
        _relativePosition = relativePosition;
    } else if (!_parametersSet) {
        _relativePosition = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    if (rROk) {
        _relativeRotation = relativeRotation;
    } else if (!_parametersSet) {
        _relativeRotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
    }

    if (tSOk) {
        _linearTimeScale = timeScale;
        _angularTimeScale = timeScale;
    } else if (!_parametersSet) {
        _linearTimeScale = 0.2f;
        _angularTimeScale = 0.2f;
    }

    if (hOk) {
        hand = hand.toLower();
        if (hand == "left") {
            _hand = "left";
        } else if (hand == "right") {
            _hand = "right";
        } else {
            qDebug() << "hold action -- invalid hand argument:" << hand;
            _hand = "right";
        }
    } else if (!_parametersSet) {
        _hand = "right";
    }

    _parametersSet = true;
    _positionalTargetSet = true;
    _rotationalTargetSet = true;
    _active = true;
    unlock();
    return true;
}

QByteArray AvatarActionHold::serialize() {
    QByteArray ba;
    QDataStream dataStream(&ba, QIODevice::WriteOnly);

    dataStream << getType();
    dataStream << getID();

    dataStream << _relativePosition;
    dataStream << _relativeRotation;
    dataStream << _hand;

    return ba;
}

void AvatarActionHold::deserialize(QByteArray serializedArguments) {
    QDataStream dataStream(serializedArguments);

    EntityActionType type;
    QUuid id;

    dataStream >> type;
    dataStream >> id;
    assert(type == getType());
    assert(id == getID());

    dataStream >> _relativePosition;
    dataStream >> _relativeRotation;
    dataStream >> _hand;
    _parametersSet = true;

    _active = true;
}
