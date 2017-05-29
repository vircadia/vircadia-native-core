//
//  ScriptAvatar.h
//  interface/src/avatars
//
//  Created by Stephen Birarda on 4/10/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptAvatar_h
#define hifi_ScriptAvatar_h

#include <ScriptAvatarData.h>

#include "Avatar.h"

class ScriptAvatar : public ScriptAvatarData {
    Q_OBJECT

    Q_PROPERTY(glm::vec3 skeletonOffset READ getSkeletonOffset)

public:
    ScriptAvatar(AvatarSharedPointer avatarData);

public slots:

    glm::quat getDefaultJointRotation(int index) const;
    glm::vec3 getDefaultJointTranslation(int index) const;

    glm::vec3 getSkeletonOffset() const;

    glm::vec3 getJointPosition(int index) const;
    glm::vec3 getJointPosition(const QString& name) const;
    glm::vec3 getNeckPosition() const;

    glm::vec3 getAcceleration() const;

    QUuid getParentID() const;
    quint16 getParentJointIndex() const;

    QVariantList getSkeleton() const;

    float getSimulationRate(const QString& rateName = QString("")) const;

    glm::vec3 getLeftPalmPosition() const;
    glm::quat getLeftPalmRotation() const;
    glm::vec3 getRightPalmPosition() const;
    glm::quat getRightPalmRotation() const;

private:
    std::shared_ptr<Avatar> lockAvatar() const;
};

#endif // hifi_ScriptAvatar_h
