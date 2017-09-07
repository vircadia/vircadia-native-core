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

#include "ScriptAvatarData.h"

ScriptAvatarData::ScriptAvatarData(AvatarSharedPointer avatarData) :
    _avatarData(avatarData)
{
    QObject::connect(avatarData.get(), &AvatarData::displayNameChanged, this, &ScriptAvatarData::displayNameChanged);
    QObject::connect(avatarData.get(), &AvatarData::lookAtSnappingChanged, this, &ScriptAvatarData::lookAtSnappingChanged);
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
float ScriptAvatarData::getTargetScale() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getTargetScale();
    } else {
        return 0.0f;
    }
}
glm::vec3 ScriptAvatarData::getHandPosition() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getHandPosition();
    } else {
        return glm::vec3();
    }
}
float ScriptAvatarData::getBodyPitch() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getBodyPitch();
    } else {
        return 0.0f;
    }
}
float ScriptAvatarData::getBodyYaw() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getBodyYaw();
    } else {
        return 0.0f;
    }
}
float ScriptAvatarData::getBodyRoll() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getBodyRoll();
    } else {
        return 0.0f;
    }
}
glm::quat ScriptAvatarData::getOrientation() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getOrientation();
    } else {
        return glm::quat();
    }
}
glm::quat ScriptAvatarData::getHeadOrientation() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getHeadOrientation();
    } else {
        return glm::quat();
    }
}
float ScriptAvatarData::getHeadPitch() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getHeadPitch();
    } else {
        return 0.0f;
    }
}
float ScriptAvatarData::getHeadYaw() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getHeadYaw();
    } else {
        return 0.0f;
    }
}
float ScriptAvatarData::getHeadRoll() const {
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
glm::vec3 ScriptAvatarData::getVelocity() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getVelocity();
    } else {
        return glm::vec3();
    }
}
glm::vec3 ScriptAvatarData::getAngularVelocity() const {
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
QUuid ScriptAvatarData::getSessionUUID() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getSessionUUID();
    } else {
        return QUuid();
    }
}
QString ScriptAvatarData::getDisplayName() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getDisplayName();
    } else {
        return QString();
    }
}
QString ScriptAvatarData::getSessionDisplayName() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getSessionDisplayName();
    } else {
        return QString();
    }
}

bool ScriptAvatarData::getIsReplicated() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getIsReplicated();
    } else {
        return false;
    }
}

bool ScriptAvatarData::getLookAtSnappingEnabled() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getLookAtSnappingEnabled();
    } else {
        return false;
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
QString ScriptAvatarData::getSkeletonModelURLFromScript() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getSkeletonModelURLFromScript();
    } else {
        return QString();
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
glm::quat ScriptAvatarData::getJointRotation(const QString& name) const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getJointRotation(name);
    } else {
        return glm::quat();
    }
}
glm::vec3 ScriptAvatarData::getJointTranslation(const QString& name) const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getJointTranslation(name);
    } else {
        return glm::vec3();
    }
}
QVector<glm::quat> ScriptAvatarData::getJointRotations() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getJointRotations();
    } else {
        return QVector<glm::quat>();
    }
}
QVector<glm::vec3> ScriptAvatarData::getJointTranslations() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getJointTranslations();
    } else {
        return QVector<glm::vec3>();
    }
}
bool ScriptAvatarData::isJointDataValid(const QString& name) const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->isJointDataValid(name);
    } else {
        return false;
    }
}
int ScriptAvatarData::getJointIndex(const QString& name) const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getJointIndex(name);
    } else {
        return -1;
    }
}
QStringList ScriptAvatarData::getJointNames() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getJointNames();
    } else {
        return QStringList();
    }
}
QVector<AttachmentData> ScriptAvatarData::getAttachmentData() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getAttachmentData();
    } else {
        return QVector<AttachmentData>();
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
float ScriptAvatarData::getAudioLoudness() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getAudioLoudness();
    } else {
        return 0.0f;
    }
}
float ScriptAvatarData::getAudioAverageLoudness() const {
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
glm::mat4 ScriptAvatarData::getSensorToWorldMatrix() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getSensorToWorldMatrix();
    } else {
        return glm::mat4();
    }
}
glm::mat4 ScriptAvatarData::getControllerLeftHandMatrix() const {
    if (AvatarSharedPointer sharedAvatarData = _avatarData.lock()) {
        return sharedAvatarData->getControllerLeftHandMatrix();
    } else {
        return glm::mat4();
    }
}
glm::mat4 ScriptAvatarData::getControllerRightHandMatrix() const {
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
