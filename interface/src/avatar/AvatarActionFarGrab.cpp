//
//  AvatarActionFarGrab.cpp
//  interface/src/avatar/
//
//  Created by Seth Alves 2017-4-14
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarActionFarGrab.h"

AvatarActionFarGrab::AvatarActionFarGrab(const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectActionTractor(id, ownerEntity) {
    _type = DYNAMIC_TYPE_FAR_GRAB;
#if WANT_DEBUG
    qDebug() << "AvatarActionFarGrab::AvatarActionFarGrab";
#endif
}

AvatarActionFarGrab::~AvatarActionFarGrab() {
#if WANT_DEBUG
    qDebug() << "AvatarActionFarGrab::~AvatarActionFarGrab";
#endif
}


QByteArray AvatarActionFarGrab::serialize() const {
    QByteArray serializedActionArguments;
    QDataStream dataStream(&serializedActionArguments, QIODevice::WriteOnly);

    dataStream << DYNAMIC_TYPE_FAR_GRAB;
    dataStream << getID();
    dataStream << ObjectActionTractor::tractorVersion;

    serializeParameters(dataStream);

    return serializedActionArguments;
}

void AvatarActionFarGrab::deserialize(QByteArray serializedArguments) {
    QDataStream dataStream(serializedArguments);

    EntityDynamicType type;
    dataStream >> type;

    QUuid id;
    dataStream >> id;

    if (type != getType() || id != getID()) {
        qDebug() << "AvatarActionFarGrab::deserialize type or ID don't match." << type << id << getID();
        return;
    }

    uint16_t serializationVersion;
    dataStream >> serializationVersion;
    if (serializationVersion != ObjectActionTractor::tractorVersion) {
        assert(false);
        return;
    }

    deserializeParameters(serializedArguments, dataStream);
}
