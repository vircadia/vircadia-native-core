//
//  EntityDynamicInterface.h
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityDynamicInterface_h
#define hifi_EntityDynamicInterface_h

#include <memory>
#include <QUuid>
#include <glm/glm.hpp>

class EntityItem;
class EntityItemID;
class EntitySimulation;
using EntityItemPointer = std::shared_ptr<EntityItem>;
using EntityItemWeakPointer = std::weak_ptr<EntityItem>;
class EntitySimulation;
using EntitySimulationPointer = std::shared_ptr<EntitySimulation>;

enum EntityDynamicType {
    // keep these synchronized with dynamicTypeFromString and dynamicTypeToString
    DYNAMIC_TYPE_NONE = 0,
    DYNAMIC_TYPE_OFFSET = 1000,
    DYNAMIC_TYPE_SPRING = 2000,
    DYNAMIC_TYPE_TRACTOR = 2100,
    DYNAMIC_TYPE_HOLD = 3000,
    DYNAMIC_TYPE_TRAVEL_ORIENTED = 4000,
    DYNAMIC_TYPE_HINGE = 5000,
    DYNAMIC_TYPE_FAR_GRAB = 6000,
    DYNAMIC_TYPE_SLIDER = 7000,
    DYNAMIC_TYPE_BALL_SOCKET = 8000,
    DYNAMIC_TYPE_CONE_TWIST = 9000
};


class EntityDynamicInterface {
public:
    EntityDynamicInterface(EntityDynamicType type, const QUuid& id) : _id(id), _type(type) { }
    virtual ~EntityDynamicInterface() { }
    const QUuid& getID() const { return _id; }
    EntityDynamicType getType() const { return _type; }

    virtual void remapIDs(QHash<EntityItemID, EntityItemID>& map) = 0;

    virtual bool isAction() const { return false; }
    virtual bool isConstraint() const { return false; }
    virtual bool isReadyForAdd() const { return true; }

    bool isActive() { return _active; }

    virtual void removeFromSimulation(EntitySimulationPointer simulation) const = 0;
    virtual EntityItemWeakPointer getOwnerEntity() const = 0;
    virtual void setOwnerEntity(const EntityItemPointer ownerEntity) = 0;
    virtual bool updateArguments(QVariantMap arguments) = 0;
    virtual QVariantMap getArguments() = 0;

    virtual QByteArray serialize() const = 0;
    virtual void deserialize(QByteArray serializedArguments) = 0;

    static EntityDynamicType dynamicTypeFromString(QString dynamicTypeString);
    static QString dynamicTypeToString(EntityDynamicType dynamicType);

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
    QUuid _id;
    EntityDynamicType _type;
    bool _active { false };
    bool _isMine { false }; // did this interface create / edit this dynamic?
};


typedef std::shared_ptr<EntityDynamicInterface> EntityDynamicPointer;

QDataStream& operator<<(QDataStream& stream, const EntityDynamicType& entityDynamicType);
QDataStream& operator>>(QDataStream& stream, EntityDynamicType& entityDynamicType);

QString serializedDynamicsToDebugString(QByteArray data);

#endif // hifi_EntityDynamicInterface_h
