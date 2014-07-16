//
//  EntityItemID.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityItemID_h
#define hifi_EntityItemID_h

#include <stdint.h>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>

const uint32_t NEW_ENTITY = 0xFFFFFFFF;
const uint32_t UNKNOWN_ENTITY_TOKEN = 0xFFFFFFFF;
const uint32_t UNKNOWN_ENTITY_ID = 0xFFFFFFFF;


/// Abstract ID for editing model items. Used in EntityItem JS API - When models are created in the JS api, they are given a
/// local creatorTokenID, the actual id for the model is not known until the server responds to the creator with the
/// correct mapping. This class works with the scripting API an allows the developer to edit models they created.
class EntityItemID {
public:
    EntityItemID() :
            id(NEW_ENTITY), creatorTokenID(UNKNOWN_ENTITY_TOKEN), isKnownID(false) { 
    //qDebug() << "EntityItemID::EntityItemID()... isKnownID=" << isKnownID << "id=" << id << "creatorTokenID=" << creatorTokenID;
    };

    EntityItemID(uint32_t id, uint32_t creatorTokenID, bool isKnownID) :
            id(id), creatorTokenID(creatorTokenID), isKnownID(isKnownID) { 
    //qDebug() << "EntityItemID::EntityItemID(uint32_t id, uint32_t creatorTokenID, bool isKnownID)... isKnownID=" << isKnownID << "id=" << id << "creatorTokenID=" << creatorTokenID;
    };

    EntityItemID(uint32_t id) :
            id(id), creatorTokenID(UNKNOWN_ENTITY_TOKEN), isKnownID(true) { 
    //qDebug() << "EntityItemID::EntityItemID(uint32_t id)... isKnownID=" << isKnownID << "id=" << id << "creatorTokenID=" << creatorTokenID;
    };

    uint32_t id;
    uint32_t creatorTokenID;
    bool isKnownID;
};

inline bool operator<(const EntityItemID& a, const EntityItemID& b) {
    return (a.id == b.id) ? (a.creatorTokenID < b.creatorTokenID) : (a.id < b.id);
}

inline bool operator==(const EntityItemID& a, const EntityItemID& b) {
    if (a.id == UNKNOWN_ENTITY_ID && b.id == UNKNOWN_ENTITY_ID) {
        return a.creatorTokenID == b.creatorTokenID;
    }
    return a.id == b.id;
}

inline uint qHash(const EntityItemID& a, uint seed) {
    qint64 temp;
    if (a.id == UNKNOWN_ENTITY_ID) {
        temp = -a.creatorTokenID;
    } else {
        temp = a.id;
    }
    return qHash(temp, seed);
}

inline QDebug operator<<(QDebug debug, const EntityItemID& id) {
    debug << "[ id:" << id.id << ", creatorTokenID:" << id.creatorTokenID << ", isKnownID:" << id.isKnownID << "]";
    return debug;
}

Q_DECLARE_METATYPE(EntityItemID);
Q_DECLARE_METATYPE(QVector<EntityItemID>);
QScriptValue EntityItemIDtoScriptValue(QScriptEngine* engine, const EntityItemID& properties);
void EntityItemIDfromScriptValue(const QScriptValue &object, EntityItemID& properties);


#endif // hifi_EntityItemID_h
