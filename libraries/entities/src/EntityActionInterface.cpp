//
//  EntityActionInterface.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-6-4
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//



/*




     +-----------------------+        +-------------------+      +------------------------------+
     |                       |        |                   |      |                              |
     | EntityActionInterface |        | btActionInterface |      | EntityActionFactoryInterface |
     |     (entities)        |        |    (bullet)       |      |         (entities)           |
     +-----------------------+        +-------------------+      +------------------------------+
                  |       |                    |                      |                   |
             +----+       +--+      +----------+                      |                   |
             |               |      |                                 |                   |
 +-------------------+    +--------------+         +------------------------+     +-------------------------+
 |                   |    |              |         |                        |     |                         |
 |  AssignmentAction |    | ObjectAction |         | InterfaceActionFactory |     | AssignmentActionFactory |
 |(assignment client)|    |   (physics)  |         |       (interface)      |     |   (assignment client)   |
 +-------------------+    +--------------+         +------------------------+     +-------------------------+
                                 |
                                 |
                                 |
                       +--------------------+
                       |                    |
                       | ObjectActionSpring |
                       |     (physics)      |
                       +--------------------+




An action is a callback which is registered with bullet.  An action is called-back every physics
simulation step and can do whatever it wants with the various datastructures it has available.  An
action, for example, can pull an EntityItem toward a point as if that EntityItem were connected to that
point by a spring.

In this system, an action is a property of an EntityItem (rather, an EntityItem has a property which
encodes a list of actions).  Each action has a type and some arguments.  Actions can be created by a
script or when receiving information via an EntityTree data-stream (either over the network or from an
svo file).

In the interface, if an EntityItem has actions, this EntityItem will have pointers to ObjectAction
subclass (like ObjectActionSpring) instantiations.  Code in the entities library affects an action-object
via the EntityActionInterface (which knows nothing about bullet).  When the ObjectAction subclass
instance is created, it is registered as an action with bullet.  Bullet will call into code in this
instance with the btActionInterface every physics-simulation step.

Because the action can exist next to the interface's EntityTree or the entity-server's EntityTree,
parallel versions of the factories and actions are needed.

In an entity-server, any type of action is instantiated as an AssignmentAction.  This action isn't called
by bullet (which isn't part of an assignment-client).  It does nothing but remember its type and its
arguments.  This may change as we try to make the entity-server's simple physics simulation better, but
right now the AssignmentAction class is a place-holder.

The action-objects are instantiated by singleton (dependecy) subclasses of EntityActionFactoryInterface.
In the interface, the subclass is an InterfaceActionFactory and it will produce things like
ObjectActionSpring.  In an entity-server the subclass is an AssignmentActionFactory and it always
produces AssignmentActions.

Depending on the action's type, it will have various arguments.  When a script changes an argument of an
action, the argument-holding member-variables of ObjectActionSpring (in this example) are updated and
also serialized into _actionData in the EntityItem.  Each subclass of ObjectAction knows how to serialize
and deserialize its own arguments.  _actionData is what gets sent over the wire or saved in an svo file.
When a packet-reader receives data for _actionData, it will save it in the EntityItem; this causes the
deserializer in the ObjectAction subclass to be called with the new data, thereby updating its argument
variables.  These argument variables are used by the code which is run when bullet does a callback.


 */

#include "EntityItem.h"

#include "EntityActionInterface.h"


EntityActionType EntityActionInterface::actionTypeFromString(QString actionTypeString) {
    QString normalizedActionTypeString = actionTypeString.toLower().remove('-').remove('_');
    if (normalizedActionTypeString == "none") {
        return ACTION_TYPE_NONE;
    }
    if (normalizedActionTypeString == "offset") {
        return ACTION_TYPE_OFFSET;
    }
    if (normalizedActionTypeString == "spring") {
        return ACTION_TYPE_SPRING;
    }
    if (normalizedActionTypeString == "hold") {
        return ACTION_TYPE_HOLD;
    }
    if (normalizedActionTypeString == "traveloriented") {
        return ACTION_TYPE_TRAVEL_ORIENTED;
    }

    qCDebug(entities) << "Warning -- EntityActionInterface::actionTypeFromString got unknown action-type name" << actionTypeString;
    return ACTION_TYPE_NONE;
}

QString EntityActionInterface::actionTypeToString(EntityActionType actionType) {
    switch(actionType) {
        case ACTION_TYPE_NONE:
            return "none";
        case ACTION_TYPE_OFFSET:
            return "offset";
        case ACTION_TYPE_SPRING:
            return "spring";
        case ACTION_TYPE_HOLD:
            return "hold";
        case ACTION_TYPE_TRAVEL_ORIENTED:
            return "travel-oriented";
    }
    assert(false);
    return "none";
}

glm::vec3 EntityActionInterface::extractVec3Argument(QString objectName, QVariantMap arguments,
                                                     QString argumentName, bool& ok, bool required) {
    if (!arguments.contains(argumentName)) {
        if (required) {
            qCDebug(entities) << objectName << "requires argument:" << argumentName;
        }
        ok = false;
        return glm::vec3(0.0f);
    }

    QVariant resultV = arguments[argumentName];
    if (resultV.type() != (QVariant::Type) QMetaType::QVariantMap) {
        qCDebug(entities) << objectName << "argument" << argumentName << "must be a map";
        ok = false;
        return glm::vec3(0.0f);
    }

    QVariantMap resultVM = resultV.toMap();
    if (!resultVM.contains("x") || !resultVM.contains("y") || !resultVM.contains("z")) {
        qCDebug(entities) << objectName << "argument" << argumentName << "must be a map with keys: x, y, z";
        ok = false;
        return glm::vec3(0.0f);
    }

    QVariant xV = resultVM["x"];
    QVariant yV = resultVM["y"];
    QVariant zV = resultVM["z"];

    bool xOk = true;
    bool yOk = true;
    bool zOk = true;
    float x = xV.toFloat(&xOk);
    float y = yV.toFloat(&yOk);
    float z = zV.toFloat(&zOk);
    if (!xOk || !yOk || !zOk) {
        qCDebug(entities) << objectName << "argument" << argumentName << "must be a map with keys: x, y, and z of type float.";
        ok = false;
        return glm::vec3(0.0f);
    }

    if (x != x || y != y || z != z) {
        // at least one of the values is NaN
        ok = false;
        return glm::vec3(0.0f);
    }

    return glm::vec3(x, y, z);
}

glm::quat EntityActionInterface::extractQuatArgument(QString objectName, QVariantMap arguments,
                                                     QString argumentName, bool& ok, bool required) {
    if (!arguments.contains(argumentName)) {
        if (required) {
            qCDebug(entities) << objectName << "requires argument:" << argumentName;
        }
        ok = false;
        return glm::quat();
    }

    QVariant resultV = arguments[argumentName];
    if (resultV.type() != (QVariant::Type) QMetaType::QVariantMap) {
        qCDebug(entities) << objectName << "argument" << argumentName << "must be a map, not" << resultV.typeName();
        ok = false;
        return glm::quat();
    }

    QVariantMap resultVM = resultV.toMap();
    if (!resultVM.contains("x") || !resultVM.contains("y") || !resultVM.contains("z") || !resultVM.contains("w")) {
        qCDebug(entities) << objectName << "argument" << argumentName << "must be a map with keys: x, y, z, and w";
        ok = false;
        return glm::quat();
    }

    QVariant xV = resultVM["x"];
    QVariant yV = resultVM["y"];
    QVariant zV = resultVM["z"];
    QVariant wV = resultVM["w"];

    bool xOk = true;
    bool yOk = true;
    bool zOk = true;
    bool wOk = true;
    float x = xV.toFloat(&xOk);
    float y = yV.toFloat(&yOk);
    float z = zV.toFloat(&zOk);
    float w = wV.toFloat(&wOk);
    if (!xOk || !yOk || !zOk || !wOk) {
        qCDebug(entities) << objectName << "argument" << argumentName
                 << "must be a map with keys: x, y, z, and w of type float.";
        ok = false;
        return glm::quat();
    }

    if (x != x || y != y || z != z || w != w) {
        // at least one of the components is NaN!
        ok = false;
        return glm::quat();
    }

    return glm::normalize(glm::quat(w, x, y, z));
}

float EntityActionInterface::extractFloatArgument(QString objectName, QVariantMap arguments,
                                                  QString argumentName, bool& ok, bool required) {
    if (!arguments.contains(argumentName)) {
        if (required) {
            qCDebug(entities) << objectName << "requires argument:" << argumentName;
        }
        ok = false;
        return 0.0f;
    }

    QVariant variant = arguments[argumentName];
    bool variantOk = true;
    float value = variant.toFloat(&variantOk);

    if (!variantOk || std::isnan(value)) {
        ok = false;
        return 0.0f;
    }

    return value;
}

int EntityActionInterface::extractIntegerArgument(QString objectName, QVariantMap arguments,
                                                  QString argumentName, bool& ok, bool required) {
    if (!arguments.contains(argumentName)) {
        if (required) {
            qCDebug(entities) << objectName << "requires argument:" << argumentName;
        }
        ok = false;
        return 0.0f;
    }

    QVariant variant = arguments[argumentName];
    bool variantOk = true;
    int value = variant.toInt(&variantOk);

    if (!variantOk) {
        ok = false;
        return 0;
    }

    return value;
}

QString EntityActionInterface::extractStringArgument(QString objectName, QVariantMap arguments,
                                                     QString argumentName, bool& ok, bool required) {
    if (!arguments.contains(argumentName)) {
        if (required) {
            qCDebug(entities) << objectName << "requires argument:" << argumentName;
        }
        ok = false;
        return "";
    }
    return arguments[argumentName].toString();
}

bool EntityActionInterface::extractBooleanArgument(QString objectName, QVariantMap arguments,
                                                   QString argumentName, bool& ok, bool required) {
    if (!arguments.contains(argumentName)) {
        if (required) {
            qCDebug(entities) << objectName << "requires argument:" << argumentName;
        }
        ok = false;
        return false;
    }
    return arguments[argumentName].toBool();
}



QDataStream& operator<<(QDataStream& stream, const EntityActionType& entityActionType)
{
    return stream << (quint16)entityActionType;
}

QDataStream& operator>>(QDataStream& stream, EntityActionType& entityActionType)
{
    quint16 actionTypeAsInt;
    stream >> actionTypeAsInt;
    entityActionType = (EntityActionType)actionTypeAsInt;
    return stream;
}

QString serializedActionsToDebugString(QByteArray data) {
    if (data.size() == 0) {
        return QString();
    }
    QVector<QByteArray> serializedActions;
    QDataStream serializedActionsStream(data);
    serializedActionsStream >> serializedActions;

    QString result;
    foreach(QByteArray serializedAction, serializedActions) {
        QDataStream serializedActionStream(serializedAction);
        EntityActionType actionType;
        QUuid actionID;
        serializedActionStream >> actionType;
        serializedActionStream >> actionID;
        result += EntityActionInterface::actionTypeToString(actionType) + "-" + actionID.toString() + " ";
    }

    return result;
}
