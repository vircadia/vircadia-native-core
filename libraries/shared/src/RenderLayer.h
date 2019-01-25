//
//  Created by Sam Gondelman on 1/3/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderLayer_h
#define hifi_RenderLayer_h

#include "QString"

/**jsdoc
 * <p>In which layer an entity is rendered.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>world</code></td><td>The entity will be drawn in the world with everything else.</td></tr>
 *     <tr><td><code>front</code></td><td>The entity will be drawn on top of the world layer, but behind the HUD sphere.</td></tr>
 *     <tr><td><code>hud</code></td><td>The entity will be drawn on top of other layers and the HUD sphere.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} RenderLayer
 */

enum class RenderLayer {
    WORLD = 0,
    FRONT,
    HUD
};

class RenderLayerHelpers {
public:
    static QString getNameForRenderLayer(RenderLayer mode);
};

#endif // hifi_RenderLayer_h

