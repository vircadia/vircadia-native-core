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

#include <QDebug>
#include <QObject>
#include <QHash>
#include <QScriptEngine>
#include <QUuid>

const QUuid UNKNOWN_ENTITY_ID; // null uuid

/// Abstract ID for editing model items. Used in EntityItem JS API.
class EntityItemID : public QUuid {
public:
    EntityItemID();
    EntityItemID(const QUuid& id);
    // EntityItemID(const EntityItemID& other);
    static EntityItemID readEntityItemIDFromBuffer(const unsigned char* data, int bytesLeftToRead);
    QScriptValue toScriptValue(QScriptEngine* engine) const;

    bool isInvalidID() const { return *this == UNKNOWN_ENTITY_ID; }
};

inline QDebug operator<<(QDebug debug, const EntityItemID& id) {
    debug << "[entity-id:" << id.toString() << "]";
    return debug;
}

Q_DECLARE_METATYPE(EntityItemID);
Q_DECLARE_METATYPE(QVector<EntityItemID>);
QScriptValue EntityItemIDtoScriptValue(QScriptEngine* engine, const EntityItemID& properties);
void EntityItemIDfromScriptValue(const QScriptValue &object, EntityItemID& properties);
QVector<EntityItemID> qVectorEntityItemIDFromScriptValue(const QScriptValue& array);

#endif // hifi_EntityItemID_h
