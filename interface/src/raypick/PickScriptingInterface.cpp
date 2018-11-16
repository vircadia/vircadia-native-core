//
//  Created by Sam Gondelman 10/20/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PickScriptingInterface.h"

#include <QVariant>
#include "GLMHelpers.h"

#include "Application.h"
#include <PickManager.h>

#include "RayPick.h"
#include "StylusPick.h"
#include "ParabolaPick.h"
#include "CollisionPick.h"

#include "SpatialParentFinder.h"
#include "PickTransformNode.h"
#include "MouseTransformNode.h"
#include "avatar/MyAvatarHeadTransformNode.h"
#include "avatar/AvatarManager.h"
#include "NestableTransformNode.h"
#include "avatars-renderer/AvatarTransformNode.h"
#include "ui/overlays/OverlayTransformNode.h"
#include "EntityTransformNode.h"

#include <ScriptEngine.h>

static const float WEB_TOUCH_Y_OFFSET = 0.105f;  // how far forward (or back with a negative number) to slide stylus in hand
static const glm::vec3 TIP_OFFSET = glm::vec3(0.0f, StylusPick::WEB_STYLUS_LENGTH - WEB_TOUCH_Y_OFFSET, 0.0f);

unsigned int PickScriptingInterface::createPick(const PickQuery::PickType type, const QVariant& properties) {
    switch (type) {
        case PickQuery::PickType::Ray:
            return createRayPick(properties);
        case PickQuery::PickType::Stylus:
            return createStylusPick(properties);
        case PickQuery::PickType::Parabola:
            return createParabolaPick(properties);
        case PickQuery::PickType::Collision:
            return createCollisionPick(properties);
        default:
            return PickManager::INVALID_PICK_ID;
    }
}

/**jsdoc
 * A set of properties that can be passed to {@link Picks.createPick} to create a new Ray Pick.
 * @typedef {object} Picks.RayPickProperties
 * @property {boolean} [enabled=false] If this Pick should start enabled or not.  Disabled Picks do not updated their pick results.
 * @property {number} [filter=Picks.PICK_NOTHING] The filter for this Pick to use, constructed using filter flags combined using bitwise OR.
 * @property {number} [maxDistance=0.0] The max distance at which this Pick will intersect.  0.0 = no max.  < 0.0 is invalid.
 * @property {Uuid} parentID - The ID of the parent, either an avatar, an entity, an overlay, or a pick.
 * @property {number} [parentJointIndex=0] - The joint of the parent to parent to, for example, the joints on the model of an avatar. (default = 0, no joint)
 * @property {string} joint - If "Mouse," parents the pick to the mouse. If "Avatar," parents the pick to MyAvatar's head. Otherwise, parents to the joint of the given name on MyAvatar.
 * @property {Vec3} [posOffset=Vec3.ZERO] Only for Joint Ray Picks.  A local joint position offset, in meters.  x = upward, y = forward, z = lateral
 * @property {Vec3} [dirOffset=Vec3.UP] Only for Joint Ray Picks.  A local joint direction offset.  x = upward, y = forward, z = lateral
 * @property {Vec3} [position] Only for Static Ray Picks.  The world-space origin of the ray.
 * @property {Vec3} [direction=-Vec3.UP] Only for Static Ray Picks.  The world-space direction of the ray.
 */
unsigned int PickScriptingInterface::createRayPick(const QVariant& properties) {
    QVariantMap propMap = properties.toMap();

    bool enabled = false;
    if (propMap["enabled"].isValid()) {
        enabled = propMap["enabled"].toBool();
    }

    PickFilter filter = PickFilter();
    if (propMap["filter"].isValid()) {
        filter = PickFilter(propMap["filter"].toUInt());
    }

    float maxDistance = 0.0f;
    if (propMap["maxDistance"].isValid()) {
        maxDistance = propMap["maxDistance"].toFloat();
    }

    glm::vec3 position = Vectors::ZERO;
    if (propMap["position"].isValid()) {
        position = vec3FromVariant(propMap["position"]);
    } else if (propMap["posOffset"].isValid()) {
        position = vec3FromVariant(propMap["posOffset"]);
    }

    // direction has two defaults to ensure compatibility with older scripts
    // Joint ray picks had default direction = Vec3.UP
    // Static ray picks had default direction = -Vec3.UP
    glm::vec3 direction = propMap["joint"].isValid() ? Vectors::UP : -Vectors::UP;
    if (propMap["orientation"].isValid()) {
        direction = quatFromVariant(propMap["orientation"]) * Vectors::UP;
    } else if (propMap["direction"].isValid()) {
        direction = vec3FromVariant(propMap["direction"]);
    } else if (propMap["dirOffset"].isValid()) {
        direction = vec3FromVariant(propMap["dirOffset"]);
    }

    auto rayPick = std::make_shared<RayPick>(position, direction, filter, maxDistance, enabled);
    setParentTransform(rayPick, propMap);

    return DependencyManager::get<PickManager>()->addPick(PickQuery::Ray, rayPick);
}

/**jsdoc
 * A set of properties that can be passed to {@link Picks.createPick} to create a new Stylus Pick.
 * @typedef {object} Picks.StylusPickProperties
 * @property {number} [hand=-1] An integer.  0 == left, 1 == right.  Invalid otherwise.
 * @property {boolean} [enabled=false] If this Pick should start enabled or not.  Disabled Picks do not updated their pick results.
 * @property {number} [filter=Picks.PICK_NOTHING] The filter for this Pick to use, constructed using filter flags combined using bitwise OR.
 * @property {number} [maxDistance=0.0] The max distance at which this Pick will intersect.  0.0 = no max.  < 0.0 is invalid.
 */
unsigned int PickScriptingInterface::createStylusPick(const QVariant& properties) {
    QVariantMap propMap = properties.toMap();

    bilateral::Side side = bilateral::Side::Invalid;
    {
        QVariant handVar = propMap["hand"];
        if (handVar.isValid()) {
            side = bilateral::side(handVar.toInt());
        }
    }

    bool enabled = false;
    if (propMap["enabled"].isValid()) {
        enabled = propMap["enabled"].toBool();
    }

    PickFilter filter = PickFilter();
    if (propMap["filter"].isValid()) {
        filter = PickFilter(propMap["filter"].toUInt());
    }

    float maxDistance = 0.0f;
    if (propMap["maxDistance"].isValid()) {
        maxDistance = propMap["maxDistance"].toFloat();
    }

    glm::vec3 tipOffset = TIP_OFFSET;
    if (propMap["tipOffset"].isValid()) {
        tipOffset = vec3FromVariant(propMap["tipOffset"]);
    }

    return DependencyManager::get<PickManager>()->addPick(PickQuery::Stylus, std::make_shared<StylusPick>(side, filter, maxDistance, enabled, tipOffset));
}

// NOTE: Laser pointer still uses scaleWithAvatar. Until scaleWithAvatar is also deprecated for pointers, scaleWithAvatar should not be removed from the pick API.
/**jsdoc
 * A set of properties that can be passed to {@link Picks.createPick} to create a new Parabola Pick.
 * @typedef {object} Picks.ParabolaPickProperties
 * @property {boolean} [enabled=false] If this Pick should start enabled or not.  Disabled Picks do not updated their pick results.
 * @property {number} [filter=Picks.PICK_NOTHING] The filter for this Pick to use, constructed using filter flags combined using bitwise OR.
 * @property {number} [maxDistance=0.0] The max distance at which this Pick will intersect.  0.0 = no max.  < 0.0 is invalid.
 * @property {Uuid} parentID - The ID of the parent, either an avatar, an entity, an overlay, or a pick.
 * @property {number} [parentJointIndex=0] - The joint of the parent to parent to, for example, the joints on the model of an avatar. (default = 0, no joint)
 * @property {string} joint - If "Mouse," parents the pick to the mouse. If "Avatar," parents the pick to MyAvatar's head. Otherwise, parents to the joint of the given name on MyAvatar.
 * @property {Vec3} [posOffset=Vec3.ZERO] Only for Joint Parabola Picks.  A local joint position offset, in meters.  x = upward, y = forward, z = lateral
 * @property {Vec3} [dirOffset=Vec3.UP] Only for Joint Parabola Picks.  A local joint direction offset.  x = upward, y = forward, z = lateral
 * @property {Vec3} [position] Only for Static Parabola Picks.  The world-space origin of the parabola segment.
 * @property {Vec3} [direction=-Vec3.FRONT] Only for Static Parabola Picks.  The world-space direction of the parabola segment.
 * @property {number} [speed=1] The initial speed of the parabola, i.e. the initial speed of the projectile whose trajectory defines the parabola.
 * @property {Vec3} [accelerationAxis=-Vec3.UP] The acceleration of the parabola, i.e. the acceleration of the projectile whose trajectory defines the parabola, both magnitude and direction.
 * @property {boolean} [rotateAccelerationWithAvatar=true] Whether or not the acceleration axis should rotate with the avatar's local Y axis.
 * @property {boolean} [rotateAccelerationWithParent=false] Whether or not the acceleration axis should rotate with the parent's local Y axis, if available.
 * @property {boolean} [scaleWithParent=true] If true, the velocity and acceleration of the Pick will scale linearly with the parent, if available. scaleWithAvatar is an alias but is deprecated.
 */
unsigned int PickScriptingInterface::createParabolaPick(const QVariant& properties) {
    QVariantMap propMap = properties.toMap();

    bool enabled = false;
    if (propMap["enabled"].isValid()) {
        enabled = propMap["enabled"].toBool();
    }

    PickFilter filter = PickFilter();
    if (propMap["filter"].isValid()) {
        filter = PickFilter(propMap["filter"].toUInt());
    }

    float maxDistance = 0.0f;
    if (propMap["maxDistance"].isValid()) {
        maxDistance = propMap["maxDistance"].toFloat();
    }

    float speed = 1.0f;
    if (propMap["speed"].isValid()) {
        speed = propMap["speed"].toFloat();
    }

    glm::vec3 accelerationAxis = -Vectors::UP;
    if (propMap["accelerationAxis"].isValid()) {
        accelerationAxis = vec3FromVariant(propMap["accelerationAxis"]);
    }

    bool rotateAccelerationWithAvatar = true;
    if (propMap["rotateAccelerationWithAvatar"].isValid()) {
        rotateAccelerationWithAvatar = propMap["rotateAccelerationWithAvatar"].toBool();
    }

    bool rotateAccelerationWithParent = false;
    if (propMap["rotateAccelerationWithParent"].isValid()) {
        rotateAccelerationWithParent = propMap["rotateAccelerationWithParent"].toBool();
    }

    bool scaleWithParent = true;
    if (propMap["scaleWithParent"].isValid()) {
        scaleWithParent = propMap["scaleWithParent"].toBool();
    } else if (propMap["scaleWithAvatar"].isValid()) {
        scaleWithParent = propMap["scaleWithAvatar"].toBool();
    }

    glm::vec3 position = Vectors::ZERO;
    glm::vec3 direction = propMap["joint"].isValid() ? Vectors::UP : -Vectors::FRONT;
    if (propMap["position"].isValid()) {
        position = vec3FromVariant(propMap["position"]);
    } else if (propMap["posOffset"].isValid()) {
        position = vec3FromVariant(propMap["posOffset"]);
    }
    if (propMap["orientation"].isValid()) {
        direction = quatFromVariant(propMap["orientation"]) * Vectors::UP;
    } else if (propMap["direction"].isValid()) {
        direction = vec3FromVariant(propMap["direction"]);
    } else if (propMap["dirOffset"].isValid()) {
        direction = vec3FromVariant(propMap["dirOffset"]);
    }

    auto parabolaPick = std::make_shared<ParabolaPick>(position, direction, speed, accelerationAxis,
        rotateAccelerationWithAvatar, rotateAccelerationWithParent, scaleWithParent, filter, maxDistance, enabled);
    setParentTransform(parabolaPick, propMap);
    return DependencyManager::get<PickManager>()->addPick(PickQuery::Parabola, parabolaPick);
}

/**jsdoc
* A Shape defines a physical volume.
*
* @typedef {object} Shape
* @property {string} shapeType The type of shape to use. Can be one of the following: "box", "sphere", "capsule-x", "capsule-y", "capsule-z", "cylinder-x", "cylinder-y", "cylinder-z"
* @property {Vec3} dimensions - The size to scale the shape to.
*/

// TODO: Add this property to the Shape jsdoc above once model picks work properly
// * @property {string} modelURL - If shapeType is one of: "compound", "simple-hull", "simple-compound", or "static-mesh", this defines the model to load to generate the collision volume.

/**jsdoc
* A set of properties that can be passed to {@link Picks.createPick} to create a new Collision Pick.

* @typedef {object} Picks.CollisionPickProperties
* @property {boolean} [enabled=false] If this Pick should start enabled or not.  Disabled Picks do not updated their pick results.
* @property {number} [filter=Picks.PICK_NOTHING] The filter for this Pick to use, constructed using filter flags combined using bitwise OR.
* @property {Shape} shape - The information about the collision region's size and shape. Dimensions are in world space, but will scale with the parent if defined.
* @property {Vec3} position - The position of the collision region, relative to a parent if defined.
* @property {Quat} orientation - The orientation of the collision region, relative to a parent if defined.
* @property {float} threshold - The approximate minimum penetration depth for a test object to be considered in contact with the collision region.
* The depth is measured in world space, but will scale with the parent if defined.
* @property {CollisionMask} [collisionGroup=8] - The type of object this collision pick collides as. Objects whose collision masks overlap with the pick's collision group
* will be considered colliding with the pick.
* @property {Uuid} parentID - The ID of the parent, either an avatar, an entity, an overlay, or a pick.
* @property {number} [parentJointIndex=0] - The joint of the parent to parent to, for example, the joints on the model of an avatar. (default = 0, no joint)
* @property {string} joint - If "Mouse," parents the pick to the mouse. If "Avatar," parents the pick to MyAvatar's head. Otherwise, parents to the joint of the given name on MyAvatar.
* @property {boolean} [scaleWithParent=true] If true, the collision pick's dimensions and threshold will adjust according to the scale of the parent.
*/
unsigned int PickScriptingInterface::createCollisionPick(const QVariant& properties) {
    QVariantMap propMap = properties.toMap();

    bool enabled = false;
    if (propMap["enabled"].isValid()) {
        enabled = propMap["enabled"].toBool();
    }

    PickFilter filter = PickFilter();
    if (propMap["filter"].isValid()) {
        filter = PickFilter(propMap["filter"].toUInt());
    }

    float maxDistance = 0.0f;
    if (propMap["maxDistance"].isValid()) {
        maxDistance = propMap["maxDistance"].toFloat();
    }

    bool scaleWithParent = true;
    if (propMap["scaleWithParent"].isValid()) {
        scaleWithParent = propMap["scaleWithParent"].toBool();
    }

    CollisionRegion collisionRegion(propMap);
    auto collisionPick = std::make_shared<CollisionPick>(filter, maxDistance, enabled, scaleWithParent, collisionRegion, qApp->getPhysicsEngine());
    setParentTransform(collisionPick, propMap);

    return DependencyManager::get<PickManager>()->addPick(PickQuery::Collision, collisionPick);
}

void PickScriptingInterface::enablePick(unsigned int uid) {
    DependencyManager::get<PickManager>()->enablePick(uid);
}

void PickScriptingInterface::disablePick(unsigned int uid) {
    DependencyManager::get<PickManager>()->disablePick(uid);
}

void PickScriptingInterface::removePick(unsigned int uid) {
    DependencyManager::get<PickManager>()->removePick(uid);
}

QVariantMap PickScriptingInterface::getPrevPickResult(unsigned int uid) {
    QVariantMap result;
    auto pickResult = DependencyManager::get<PickManager>()->getPrevPickResult(uid);
    if (pickResult) {
        result = pickResult->toVariantMap();
    }
    return result;
}

void PickScriptingInterface::setPrecisionPicking(unsigned int uid, bool precisionPicking) {
    DependencyManager::get<PickManager>()->setPrecisionPicking(uid, precisionPicking);
}

void PickScriptingInterface::setIgnoreItems(unsigned int uid, const QScriptValue& ignoreItems) {
    DependencyManager::get<PickManager>()->setIgnoreItems(uid, qVectorQUuidFromScriptValue(ignoreItems));
}

void PickScriptingInterface::setIncludeItems(unsigned int uid, const QScriptValue& includeItems) {
    DependencyManager::get<PickManager>()->setIncludeItems(uid, qVectorQUuidFromScriptValue(includeItems));
}

bool PickScriptingInterface::isLeftHand(unsigned int uid) {
    return DependencyManager::get<PickManager>()->isLeftHand(uid);
}

bool PickScriptingInterface::isRightHand(unsigned int uid) {
    return DependencyManager::get<PickManager>()->isRightHand(uid);
}

bool PickScriptingInterface::isMouse(unsigned int uid) {
    return DependencyManager::get<PickManager>()->isMouse(uid);
}

QScriptValue pickTypesToScriptValue(QScriptEngine* engine, const PickQuery::PickType& pickType) {
    return pickType;
}

void pickTypesFromScriptValue(const QScriptValue& object, PickQuery::PickType& pickType) {
    pickType = static_cast<PickQuery::PickType>(object.toUInt16());
}

void PickScriptingInterface::registerMetaTypes(QScriptEngine* engine) {
    QScriptValue pickTypes = engine->newObject();
    auto metaEnum = QMetaEnum::fromType<PickQuery::PickType>();
    for (int i = 0; i < PickQuery::PickType::NUM_PICK_TYPES; ++i) {
        pickTypes.setProperty(metaEnum.key(i), metaEnum.value(i));
    }
    engine->globalObject().setProperty("PickType", pickTypes);

    qScriptRegisterMetaType(engine, pickTypesToScriptValue, pickTypesFromScriptValue);
}

unsigned int PickScriptingInterface::getPerFrameTimeBudget() const {
    return DependencyManager::get<PickManager>()->getPerFrameTimeBudget();
}

void PickScriptingInterface::setPerFrameTimeBudget(unsigned int numUsecs) {
    DependencyManager::get<PickManager>()->setPerFrameTimeBudget(numUsecs);
}

void PickScriptingInterface::setParentTransform(std::shared_ptr<PickQuery> pick, const QVariantMap& propMap) {
    QUuid parentUuid;
    int parentJointIndex = 0;
    auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    if (propMap["parentID"].isValid()) {
        parentUuid = propMap["parentID"].toUuid();
        if (propMap["parentJointIndex"].isValid()) {
            parentJointIndex = propMap["parentJointIndex"].toInt();
        }
    } else if (propMap["joint"].isValid()) {
        QString joint = propMap["joint"].toString();
        if (joint == "Mouse") {
            pick->parentTransform = std::make_shared<MouseTransformNode>();
            pick->setJointState(PickQuery::JOINT_STATE_MOUSE);
            return;
        } else if (joint == "Avatar") {
            pick->parentTransform = std::make_shared<MyAvatarHeadTransformNode>();
            return;
        } else {
            parentUuid = myAvatar->getSessionUUID();
            parentJointIndex = myAvatar->getJointIndex(joint);
        }
    }

    if (parentUuid == myAvatar->getSessionUUID()) {
        if (parentJointIndex == CONTROLLER_LEFTHAND_INDEX || parentJointIndex == CAMERA_RELATIVE_CONTROLLER_LEFTHAND_INDEX) {
            pick->setJointState(PickQuery::JOINT_STATE_LEFT_HAND);
        } else if (parentJointIndex == CONTROLLER_RIGHTHAND_INDEX || parentJointIndex == CAMERA_RELATIVE_CONTROLLER_RIGHTHAND_INDEX) {
            pick->setJointState(PickQuery::JOINT_STATE_RIGHT_HAND);
        }

        pick->parentTransform = std::make_shared<AvatarTransformNode>(myAvatar, parentJointIndex);
    } else if (!parentUuid.isNull()) {
        // Infer object type from parentID
        // For now, assume a QUuid is a SpatiallyNestable. This should change when picks are converted over to QUuids.
        bool success;
        std::weak_ptr<SpatiallyNestable> nestablePointer = DependencyManager::get<SpatialParentFinder>()->find(parentUuid, success, nullptr);
        auto sharedNestablePointer = nestablePointer.lock();

        if (success && sharedNestablePointer) {
            NestableType nestableType = sharedNestablePointer->getNestableType();
            if (nestableType == NestableType::Avatar) {
                pick->parentTransform = std::make_shared<AvatarTransformNode>(std::static_pointer_cast<Avatar>(sharedNestablePointer), parentJointIndex);
            } else if (nestableType == NestableType::Overlay) {
                pick->parentTransform = std::make_shared<OverlayTransformNode>(std::static_pointer_cast<Base3DOverlay>(sharedNestablePointer), parentJointIndex);
            } else if (nestableType == NestableType::Entity) {
                pick->parentTransform = std::make_shared<EntityTransformNode>(std::static_pointer_cast<EntityItem>(sharedNestablePointer), parentJointIndex);
            } else {
                pick->parentTransform = std::make_shared<NestableTransformNode>(nestablePointer, parentJointIndex);
            }
        }
    } else {
        unsigned int pickID = propMap["parentID"].toUInt();

        if (pickID != 0) {
            pick->parentTransform = std::make_shared<PickTransformNode>(pickID);
        }
    }
}
