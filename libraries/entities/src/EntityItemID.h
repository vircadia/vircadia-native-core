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


#include <QObject>
#include <QHash>
#include <QScriptEngine>
#include <QUuid>

const uint32_t UNKNOWN_ENTITY_TOKEN = 0xFFFFFFFF;

//const uint32_t NEW_ENTITY = 0xFFFFFFFF;
//const uint32_t UNKNOWN_ENTITY_ID = 0xFFFFFFFF;

const QUuid NEW_ENTITY;
const QUuid UNKNOWN_ENTITY_ID;


/// Abstract ID for editing model items. Used in EntityItem JS API - When models are created in the JS api, they are given a
/// local creatorTokenID, the actual id for the model is not known until the server responds to the creator with the
/// correct mapping. This class works with the scripting API an allows the developer to edit models they created.
class EntityItemID {
public:
    EntityItemID();
    EntityItemID(const EntityItemID& other);
    EntityItemID(const QUuid& id, uint32_t creatorTokenID, bool isKnownID);
    EntityItemID(const QUuid& id);

    //uint32_t id;
    QUuid id;
    
    uint32_t creatorTokenID;
    bool isKnownID;

    // these methods will reduce the ID down to half the IDs data to allow for comparisons and searches of known values
    EntityItemID convertToKnownIDVersion() const;
    EntityItemID convertToCreatorTokenVersion() const;

    // these methods allow you to create models, and later edit them.
    //static uint32_t getIDfromCreatorTokenID(uint32_t creatorTokenID);

    static EntityItemID getIDfromCreatorTokenID(uint32_t creatorTokenID);
    static uint32_t getNextCreatorTokenID();
    static void handleAddEntityResponse(const QByteArray& packet);
    static EntityItemID readEntityItemIDFromBuffer(const unsigned char* data, int bytesLeftToRead);
    
private:
    friend class EntityTree;
    EntityItemID assignActualIDForToken() const; // only called by EntityTree

    static uint32_t _nextCreatorTokenID; /// used by the static interfaces for creator token ids
    static QHash<uint32_t, QUuid> _tokenIDsToIDs;
};

inline bool operator<(const EntityItemID& a, const EntityItemID& b) {
    return (a.id == b.id) ? (a.creatorTokenID < b.creatorTokenID) : (a.id < b.id);
}

inline bool operator==(const EntityItemID& a, const EntityItemID& b) {
    if (a.id == UNKNOWN_ENTITY_ID || b.id == UNKNOWN_ENTITY_ID) {
        return a.creatorTokenID == b.creatorTokenID;
    }
    return a.id == b.id;
}

inline uint qHash(const EntityItemID& a, uint seed) {
    QUuid temp;
    if (a.id == UNKNOWN_ENTITY_ID) {
        temp = QUuid(a.creatorTokenID,0,0,0,0,0,0,0,0,0,0);
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
