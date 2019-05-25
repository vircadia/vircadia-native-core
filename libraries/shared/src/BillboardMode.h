//
//  Created by Sam Gondelman on 11/30/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BillboardMode_h
#define hifi_BillboardMode_h

#include "QString"

/**jsdoc
 * <p>How an entity is billboarded.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"none"</code></td><td>The entity will not be billboarded.</td></tr>
 *     <tr><td><code>"yaw"</code></td><td>The entity will yaw, but not pitch, to face the camera. Its actual rotation will be 
 *       ignored.</td></tr>
 *     <tr><td><code>"full"</code></td><td>The entity will be billboarded to face the camera. Its actual rotation will be 
 *       ignored.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} BillboardMode
 */

enum class BillboardMode {
    NONE = 0,
    YAW,
    FULL
};

class BillboardModeHelpers {
public:
    static QString getNameForBillboardMode(BillboardMode mode);
};

#endif // hifi_BillboardMode_h

