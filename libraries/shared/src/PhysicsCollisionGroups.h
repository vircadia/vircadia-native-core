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

const int16_t BULLET_COLLISION_GROUP_STATIC = 1 << 0;
const int16_t BULLET_COLLISION_GROUP_DYNAMIC = 1 << 1;
const int16_t BULLET_COLLISION_GROUP_KINEMATIC = 1 << 2;
const int16_t BULLET_COLLISION_GROUP_MY_AVATAR = 1 << 3;
const int16_t BULLET_COLLISION_GROUP_OTHER_AVATAR = 1 << 4;
// ...
const int16_t BULLET_COLLISION_GROUP_COLLISIONLESS = 1 << 14;


/* Note: In order for objectA to collide with objectB at the filter stage
 * both (groupA & maskB) and (groupB & maskA) must be non-zero.
 */

// the default collision mask is: collides with everything except collisionless
const int16_t BULLET_COLLISION_MASK_DEFAULT = ~ BULLET_COLLISION_GROUP_COLLISIONLESS;

// STATIC does not collide with itself (as optimization of physics simulation)
const int16_t BULLET_COLLISION_MASK_STATIC = ~ (BULLET_COLLISION_GROUP_COLLISIONLESS | BULLET_COLLISION_GROUP_KINEMATIC | BULLET_COLLISION_GROUP_STATIC);

const int16_t BULLET_COLLISION_MASK_DYNAMIC = BULLET_COLLISION_MASK_DEFAULT;
const int16_t BULLET_COLLISION_MASK_KINEMATIC = BULLET_COLLISION_MASK_STATIC;

// MY_AVATAR does not collide with itself
const int16_t BULLET_COLLISION_MASK_MY_AVATAR = ~(BULLET_COLLISION_GROUP_COLLISIONLESS | BULLET_COLLISION_GROUP_MY_AVATAR);

const int16_t BULLET_COLLISION_MASK_OTHER_AVATAR = BULLET_COLLISION_MASK_DEFAULT;

// COLLISIONLESS gets an empty mask.
const int16_t BULLET_COLLISION_MASK_COLLISIONLESS = 0;


// The USER collision groups are exposed to script and can be used to generate per-object collision masks.
// They are not necessarily the same as the BULLET_COLLISION_GROUPS, but we start them off with matching numbers.
const uint8_t USER_COLLISION_GROUP_STATIC = 1 << 0;
const uint8_t USER_COLLISION_GROUP_DYNAMIC = 1 << 1;
const uint8_t USER_COLLISION_GROUP_KINEMATIC = 1 << 2;
const uint8_t USER_COLLISION_GROUP_MY_AVATAR = 1 << 3;
const uint8_t USER_COLLISION_GROUP_OTHER_AVATAR = 1 << 4;

const uint8_t ENTITY_COLLISION_MASK_DEFAULT =
    USER_COLLISION_GROUP_STATIC |
    USER_COLLISION_GROUP_DYNAMIC |
    USER_COLLISION_GROUP_KINEMATIC |
    USER_COLLISION_GROUP_MY_AVATAR |
    USER_COLLISION_GROUP_OTHER_AVATAR;

const uint8_t USER_COLLISION_MASK_AVATARS = USER_COLLISION_GROUP_MY_AVATAR | USER_COLLISION_GROUP_OTHER_AVATAR;

const int NUM_USER_COLLISION_GROUPS = 5;

#endif // hifi_PhysicsCollisionGroups_h
