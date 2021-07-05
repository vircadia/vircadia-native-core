//
//  Created by Sam Gondelman on 1/7/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PrimitiveMode_h
#define hifi_PrimitiveMode_h

#include "QString"

/*@jsdoc
 * <p>How the geometry of an entity is rendered.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"solid"</code></td><td>The entity is drawn as a solid shape.</td></tr>
 *     <tr><td><code>"lines"</code></td><td>The entity is drawn as wireframe.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} Entities.PrimitiveMode
 */

enum class PrimitiveMode {
    SOLID = 0,
    LINES
};

class PrimitiveModeHelpers {
public:
    static QString getNameForPrimitiveMode(PrimitiveMode mode);
};

#endif // hifi_PrimitiveMode_h

