//
//  EntityDynamicInterface.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-6-4
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//



/*

     +-------------------------+                               +--------------------------------+
     |                         |                               |                                |
     | EntityDynamicsInterface |                               | EntityDynamicsFactoryInterface |
     |     (entities)          |                               |         (entities)             |
     +-------------------------+                               +--------------------------------+
                  |       |                                           |                   |
             +----+       +--+                                        |                   |
             |               |                                        |                   |
 +---------------------+   +----------------+       +--------------------------+     +---------------------------+
 |                     |   |                |       |                          |     |                           |
 |  AssignmentDynamics |   | ObjectDynamics |       | InterfaceDynamicsFactory |     | AssignmentDynamicsFactory |
 |(assignment client)  |   |   (physics)    |       |       (interface)        |     |   (assignment client)     |
 +---------------------+   +----------------+       +--------------------------+     +---------------------------+
                              |          |
                              |          |
  +---------------------+     |          |
  |                     |     |          |
  | btActionInterface   |     |          |
  |    (bullet)         |     |          |
  +---------------------+     |          |
                    |         |          |
                 +--------------+     +------------------+    +-------------------+
                 |              |     |                  |    |                   |
                 | ObjectAction |     | ObjectConstraint |    | btTypedConstraint |
                 |   (physics)  |     |   (physics)    --|--->|   (bullet)        |
                 +--------------+     +------------------+    +-------------------+
                         |                        |
                         |                        |
              +--------------------+     +-----------------------+
              |                    |     |                       |
              | ObjectActionTractor|     | ObjectConstraintHinge |
              |     (physics)      |     |       (physics)       |
              +--------------------+     +-----------------------+



A dynamic is a callback which is registered with bullet.  A dynamic is called-back every physics
simulation step and can do whatever it wants with the various datastructures it has available.  A
dynamic, for example, can pull an EntityItem toward a point as if that EntityItem were connected to that
point by a spring.

In this system, a dynamic is a property of an EntityItem (rather, an EntityItem has a property which
encodes a list of dynamics).  Each dynamic has a type and some arguments.  Dynamics can be created by a
script or when receiving information via an EntityTree data-stream (either over the network or from an
svo file).

In the interface, if an EntityItem has dynamics, this EntityItem will have pointers to ObjectDynamic
subclass (like ObjectDynamicTractor) instantiations.  Code in the entities library affects a dynamic-object
via the EntityDynamicInterface (which knows nothing about bullet).  When the ObjectDynamic subclass
instance is created, it is registered as a dynamic with bullet.  Bullet will call into code in this
instance with the btDynamicInterface every physics-simulation step.

Because the dynamic can exist next to the interface's EntityTree or the entity-server's EntityTree,
parallel versions of the factories and dynamics are needed.

In an entity-server, any type of dynamic is instantiated as an AssignmentDynamic.  This dynamic isn't called
by bullet (which isn't part of an assignment-client).  It does nothing but remember its type and its
arguments.  This may change as we try to make the entity-server's simple physics simulation better, but
right now the AssignmentDynamic class is a place-holder.

The dynamic-objects are instantiated by singleton (dependecy) subclasses of EntityDynamicFactoryInterface.
In the interface, the subclass is an InterfaceDynamicFactory and it will produce things like
ObjectDynamicTractor.  In an entity-server the subclass is an AssignmentDynamicFactory and it always
produces AssignmentDynamics.

Depending on the dynamic's type, it will have various arguments.  When a script changes an argument of an
dynamic, the argument-holding member-variables of ObjectDynamicTractor (in this example) are updated and
also serialized into _dynamicData in the EntityItem.  Each subclass of ObjectDynamic knows how to serialize
and deserialize its own arguments.  _dynamicData is what gets sent over the wire or saved in an svo file.
When a packet-reader receives data for _dynamicData, it will save it in the EntityItem; this causes the
deserializer in the ObjectDynamic subclass to be called with the new data, thereby updating its argument
variables.  These argument variables are used by the code which is run when bullet does a callback.


*/

#include "EntityItem.h"

#include "EntityDynamicInterface.h"


EntityDynamicType EntityDynamicInterface::dynamicTypeFromString(QString dynamicTypeString) {
    QString normalizedDynamicTypeString = dynamicTypeString.toLower().remove('-').remove('_');
    if (normalizedDynamicTypeString == "none") {
        return DYNAMIC_TYPE_NONE;
    }
    if (normalizedDynamicTypeString == "offset") {
        return DYNAMIC_TYPE_OFFSET;
    }
    if (normalizedDynamicTypeString == "spring") {
        return DYNAMIC_TYPE_TRACTOR;
    }
    if (normalizedDynamicTypeString == "tractor") {
        return DYNAMIC_TYPE_TRACTOR;
    }
    if (normalizedDynamicTypeString == "hold") {
        return DYNAMIC_TYPE_HOLD;
    }
    if (normalizedDynamicTypeString == "traveloriented") {
        return DYNAMIC_TYPE_TRAVEL_ORIENTED;
    }
    if (normalizedDynamicTypeString == "hinge") {
        return DYNAMIC_TYPE_HINGE;
    }
    if (normalizedDynamicTypeString == "fargrab") {
        return DYNAMIC_TYPE_FAR_GRAB;
    }
    if (normalizedDynamicTypeString == "slider") {
        return DYNAMIC_TYPE_SLIDER;
    }
    if (normalizedDynamicTypeString == "ballsocket") {
        return DYNAMIC_TYPE_BALL_SOCKET;
    }
    if (normalizedDynamicTypeString == "conetwist") {
        return DYNAMIC_TYPE_CONE_TWIST;
    }

    qCDebug(entities) << "Warning -- EntityDynamicInterface::dynamicTypeFromString got unknown dynamic-type name"
                      << dynamicTypeString;
    return DYNAMIC_TYPE_NONE;
}

QString EntityDynamicInterface::dynamicTypeToString(EntityDynamicType dynamicType) {
    switch(dynamicType) {
        case DYNAMIC_TYPE_NONE:
            return "none";
        case DYNAMIC_TYPE_OFFSET:
            return "offset";
        case DYNAMIC_TYPE_SPRING:
        case DYNAMIC_TYPE_TRACTOR:
            return "tractor";
        case DYNAMIC_TYPE_HOLD:
            return "hold";
        case DYNAMIC_TYPE_TRAVEL_ORIENTED:
            return "travel-oriented";
        case DYNAMIC_TYPE_HINGE:
            return "hinge";
        case DYNAMIC_TYPE_FAR_GRAB:
            return "far-grab";
        case DYNAMIC_TYPE_SLIDER:
            return "slider";
        case DYNAMIC_TYPE_BALL_SOCKET:
            return "ball-socket";
        case DYNAMIC_TYPE_CONE_TWIST:
            return "cone-twist";
    }
    assert(false);
    return "none";
}

glm::vec3 EntityDynamicInterface::extractVec3Argument(QString objectName, QVariantMap arguments,
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

glm::quat EntityDynamicInterface::extractQuatArgument(QString objectName, QVariantMap arguments,
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

float EntityDynamicInterface::extractFloatArgument(QString objectName, QVariantMap arguments,
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

int EntityDynamicInterface::extractIntegerArgument(QString objectName, QVariantMap arguments,
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

QString EntityDynamicInterface::extractStringArgument(QString objectName, QVariantMap arguments,
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

bool EntityDynamicInterface::extractBooleanArgument(QString objectName, QVariantMap arguments,
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

QDataStream& operator<<(QDataStream& stream, const EntityDynamicType& entityDynamicType) {
    return stream << (quint16)entityDynamicType;
}

QDataStream& operator>>(QDataStream& stream, EntityDynamicType& entityDynamicType) {
    quint16 dynamicTypeAsInt;
    stream >> dynamicTypeAsInt;
    entityDynamicType = (EntityDynamicType)dynamicTypeAsInt;
    return stream;
}

QString serializedDynamicsToDebugString(QByteArray data) {
    if (data.size() == 0) {
        return QString();
    }
    QVector<QByteArray> serializedDynamics;
    QDataStream serializedDynamicsStream(data);
    serializedDynamicsStream >> serializedDynamics;

    QString result;
    foreach(QByteArray serializedDynamic, serializedDynamics) {
        QDataStream serializedDynamicStream(serializedDynamic);
        EntityDynamicType dynamicType;
        QUuid dynamicID;
        serializedDynamicStream >> dynamicType;
        serializedDynamicStream >> dynamicID;
        result += EntityDynamicInterface::dynamicTypeToString(dynamicType) + "-" + dynamicID.toString() + " ";
    }

    return result;
}
