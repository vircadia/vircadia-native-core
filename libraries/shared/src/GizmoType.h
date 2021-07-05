//
//  Created by Sam Gondelman on 1/22/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GizmoType_h
#define hifi_GizmoType_h

#include "QString"

/*@jsdoc
 * <p>A {@link Entities.EntityProperties-Gizmo|Gizmo} entity may be one of the following types:</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"ring"</code></td><td>A ring gizmo.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} Entities.GizmoType
 */

enum GizmoType {
    RING = 0,
    // put new gizmo-types before this line.
    UNSET_GIZMO_TYPE
};

class GizmoTypeHelpers {
public:
    static QString getNameForGizmoType(GizmoType mode);
};

#endif // hifi_GizmoType_h

