//
//  ObjectActionSpring.h
//  libraries/physics/src
//
//  Created by Seth Alves 2015-6-5
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ObjectActionSpring_h
#define hifi_ObjectActionSpring_h

#include "ObjectAction.h"

class ObjectActionSpring : public ObjectAction {
public:
    ObjectActionSpring(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~ObjectActionSpring();

    virtual bool updateArguments(QVariantMap arguments) override;
    virtual QVariantMap getArguments() override;

    virtual void updateActionWorker(float deltaTimeStep) override;

    virtual QByteArray serialize() const override;
    virtual void deserialize(QByteArray serializedArguments) override;

    virtual bool getTarget(float deltaTimeStep, glm::quat& rotation, glm::vec3& position,
                           glm::vec3& linearVelocity, glm::vec3& angularVelocity);

protected:
    static const uint16_t springVersion;

    glm::vec3 _positionalTarget;
    glm::vec3 _desiredPositionalTarget;
    float _linearTimeScale;
    bool _positionalTargetSet;

    glm::quat _rotationalTarget;
    glm::quat _desiredRotationalTarget;
    float _angularTimeScale;
    bool _rotationalTargetSet;

    glm::vec3 _linearVelocityTarget;
    glm::vec3 _angularVelocityTarget;

    virtual bool prepareForSpringUpdate(btScalar deltaTimeStep);
};

#endif // hifi_ObjectActionSpring_h
