//
//  PhysicsCollisionGroups.h
//  libraries/shared/src
//
//  Created by Andrew Meadows 2015.06.03
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PhysicsCollisionGroups_h
#define hifi_PhysicsCollisionGroups_h

#include <stdint.h>

/* Note: These are the Bullet collision groups defined in btBroadphaseProxy. Only 
 * DefaultFilter and StaticFilter are explicitly used by Bullet (when the collision 
 * filter of an object is not manually specified), the rest are merely suggestions.
 *
enum CollisionFilterGroups {
    DefaultFilter = 1,
    StaticFilter = 2,
    KinematicFilter = 4,
    DebrisFilter = 8,
    SensorTrigger = 16,
    CharacterFilter = 32,
    AllFilter = -1
}
 *
 * When using custom collision filters we pretty much need to do all or nothing. 
 * We'll be doing it all which means we define our own groups and build custom masks 
 * for everything.
 *
*/

const int16_t COLLISION_GROUP_DEFAULT = 1 << 0;
const int16_t COLLISION_GROUP_STATIC = 1 << 1;
const int16_t COLLISION_GROUP_KINEMATIC = 1 << 2;
const int16_t COLLISION_GROUP_DEBRIS = 1 << 3;
const int16_t COLLISION_GROUP_TRIGGER = 1 << 4;
const int16_t COLLISION_GROUP_MY_AVATAR = 1 << 5;
const int16_t COLLISION_GROUP_OTHER_AVATAR = 1 << 6;
const int16_t COLLISION_GROUP_MY_ATTACHMENT = 1 << 7;
const int16_t COLLISION_GROUP_OTHER_ATTACHMENT = 1 << 8;
// ...
const int16_t COLLISION_GROUP_COLLISIONLESS = 1 << 14;


/* Note: In order for objectA to collide with objectB at the filter stage 
 * both (groupA & maskB) and (groupB & maskA) must be non-zero.
 */

// DEFAULT collides with everything except COLLISIONLESS
const int16_t COLLISION_MASK_DEFAULT = ~ COLLISION_GROUP_COLLISIONLESS;

// STATIC also doesn't collide with other STATIC
const int16_t COLLISION_MASK_STATIC = ~ (COLLISION_GROUP_COLLISIONLESS | COLLISION_GROUP_STATIC);

const int16_t COLLISION_MASK_KINEMATIC = COLLISION_MASK_DEFAULT;

// DEBRIS also doesn't collide with other DEBRIS, or TRIGGER
const int16_t COLLISION_MASK_DEBRIS = ~ (COLLISION_GROUP_COLLISIONLESS
        | COLLISION_GROUP_DEBRIS
        | COLLISION_GROUP_TRIGGER);

// TRIGGER also doesn't collide with DEBRIS, TRIGGER, or STATIC (TRIGGER only detects moveable things that matter)
const int16_t COLLISION_MASK_TRIGGER = COLLISION_MASK_DEBRIS & ~(COLLISION_GROUP_STATIC);

// AVATAR also doesn't collide with corresponding ATTACHMENTs
const int16_t COLLISION_MASK_MY_AVATAR = ~(COLLISION_GROUP_COLLISIONLESS | COLLISION_GROUP_MY_ATTACHMENT);
const int16_t COLLISION_MASK_MY_ATTACHMENT = ~(COLLISION_GROUP_COLLISIONLESS | COLLISION_GROUP_MY_AVATAR);
const int16_t COLLISION_MASK_OTHER_AVATAR = ~(COLLISION_GROUP_COLLISIONLESS | COLLISION_GROUP_OTHER_ATTACHMENT);
const int16_t COLLISION_MASK_OTHER_ATTACHMENT = ~(COLLISION_GROUP_COLLISIONLESS | COLLISION_GROUP_OTHER_AVATAR);

// COLLISIONLESS gets an empty mask.
const int16_t COLLISION_MASK_COLLISIONLESS = 0;

#endif // hifi_PhysicsCollisionGroups_h
