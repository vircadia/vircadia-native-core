//
//  ObjectActionTravelOriented.h
//  libraries/physics/src
//
//  Created by Seth Alves 2016-8-28
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ObjectActionTravelOriented_h
#define hifi_ObjectActionTravelOriented_h

#include "ObjectAction.h"

class ObjectActionTravelOriented : public ObjectAction {
public:
    ObjectActionTravelOriented(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~ObjectActionTravelOriented();

    virtual bool updateArguments(QVariantMap arguments) override;
    virtual QVariantMap getArguments() override;

    virtual void updateActionWorker(float deltaTimeStep) override;

    virtual QByteArray serialize() const override;
    virtual void deserialize(QByteArray serializedArguments) override;

protected:
    static const uint16_t actionVersion;

    glm::vec3 _forward { glm::vec3() }; // the vector in object space that should point in the direction of travel
    float _angularTimeScale { 0.1f };

    glm::vec3 _angularVelocityTarget;
};

#endif // hifi_ObjectActionTravelOriented_h
