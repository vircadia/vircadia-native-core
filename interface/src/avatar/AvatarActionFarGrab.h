//
//  AvatarActionFarGrab.h
//  interface/src/avatar/
//
//  Created by Seth Alves 2017-4-14
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarActionFarGrab_h
#define hifi_AvatarActionFarGrab_h

#include <EntityItem.h>
#include <ObjectActionTractor.h>

/*@jsdoc
 * The <code>"far-grab"</code> {@link Entities.ActionType|ActionType} moves and rotates an entity to a target position and 
 * orientation, optionally relative to another entity. Collisions between the entity and the user's avatar are disabled during 
 * the far-grab.
 * It has arguments in addition to the common {@link Entities.ActionArguments|ActionArguments}:
 *
 * @typedef {object} Entities.ActionArguments-FarGrab
 * @property {Uuid} otherID=null - If an entity ID, the <code>targetPosition</code> and <code>targetRotation</code> are
 *     relative to the entity's position and rotation.
 * @property {Uuid} otherJointIndex=null - If a joint index in the <code>otherID</code> entity, the <code>targetPosition</code>
 *     and <code>targetRotation</code> are relative to the entity joint's position and rotation.
 * @property {Vec3} targetPosition=0,0,0 - The target position.
 * @property {Quat} targetRotation=0,0,0,1 - The target rotation.
 * @property {number} linearTimeScale=3.4e+38 - Controls how long it takes for the entity's position to catch up with the
 *     target position. The value is the time for the action to catch up to 1/e = 0.368 of the target value, where the action 
 *     is applied using an exponential decay.
 * @property {number} angularTimeScale=3.4e+38 - Controls how long it takes for the entity's orientation to catch up with the
 *     target orientation. The value is the time for the action to catch up to 1/e = 0.368 of the target value, where the 
 *     action is applied using an exponential decay.
 */
// The properties are per ObjectActionTractor.
class AvatarActionFarGrab : public ObjectActionTractor {
public:
    AvatarActionFarGrab(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~AvatarActionFarGrab();

    QByteArray serialize() const override;
    virtual void deserialize(QByteArray serializedArguments) override;
};

#endif // hifi_AvatarActionFarGrab_h
