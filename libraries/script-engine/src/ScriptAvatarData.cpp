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


//
// PHYSICAL PROPERTIES: POSITION AND ORIENTATION
// START
//
glm::vec3 ScriptAvatarData::getPosition() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getPosition();
    } else {
        return glm::vec3();
    }
}
const float ScriptAvatarData::getTargetScale() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getTargetScale();
    } else {
        return 0.0f;
    }
}
const glm::vec3 ScriptAvatarData::getHandPosition() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getHandPosition();
    } else {
        return glm::vec3();
    }
}
const float ScriptAvatarData::getBodyPitch() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getBodyPitch();
    } else {
        return 0.0f;
    }
}
const float ScriptAvatarData::getBodyYaw() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getBodyYaw();
    } else {
        return 0.0f;
    }
}
const float ScriptAvatarData::getBodyRoll() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getBodyRoll();
    } else {
        return 0.0f;
    }
}
const glm::quat ScriptAvatarData::getOrientation() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getOrientation();
    } else {
        return glm::quat();
    }
}
const glm::quat ScriptAvatarData::getHeadOrientation() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getHeadOrientation();
    } else {
        return glm::quat();
    }
}
const float ScriptAvatarData::getHeadPitch() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getHeadPitch();
    } else {
        return 0.0f;
    }
}
const float ScriptAvatarData::getHeadYaw() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getHeadYaw();
    } else {
        return 0.0f;
    }
}
const float ScriptAvatarData::getHeadRoll() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getHeadRoll();
    } else {
        return 0.0f;
    }
}
//
// PHYSICAL PROPERTIES: POSITION AND ORIENTATION
// END
//

//
// PHYSICAL PROPERTIES: VELOCITY
// START
//
const glm::vec3 ScriptAvatarData::getVelocity() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getVelocity();
    } else {
        return glm::vec3();
    }
}
const glm::vec3 ScriptAvatarData::getAngularVelocity() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getAngularVelocity();
    } else {
        return glm::vec3();
    }
}
//
// PHYSICAL PROPERTIES: VELOCITY
// END
//


//
// IDENTIFIER PROPERTIES
// START
//
const QUuid ScriptAvatarData::getSessionUUID() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getSessionUUID();
    } else {
        return QUuid();
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
//
// IDENTIFIER PROPERTIES
// END
//

//
// ATTACHMENT AND JOINT PROPERTIES
// START
//
const QString ScriptAvatarData::getSkeletonModelURLFromScript() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getSkeletonModelURLFromScript();
    } else {
        return QString();
    }
}
const QVector<AttachmentData> ScriptAvatarData::getAttachmentData() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getAttachmentData();
    } else {
        return QVector<AttachmentData>();
    }
}
const QStringList ScriptAvatarData::getJointNames() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getJointNames();
    } else {
        return QStringList();
    }
}
int ScriptAvatarData::getJointIndex(const QString& name) const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getJointIndex(name);
    } else {
        return -1;
    }
}
char ScriptAvatarData::getHandState() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getHandState();
    } else {
        return -1;
    }
}
glm::quat ScriptAvatarData::getJointRotation(int index) const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getJointRotation(index);
    } else {
        return glm::quat();
    }
}
glm::vec3 ScriptAvatarData::getJointTranslation(int index) const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getJointTranslation(index);
    } else {
        return glm::vec3();
    }
}
//
// ATTACHMENT AND JOINT PROPERTIES
// END
//


//
// AUDIO PROPERTIES
// START
//
const float ScriptAvatarData::getAudioLoudness() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getAudioLoudness();
    } else {
        return 0.0f;
    }
}
const float ScriptAvatarData::getAudioAverageLoudness() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getAudioAverageLoudness();
    } else {
        return 0.0f;
    }
}
//
// AUDIO PROPERTIES
// END
//

//
// MATRIX PROPERTIES
// START
//
const glm::mat4 ScriptAvatarData::getSensorToWorldMatrix() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getSensorToWorldMatrix();
    } else {
        return glm::mat4();
    }
}
const glm::mat4 ScriptAvatarData::getControllerLeftHandMatrix() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getControllerLeftHandMatrix();
    } else {
        return glm::mat4();
    }
}
const glm::mat4 ScriptAvatarData::getControllerRightHandMatrix() {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getControllerRightHandMatrix();
    } else {
        return glm::mat4();
    }
}
//
// MATRIX PROPERTIES
// END
//

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
