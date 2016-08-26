//
//  AvatarActionHold.h
//  interface/src/avatar/
//
//  Created by Seth Alves 2015-6-9
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarActionHold_h
#define hifi_AvatarActionHold_h

#include <QUuid>

#include <EntityItem.h>
#include <ObjectActionSpring.h>

#include "avatar/MyAvatar.h"


class AvatarActionHold : public ObjectActionSpring {
public:
    AvatarActionHold(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~AvatarActionHold();

    virtual bool updateArguments(QVariantMap arguments) override;
    virtual QVariantMap getArguments() override;

    virtual void updateActionWorker(float deltaTimeStep) override;

    QByteArray serialize() const override;
    virtual void deserialize(QByteArray serializedArguments) override;

    virtual bool shouldSuppressLocationEdits() override { return _active && !_ownerEntity.expired(); }

    bool getAvatarRigidBodyLocation(glm::vec3& avatarRigidBodyPosition, glm::quat& avatarRigidBodyRotation);
    virtual bool getTarget(float deltaTimeStep, glm::quat& rotation, glm::vec3& position,
                           glm::vec3& linearVelocity, glm::vec3& angularVelocity) override;

    virtual void prepareForPhysicsSimulation() override;

private:
    void doKinematicUpdate(float deltaTimeStep);

    static const uint16_t holdVersion;

    glm::vec3 _relativePosition { Vectors::ZERO };
    glm::quat _relativeRotation { Quaternions::IDENTITY };
    QString _hand { "right" };
    QUuid _holderID;

    bool _kinematic { false };
    bool _kinematicSetVelocity { false };
    bool _previousSet { false };
    bool _ignoreIK { false };
    glm::vec3 _previousPositionalTarget;
    glm::quat _previousRotationalTarget;

    float _previousDeltaTimeStep { 0.0f };
    glm::vec3 _previousPositionalDelta;

    glm::vec3 _palmOffsetFromRigidBody;
    // leaving this here for future refernece.
    // glm::quat _palmRotationFromRigidBody;

    static const int velocitySmoothFrames;
    QVector<glm::vec3> _measuredLinearVelocities;
    int _measuredLinearVelocitiesIndex { 0 };

    glm::vec3 _preStepAvatarPosition;
    glm::quat _preStepAvatarRotation;
};

#endif // hifi_AvatarActionHold_h
