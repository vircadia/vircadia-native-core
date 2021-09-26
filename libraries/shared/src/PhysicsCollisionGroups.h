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

const int32_t BULLET_COLLISION_GROUP_STATIC = 1 << 0;
const int32_t BULLET_COLLISION_GROUP_DYNAMIC = 1 << 1;
const int32_t BULLET_COLLISION_GROUP_KINEMATIC = 1 << 2;
const int32_t BULLET_COLLISION_GROUP_MY_AVATAR = 1 << 3;
const int32_t BULLET_COLLISION_GROUP_OTHER_AVATAR = 1 << 4;
const int32_t BULLET_COLLISION_GROUP_DETAILED_AVATAR = 1 << 5;
const int32_t BULLET_COLLISION_GROUP_DETAILED_RAY = 1 << 6;
// ...
const int32_t BULLET_COLLISION_GROUP_COLLISIONLESS = 1 << 31;


/* Note: In order for objectA to collide with objectB at the filter stage
 * both (groupA & maskB) and (groupB & maskA) must be non-zero.
 */

// the default collision mask is: collides with everything except collisionless
const int32_t BULLET_COLLISION_MASK_DEFAULT = ~ BULLET_COLLISION_GROUP_COLLISIONLESS;

// STATIC does not collide with itself (as optimization of physics simulation)
const int32_t BULLET_COLLISION_MASK_STATIC = ~ (BULLET_COLLISION_GROUP_COLLISIONLESS | BULLET_COLLISION_GROUP_KINEMATIC | BULLET_COLLISION_GROUP_STATIC);

const int32_t BULLET_COLLISION_MASK_DYNAMIC = BULLET_COLLISION_MASK_DEFAULT;
const int32_t BULLET_COLLISION_MASK_KINEMATIC = BULLET_COLLISION_MASK_STATIC;

// MY_AVATAR does not collide with itself
const int32_t BULLET_COLLISION_MASK_MY_AVATAR = ~(BULLET_COLLISION_GROUP_COLLISIONLESS | BULLET_COLLISION_GROUP_MY_AVATAR);

// OTHER_AVATARs are dynamic, but are slammed to whatever the avatar-mixer says, which means
// their motion can't actually be affected by the local physics simulation -- we rely on the remote simulation
// to move its avatar around correctly and to communicate its motion through the avatar-mixer.
// Therefore, they only need to collide against things that can be affected by their motion: dynamic and MyAvatar
const int32_t BULLET_COLLISION_MASK_OTHER_AVATAR = BULLET_COLLISION_GROUP_DYNAMIC | BULLET_COLLISION_GROUP_MY_AVATAR;
const int32_t BULLET_COLLISION_MASK_DETAILED_AVATAR = BULLET_COLLISION_GROUP_DETAILED_RAY;
const int32_t BULLET_COLLISION_MASK_DETAILED_RAY = BULLET_COLLISION_GROUP_DETAILED_AVATAR;
// COLLISIONLESS gets an empty mask.
const int32_t BULLET_COLLISION_MASK_COLLISIONLESS = 0;

/*@jsdoc
 * <p>A collision may occur with the following types of items:</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>1</code></td><td>Static entities &mdash; non-dynamic entities with no velocity.</td></tr>
 *     <tr><td><code>2</code></td><td>Dynamic entities &mdash; entities that have their <code>dynamic</code> property set to
 *         <code>true</code>.</td></tr>
 *     <tr><td><code>4</code></td><td>Kinematic entities &mdash; non-dynamic entities with velocity.</td></tr>
 *     <tr><td><code>8</code></td><td>My avatar.</td></tr>
 *     <tr><td><code>16</code></td><td>Other avatars.</td></tr>
 *   </tbody>
 * </table>
 * <p>The values for the collision types that are enabled are added together to give the CollisionMask value. For example, a
 * value of <code>31</code> means that an entity will collide with all item types.</p>
 * @typedef {number} CollisionMask
 */

// The USER collision groups are exposed to script and can be used to generate per-object collision masks.
// They are not necessarily the same as the BULLET_COLLISION_GROUPS, but we start them off with matching numbers.
const uint16_t USER_COLLISION_GROUP_STATIC = 1 << 0;
const uint16_t USER_COLLISION_GROUP_DYNAMIC = 1 << 1;
const uint16_t USER_COLLISION_GROUP_KINEMATIC = 1 << 2;
const uint16_t USER_COLLISION_GROUP_MY_AVATAR = 1 << 3;
const uint16_t USER_COLLISION_GROUP_OTHER_AVATAR = 1 << 4;

const uint16_t ENTITY_COLLISION_MASK_DEFAULT =
    USER_COLLISION_GROUP_STATIC |
    USER_COLLISION_GROUP_DYNAMIC |
    USER_COLLISION_GROUP_KINEMATIC |
    USER_COLLISION_GROUP_MY_AVATAR |
    USER_COLLISION_GROUP_OTHER_AVATAR;

const uint16_t USER_COLLISION_MASK_AVATARS = USER_COLLISION_GROUP_MY_AVATAR | USER_COLLISION_GROUP_OTHER_AVATAR;
const uint16_t USER_COLLISION_MASK_ENTITIES = USER_COLLISION_GROUP_STATIC | USER_COLLISION_GROUP_DYNAMIC | USER_COLLISION_GROUP_KINEMATIC;

const int32_t NUM_USER_COLLISION_GROUPS = 5;

#endif // hifi_PhysicsCollisionGroups_h
