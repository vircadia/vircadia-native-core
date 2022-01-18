//
//  Created by Bradley Austin Davis on 2015/07/16
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TextEffect_h
#define hifi_TextEffect_h

#include "QString"

/*@jsdoc
 * <p>A {@link Entities.EntityProperties-Text|Text} entity may use one of the following effects:</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"none"</code></td><td>No effect.</td></tr>
 *     <tr><td><code>"outline"</code></td><td>An outline effect.</td></tr>
 *     <tr><td><code>"outline fill"</code></td><td>An outline effect, with fill.</td></tr>
 *     <tr><td><code>"shadow"</code></td><td>A shadow effect.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} Entities.TextEffect
 */

enum class TextEffect {
    NO_EFFECT = 0,
    OUTLINE_EFFECT,
    OUTLINE_WITH_FILL_EFFECT,
    SHADOW_EFFECT
};

class TextEffectHelpers {
public:
    static QString getNameForTextEffect(TextEffect effect);
};

#endif // hifi_TextEffect_h