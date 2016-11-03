//
//  InterfaceActionFactory.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//



#include <avatar/AvatarActionHold.h>
#include <ObjectActionOffset.h>
#include <ObjectActionSpring.h>
#include <ObjectActionTravelOriented.h>
#include <LogHandler.h>

#include "InterfaceActionFactory.h"


EntityActionPointer interfaceActionFactory(EntityActionType type, const QUuid& id, EntityItemPointer ownerEntity) {
    switch (type) {
        case ACTION_TYPE_NONE:
            return EntityActionPointer();
        case ACTION_TYPE_OFFSET:
            return std::make_shared<ObjectActionOffset>(id, ownerEntity);
        case ACTION_TYPE_SPRING:
            return std::make_shared<ObjectActionSpring>(id, ownerEntity);
        case ACTION_TYPE_HOLD:
            return std::make_shared<AvatarActionHold>(id, ownerEntity);
        case ACTION_TYPE_TRAVEL_ORIENTED:
            return std::make_shared<ObjectActionTravelOriented>(id, ownerEntity);
    }

    Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown entity action type");
    return EntityActionPointer();
}


EntityActionPointer InterfaceActionFactory::factory(EntityActionType type,
                                                    const QUuid& id,
                                                    EntityItemPointer ownerEntity,
                                                    QVariantMap arguments) {
    EntityActionPointer action = interfaceActionFactory(type, id, ownerEntity);
    if (action) {
        bool ok = action->updateArguments(arguments);
        if (ok) {
            if (action->lifetimeIsOver()) {
                return nullptr;
            }
            return action;
        }
    }
    return nullptr;
}


EntityActionPointer InterfaceActionFactory::factoryBA(EntityItemPointer ownerEntity, QByteArray data) {
    QDataStream serializedArgumentStream(data);
    EntityActionType type;
    QUuid id;

    serializedArgumentStream >> type;
    serializedArgumentStream >> id;

    EntityActionPointer action = interfaceActionFactory(type, id, ownerEntity);

    if (action) {
        action->deserialize(data);
        if (action->lifetimeIsOver()) {
            static QString repeatedMessage =
                LogHandler::getInstance().addRepeatedMessageRegex(".*factoryBA lifetimeIsOver during action creation.*");
            qDebug() << "InterfaceActionFactory::factoryBA lifetimeIsOver during action creation --"
                     << action->getExpires() << "<" << usecTimestampNow();
            return nullptr;
        }
    }

    return action;
}
