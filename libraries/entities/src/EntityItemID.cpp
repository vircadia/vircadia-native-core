//
//  EntityItemID.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QObject>
#include <QDebug>

#include <PacketHeaders.h>

#include "RegisteredMetaTypes.h"
#include "EntityItemID.h"


EntityItemID::EntityItemID() :
    id(UNKNOWN_ENTITY_ID)
{ 
}


EntityItemID::EntityItemID(const QUuid& id) :
    id(id)
{ 
}

EntityItemID::EntityItemID(const EntityItemID& other) : id(other.id)
{
}

EntityItemID EntityItemID::readEntityItemIDFromBuffer(const unsigned char* data, int bytesLeftToRead) {
    EntityItemID result;

    if (bytesLeftToRead >= NUM_BYTES_RFC4122_UUID) {
        // id
        QByteArray encodedID((const char*)data, NUM_BYTES_RFC4122_UUID);
        result.id = QUuid::fromRfc4122(encodedID);
    }
    return result;
}

QScriptValue EntityItemID::toScriptValue(QScriptEngine* engine) const { 
    return EntityItemIDtoScriptValue(engine, *this); 
}

QScriptValue EntityItemIDtoScriptValue(QScriptEngine* engine, const EntityItemID& id) {
    return quuidToScriptValue(engine, id.id);
}

void EntityItemIDfromScriptValue(const QScriptValue &object, EntityItemID& id) {
    quuidFromScriptValue(object, id.id);
}
