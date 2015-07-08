//
//  EntityActionInterface.h
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityActionInterface_h
#define hifi_EntityActionInterface_h

#include <QUuid>

#include "EntityItem.h"

class EntitySimulation;

enum EntityActionType {
    // keep these synchronized with actionTypeFromString and actionTypeToString
    ACTION_TYPE_NONE = 0,
    ACTION_TYPE_OFFSET = 1000,
    ACTION_TYPE_SPRING = 2000,
    ACTION_TYPE_HOLD = 3000
};


class EntityActionInterface {
public:
    EntityActionInterface(EntityActionType type, const QUuid& id) : _id(id), _type(type) { }
    virtual ~EntityActionInterface() { }
    const QUuid& getID() const { return _id; }
    EntityActionType getType() const { return _type; }

    virtual void removeFromSimulation(EntitySimulation* simulation) const = 0;
    virtual EntityItemWeakPointer getOwnerEntity() const = 0;
    virtual void setOwnerEntity(const EntityItemPointer ownerEntity) = 0;
    virtual bool updateArguments(QVariantMap arguments) = 0;
    virtual QVariantMap getArguments() = 0;

    virtual QByteArray serialize() const = 0;
    virtual void deserialize(QByteArray serializedArguments) = 0;

    static EntityActionType actionTypeFromString(QString actionTypeString);
    static QString actionTypeToString(EntityActionType actionType);

protected:
    virtual glm::vec3 getPosition() = 0;
    virtual void setPosition(glm::vec3 position) = 0;
    virtual glm::quat getRotation() = 0;
    virtual void setRotation(glm::quat rotation) = 0;
    virtual glm::vec3 getLinearVelocity() = 0;
    virtual void setLinearVelocity(glm::vec3 linearVelocity) = 0;
    virtual glm::vec3 getAngularVelocity() = 0;
    virtual void setAngularVelocity(glm::vec3 angularVelocity) = 0;

    // these look in the arguments map for a named argument.  if it's not found or isn't well formed,
    // ok will be set to false (note that it's never set to true -- set it to true before calling these).
    // if required is true, failure to extract an argument will cause a warning to be printed.
    static glm::vec3 extractVec3Argument (QString objectName, QVariantMap arguments,
                                          QString argumentName, bool& ok, bool required = true);
    static glm::quat extractQuatArgument (QString objectName, QVariantMap arguments,
                                          QString argumentName, bool& ok, bool required = true);
    static float extractFloatArgument(QString objectName, QVariantMap arguments,
                                      QString argumentName, bool& ok, bool required = true);
    static QString extractStringArgument(QString objectName, QVariantMap arguments,
                                         QString argumentName, bool& ok, bool required = true);

    QUuid _id;
    EntityActionType _type;
};


typedef std::shared_ptr<EntityActionInterface> EntityActionPointer;

QDataStream& operator<<(QDataStream& stream, const EntityActionType& entityActionType);
QDataStream& operator>>(QDataStream& stream, EntityActionType& entityActionType);

#endif // hifi_EntityActionInterface_h
