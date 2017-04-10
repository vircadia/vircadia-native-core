//
//  ScriptAvatarData.cpp
//  libraries/script-engine/src
//
//  Created by Zach Fox on 2017-04-10.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptEngineLogging.h"
#include "ScriptAvatarData.h"

QScriptValue avatarDataToScriptValue(QScriptEngine* engine, const AvatarSharedPointer& in) {
    return engine->newQObject(new ScriptAvatarData(in), QScriptEngine::ScriptOwnership);
}

void avatarDataFromScriptValue(const QScriptValue& object, AvatarSharedPointer& out) {
    // Not implemented - this should never happen (yet)
    assert(false);
    out = AvatarSharedPointer(nullptr);
}

ScriptAvatarData::ScriptAvatarData(AvatarSharedPointer avatarData) :
    _avatarData(avatarData),
    SpatiallyNestable(NestableType::Avatar, QUuid())
{
    QObject::connect(avatarData.get(), &AvatarData::displayNameChanged, this, &ScriptAvatarData::displayNameChanged);
}

const QUuid ScriptAvatarData::getSessionUUID() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getSessionUUID();
    } else {
        return QUuid();
    }
}

glm::vec3 ScriptAvatarData::getPosition() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getPosition();
    } else {
        return glm::vec3();
    }
}

const float ScriptAvatarData::getAudioLoudness() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getAudioLoudness();
    } else {
        return 0.0;
    }
}

const float ScriptAvatarData::getAudioAverageLoudness() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getAudioAverageLoudness();
    } else {
        return 0.0;
    }
}

const QString ScriptAvatarData::getDisplayName() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getDisplayName();
    } else {
        return QString();
    }
}

const QString ScriptAvatarData::getSessionDisplayName() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getSessionDisplayName();
    } else {
        return QString();
    }
}

int ScriptAvatarData::getJointIndex(const QString& name) const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getJointIndex(name);
    } else {
        return -1;
    }
}

glm::quat ScriptAvatarData::getAbsoluteJointRotationInObjectFrame(int index) const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getAbsoluteJointRotationInObjectFrame(index);
    } else {
        return glm::quat();
    }
}

glm::vec3 ScriptAvatarData::getAbsoluteJointTranslationInObjectFrame(int index) const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getAbsoluteJointTranslationInObjectFrame(index);
    } else {
        return glm::vec3();
    }
}
