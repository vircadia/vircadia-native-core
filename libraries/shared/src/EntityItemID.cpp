//
//  EntityItemID.cpp
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityItemID.h"
#include <QtCore/QObject>
#include <QDebug>

#include "BufferParser.h"
#include "UUID.h"

int entityItemIDTypeID = qRegisterMetaType<EntityItemID>();

EntityItemID::EntityItemID() : QUuid()
{
}


EntityItemID::EntityItemID(const QUuid& id) : QUuid(id)
{
}

// EntityItemID::EntityItemID(const EntityItemID& other) : QUuid(other)
// {
// }

EntityItemID EntityItemID::readEntityItemIDFromBuffer(const unsigned char* data, int bytesLeftToRead) {
    EntityItemID result;
    if (bytesLeftToRead >= NUM_BYTES_RFC4122_UUID) {
        BufferParser(data, bytesLeftToRead).readUuid(result);
    }
    return result;
}

size_t std::hash<EntityItemID>::operator()(const EntityItemID& id) const { return qHash(id); }
