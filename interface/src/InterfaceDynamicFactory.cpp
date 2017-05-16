//
//  InterfaceDynamicFactory.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//



#include <avatar/AvatarActionHold.h>
#include <avatar/AvatarActionFarGrab.h>
#include <ObjectActionOffset.h>
#include <ObjectActionTractor.h>
#include <ObjectActionTravelOriented.h>
#include <ObjectConstraintHinge.h>
#include <ObjectConstraintSlider.h>
#include <ObjectConstraintBallSocket.h>
#include <ObjectConstraintConeTwist.h>
#include <LogHandler.h>

#include "InterfaceDynamicFactory.h"


EntityDynamicPointer interfaceDynamicFactory(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity) {
    switch (type) {
        case DYNAMIC_TYPE_NONE:
            return EntityDynamicPointer();
        case DYNAMIC_TYPE_OFFSET:
            return std::make_shared<ObjectActionOffset>(id, ownerEntity);
        case DYNAMIC_TYPE_SPRING:
            qDebug() << "The 'spring' Action is deprecated.  Replacing with 'tractor' Action.";
        case DYNAMIC_TYPE_TRACTOR:
            return std::make_shared<ObjectActionTractor>(id, ownerEntity);
        case DYNAMIC_TYPE_HOLD:
            return std::make_shared<AvatarActionHold>(id, ownerEntity);
        case DYNAMIC_TYPE_TRAVEL_ORIENTED:
            return std::make_shared<ObjectActionTravelOriented>(id, ownerEntity);
        case DYNAMIC_TYPE_HINGE:
            return std::make_shared<ObjectConstraintHinge>(id, ownerEntity);
        case DYNAMIC_TYPE_FAR_GRAB:
            return std::make_shared<AvatarActionFarGrab>(id, ownerEntity);
        case DYNAMIC_TYPE_SLIDER:
            return std::make_shared<ObjectConstraintSlider>(id, ownerEntity);
        case DYNAMIC_TYPE_BALL_SOCKET:
            return std::make_shared<ObjectConstraintBallSocket>(id, ownerEntity);
        case DYNAMIC_TYPE_CONE_TWIST:
            return std::make_shared<ObjectConstraintConeTwist>(id, ownerEntity);
    }

    qDebug() << "Unknown entity dynamic type";
    return EntityDynamicPointer();
}


EntityDynamicPointer InterfaceDynamicFactory::factory(EntityDynamicType type,
                                                    const QUuid& id,
                                                    EntityItemPointer ownerEntity,
                                                    QVariantMap arguments) {
    EntityDynamicPointer dynamic = interfaceDynamicFactory(type, id, ownerEntity);
    if (dynamic) {
        bool ok = dynamic->updateArguments(arguments);
        if (ok) {
            if (dynamic->lifetimeIsOver()) {
                return nullptr;
            }
            return dynamic;
        }
    }
    return nullptr;
}


EntityDynamicPointer InterfaceDynamicFactory::factoryBA(EntityItemPointer ownerEntity, QByteArray data) {
    QDataStream serializedArgumentStream(data);
    EntityDynamicType type;
    QUuid id;

    serializedArgumentStream >> type;
    serializedArgumentStream >> id;

    EntityDynamicPointer dynamic = interfaceDynamicFactory(type, id, ownerEntity);

    if (dynamic) {
        dynamic->deserialize(data);
        if (dynamic->lifetimeIsOver()) {
            static QString repeatedMessage =
                LogHandler::getInstance().addRepeatedMessageRegex(".*factoryBA lifetimeIsOver during dynamic creation.*");
            qDebug() << "InterfaceDynamicFactory::factoryBA lifetimeIsOver during dynamic creation --"
                     << dynamic->getExpires() << "<" << usecTimestampNow();
            return nullptr;
        }
    }

    return dynamic;
}
