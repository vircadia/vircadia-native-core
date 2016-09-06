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

#include <memory>
#include <QUuid>
#include <glm/glm.hpp>

class EntityItem;
class EntitySimulation;
using EntityItemPointer = std::shared_ptr<EntityItem>;
using EntityItemWeakPointer = std::weak_ptr<EntityItem>;
class EntitySimulation;
using EntitySimulationPointer = std::shared_ptr<EntitySimulation>;

enum EntityActionType {
    // keep these synchronized with actionTypeFromString and actionTypeToString
    ACTION_TYPE_NONE = 0,
    ACTION_TYPE_OFFSET = 1000,
    ACTION_TYPE_SPRING = 2000,
    ACTION_TYPE_HOLD = 3000,
    ACTION_TYPE_TRAVEL_ORIENTED = 4000
};


class EntityActionInterface {
public:
    EntityActionInterface(EntityActionType type, const QUuid& id) : _id(id), _type(type) { }
    virtual ~EntityActionInterface() { }
    const QUuid& getID() const { return _id; }
    EntityActionType getType() const { return _type; }

    bool isActive() { return _active; }

    virtual void removeFromSimulation(EntitySimulationPointer simulation) const = 0;
    virtual EntityItemWeakPointer getOwnerEntity() const = 0;
    virtual void setOwnerEntity(const EntityItemPointer ownerEntity) = 0;
    virtual bool updateArguments(QVariantMap arguments) = 0;
    virtual QVariantMap getArguments() = 0;

    virtual QByteArray serialize() const = 0;
    virtual void deserialize(QByteArray serializedArguments) = 0;

    static EntityActionType actionTypeFromString(QString actionTypeString);
    static QString actionTypeToString(EntityActionType actionType);

    virtual bool lifetimeIsOver() { return false; }
    virtual quint64 getExpires() { return 0; }

    virtual bool isMine() { return _isMine; }
    virtual void setIsMine(bool value) { _isMine = value; }

    virtual bool shouldSuppressLocationEdits() { return false; }

    virtual void prepareForPhysicsSimulation() { }

    // these look in the arguments map for a named argument.  if it's not found or isn't well formed,
    // ok will be set to false (note that it's never set to true -- set it to true before calling these).
    // if required is true, failure to extract an argument will cause a warning to be printed.
    static glm::vec3 extractVec3Argument (QString objectName, QVariantMap arguments,
                                          QString argumentName, bool& ok, bool required = true);
    static glm::quat extractQuatArgument (QString objectName, QVariantMap arguments,
                                          QString argumentName, bool& ok, bool required = true);
    static float extractFloatArgument(QString objectName, QVariantMap arguments,
                                      QString argumentName, bool& ok, bool required = true);
    static int extractIntegerArgument(QString objectName, QVariantMap arguments,
                                      QString argumentName, bool& ok, bool required = true);
    static QString extractStringArgument(QString objectName, QVariantMap arguments,
                                         QString argumentName, bool& ok, bool required = true);
    static bool extractBooleanArgument(QString objectName, QVariantMap arguments,
                                       QString argumentName, bool& ok, bool required = true);

protected:
    virtual glm::vec3 getPosition() = 0;
    virtual void setPosition(glm::vec3 position) = 0;
    virtual glm::quat getRotation() = 0;
    virtual void setRotation(glm::quat rotation) = 0;
    virtual glm::vec3 getLinearVelocity() = 0;
    virtual void setLinearVelocity(glm::vec3 linearVelocity) = 0;
    virtual glm::vec3 getAngularVelocity() = 0;
    virtual void setAngularVelocity(glm::vec3 angularVelocity) = 0;

    QUuid _id;
    EntityActionType _type;
    bool _active { false };
    bool _isMine { false }; // did this interface create / edit this action?
};


typedef std::shared_ptr<EntityActionInterface> EntityActionPointer;

QDataStream& operator<<(QDataStream& stream, const EntityActionType& entityActionType);
QDataStream& operator>>(QDataStream& stream, EntityActionType& entityActionType);

QString serializedActionsToDebugString(QByteArray data);

#endif // hifi_EntityActionInterface_h
