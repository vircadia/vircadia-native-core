//
//  Created by Sam Gondelman on 5/31/19
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ComponentMode.h"

/*@jsdoc
 * <p>How an effect is applied in a {@link Entities.EntityProperties-Zone|Zone} entity.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"inherit"</code></td><td>The effect from any enclosing zone continues into this zone.</td></tr>
 *     <tr><td><code>"disabled"</code></td><td>The effect &mdash; from any enclosing zone and this zone &mdash; is disabled in 
 *       this zone.</td></tr>
 *     <tr><td><code>"enabled"</code></td><td>The effect from this zone is enabled, overriding the effect from any enclosing 
 *       zone.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} Entities.ComponentMode
 */
const char* componentModeNames[] = {
    "inherit",
    "disabled",
    "enabled"
};

QString ComponentModeHelpers::getNameForComponentMode(ComponentMode mode) {
    if (((int)mode <= 0) || ((int)mode >= (int)COMPONENT_MODE_ITEM_COUNT)) {
        mode = (ComponentMode)0;
    }

    return componentModeNames[(int)mode];
}

/*@jsdoc
 * <p>The priority of updates from avatars in a zone to other clients.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"inherit"</code></td><td>The update priority from any enclosing zone continues into this zone.</td></tr>
 *     <tr><td><code>"crowd"</code></td><td>The update priority in this zone is the normal priority.</td></tr>
 *     <tr><td><code>"hero"</code></td><td>Avatars in this zone have an increased update priority.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} Entities.AvatarPriorityMode
 */
const char* avatarPriorityModeNames[] = {
    "inherit",
    "crowd",
    "hero"
};

QString AvatarPriorityModeHelpers::getNameForAvatarPriorityMode(AvatarPriorityMode mode) {
    if (((int)mode <= 0) || ((int)mode >= (int)AVATAR_PRIORITY_ITEM_COUNT)) {
        mode = (AvatarPriorityMode)0;
    }

    return avatarPriorityModeNames[(int)mode];
}