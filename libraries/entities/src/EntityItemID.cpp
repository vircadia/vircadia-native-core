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

#include "EntityItemID.h"

//uint32_t EntityItemID::_nextID = 0; // TODO: should be changed to UUID

// for locally created models
QHash<uint32_t, QUuid> EntityItemID::_tokenIDsToIDs;
uint32_t EntityItemID::_nextCreatorTokenID = 0;


EntityItemID::EntityItemID() :
    id(NEW_ENTITY), 
    creatorTokenID(UNKNOWN_ENTITY_TOKEN), 
    isKnownID(false) 
{ 
    //qDebug() << "EntityItemID::EntityItemID()... isKnownID=" 
    //      << isKnownID << "id=" << id << "creatorTokenID=" << creatorTokenID;
};

EntityItemID::EntityItemID(const EntityItemID& other)  :
    id(other.id), 
    creatorTokenID(other.creatorTokenID), 
    isKnownID(other.isKnownID)
{
    //qDebug() << "EntityItemID::EntityItemID(const EntityItemID& other)... isKnownID=" 
    //      << isKnownID << "id=" << id << "creatorTokenID=" << creatorTokenID;
}


EntityItemID::EntityItemID(const QUuid& id, uint32_t creatorTokenID, bool isKnownID) :
    id(id), 
    creatorTokenID(creatorTokenID), 
    isKnownID(isKnownID) 
{ 
    //qDebug() << "EntityItemID::EntityItemID(uint32_t id, uint32_t creatorTokenID, bool isKnownID)... isKnownID=" 
    //      << isKnownID << "id=" << id << "creatorTokenID=" << creatorTokenID;
};

EntityItemID::EntityItemID(const QUuid& id) :
    id(id), 
    creatorTokenID(UNKNOWN_ENTITY_TOKEN), 
    isKnownID(true) 
{ 
    //qDebug() << "EntityItemID::EntityItemID(uint32_t id)... isKnownID=" 
    //      << isKnownID << "id=" << id << "creatorTokenID=" << creatorTokenID;
};

EntityItemID EntityItemID::getIDfromCreatorTokenID(uint32_t creatorTokenID) {
    if (_tokenIDsToIDs.find(creatorTokenID) != _tokenIDsToIDs.end()) {
        return EntityItemID(_tokenIDsToIDs[creatorTokenID], creatorTokenID, true);
    }
    return EntityItemID(UNKNOWN_ENTITY_ID);
}

uint32_t EntityItemID::getNextCreatorTokenID() {
    uint32_t creatorTokenID = _nextCreatorTokenID;
    _nextCreatorTokenID++;
    return creatorTokenID;
}

EntityItemID EntityItemID::assignActualIDForToken() const {
    EntityItemID newlyAssignedEntityID;

    newlyAssignedEntityID.creatorTokenID = creatorTokenID;
    newlyAssignedEntityID.isKnownID = true;
    newlyAssignedEntityID.id = QUuid::createUuid();
    //_nextID++;
    
    return newlyAssignedEntityID;
}

EntityItemID EntityItemID::convertToKnownIDVersion() const {
    EntityItemID knownIDVersionEntityID;

    knownIDVersionEntityID.creatorTokenID = UNKNOWN_ENTITY_TOKEN;
    knownIDVersionEntityID.isKnownID = true;
    knownIDVersionEntityID.id = id;
    
    return knownIDVersionEntityID;
}

EntityItemID EntityItemID::convertToCreatorTokenVersion() const {
    EntityItemID knownIDVersionEntityID;

    knownIDVersionEntityID.creatorTokenID = creatorTokenID;
    knownIDVersionEntityID.isKnownID = false;
    knownIDVersionEntityID.id = UNKNOWN_ENTITY_ID;

    return knownIDVersionEntityID;
}

EntityItemID EntityItemID::readEntityItemIDFromBuffer(const unsigned char* data, int bytesLeftToRead) {
    EntityItemID result;
    
    if (bytesLeftToRead >= NUM_BYTES_RFC4122_UUID) {
        // id
        QByteArray encodedID((const char*)data, NUM_BYTES_RFC4122_UUID);
        result.id = QUuid::fromRfc4122(encodedID);
        result.isKnownID = true;
        result.creatorTokenID = UNKNOWN_ENTITY_TOKEN;
    }
    return result;
}


void EntityItemID::handleAddEntityResponse(const QByteArray& packet) {

qDebug() << "EntityItemID::handleAddEntityResponse()...";

    const unsigned char* dataAt = reinterpret_cast<const unsigned char*>(packet.data());
    int numBytesPacketHeader = numBytesForPacketHeader(packet);
    int bytesRead = numBytesPacketHeader;
    dataAt += numBytesPacketHeader;

    uint32_t creatorTokenID;
    memcpy(&creatorTokenID, dataAt, sizeof(creatorTokenID));
    dataAt += sizeof(creatorTokenID);
    bytesRead += sizeof(creatorTokenID);

    QUuid entityID = QUuid::fromRfc4122(packet.mid(bytesRead, NUM_BYTES_RFC4122_UUID));
    dataAt += NUM_BYTES_RFC4122_UUID;

qDebug() << "EntityItemID::handleAddEntityResponse()... entityID=" << entityID << "creatorTokenID=" << creatorTokenID;

    // add our token to id mapping
    _tokenIDsToIDs[creatorTokenID] = entityID;
}

QScriptValue EntityItemIDtoScriptValue(QScriptEngine* engine, const EntityItemID& id) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("id", id.id.toString());
    obj.setProperty("creatorTokenID", id.creatorTokenID);
    obj.setProperty("isKnownID", id.isKnownID);
    return obj;
}

void EntityItemIDfromScriptValue(const QScriptValue &object, EntityItemID& id) {
    id.id = QUuid(object.property("id").toVariant().toString());
    id.creatorTokenID = object.property("creatorTokenID").toVariant().toUInt();
    id.isKnownID = object.property("isKnownID").toVariant().toBool();
}





