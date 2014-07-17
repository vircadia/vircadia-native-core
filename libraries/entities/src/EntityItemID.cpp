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

uint32_t EntityItemID::_nextID = 0; // TODO: should be changed to UUID

// for locally created models
std::map<uint32_t,uint32_t> EntityItemID::_tokenIDsToIDs;
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


EntityItemID::EntityItemID(uint32_t id, uint32_t creatorTokenID, bool isKnownID) :
    id(id), 
    creatorTokenID(creatorTokenID), 
    isKnownID(isKnownID) 
{ 
    //qDebug() << "EntityItemID::EntityItemID(uint32_t id, uint32_t creatorTokenID, bool isKnownID)... isKnownID=" 
    //      << isKnownID << "id=" << id << "creatorTokenID=" << creatorTokenID;
};

EntityItemID::EntityItemID(uint32_t id) :
    id(id), 
    creatorTokenID(UNKNOWN_ENTITY_TOKEN), 
    isKnownID(true) 
{ 
    //qDebug() << "EntityItemID::EntityItemID(uint32_t id)... isKnownID=" 
    //      << isKnownID << "id=" << id << "creatorTokenID=" << creatorTokenID;
};

uint32_t EntityItemID::getIDfromCreatorTokenID(uint32_t creatorTokenID) {
    if (_tokenIDsToIDs.find(creatorTokenID) != _tokenIDsToIDs.end()) {
        return _tokenIDsToIDs[creatorTokenID];
    }
    return UNKNOWN_ENTITY_ID;
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
    newlyAssignedEntityID.id = _nextID;
    _nextID++;
    
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

void EntityItemID::handleAddEntityResponse(const QByteArray& packet) {

qDebug() << "EntityItemID::handleAddEntityResponse()...";

    const unsigned char* dataAt = reinterpret_cast<const unsigned char*>(packet.data());
    int numBytesPacketHeader = numBytesForPacketHeader(packet);
    dataAt += numBytesPacketHeader;

    uint32_t creatorTokenID;
    memcpy(&creatorTokenID, dataAt, sizeof(creatorTokenID));
    dataAt += sizeof(creatorTokenID);

    uint32_t entityItemID;
    memcpy(&entityItemID, dataAt, sizeof(entityItemID));
    dataAt += sizeof(entityItemID);

qDebug() << "EntityItemID::handleAddEntityResponse()... entityItemID=" << entityItemID << "creatorTokenID=" << creatorTokenID;

    // add our token to id mapping
    _tokenIDsToIDs[creatorTokenID] = entityItemID;
}

QScriptValue EntityItemIDtoScriptValue(QScriptEngine* engine, const EntityItemID& id) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("id", id.id);
    obj.setProperty("creatorTokenID", id.creatorTokenID);
    obj.setProperty("isKnownID", id.isKnownID);
    return obj;
}

void EntityItemIDfromScriptValue(const QScriptValue &object, EntityItemID& id) {
    id.id = object.property("id").toVariant().toUInt();
    id.creatorTokenID = object.property("creatorTokenID").toVariant().toUInt();
    id.isKnownID = object.property("isKnownID").toVariant().toBool();
}



