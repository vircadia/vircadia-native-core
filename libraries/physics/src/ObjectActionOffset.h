//
//  ObjectActionOffset.h
//  libraries/physics/src
//
//  Created by Seth Alves 2015-6-17
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ObjectActionOffset_h
#define hifi_ObjectActionOffset_h

#include <QUuid>

#include <EntityItem.h>
#include "ObjectAction.h"

class ObjectActionOffset : public ObjectAction {
public:
    ObjectActionOffset(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~ObjectActionOffset();

    virtual bool updateArguments(QVariantMap arguments) override;
    virtual QVariantMap getArguments() override;

    virtual void updateActionWorker(float deltaTimeStep) override;

    virtual QByteArray serialize() const override;
    virtual void deserialize(QByteArray serializedArguments) override;

 private:
    static const uint16_t offsetVersion;
    glm::vec3 _pointToOffsetFrom;
    float _linearDistance;
    float _linearTimeScale;
    bool _positionalTargetSet;
};

#endif // hifi_ObjectActionOffset_h
