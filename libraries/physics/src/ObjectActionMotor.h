//
//  ObjectActionMotor.h
//  libraries/physics/src
//
//  Created by Seth Alves 2017-4-30
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ObjectActionMotor_h
#define hifi_ObjectActionMotor_h

#include "ObjectAction.h"

class ObjectActionMotor : public ObjectAction {
public:
    ObjectActionMotor(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~ObjectActionMotor();

    virtual bool updateArguments(QVariantMap arguments) override;
    virtual QVariantMap getArguments() override;

    virtual void updateActionWorker(float deltaTimeStep) override;

    virtual QByteArray serialize() const override;
    virtual void deserialize(QByteArray serializedArguments) override;

protected:
    static const uint16_t motorVersion;

    glm::vec3 _angularVelocityTarget;
    float _angularTimeScale { FLT_MAX };

    void serializeParameters(QDataStream& dataStream) const;
    void deserializeParameters(QByteArray serializedArguments, QDataStream& dataStream);
};

#endif // hifi_ObjectActionMotor_h
