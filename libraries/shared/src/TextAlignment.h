//
//  Created by HifiExperiments on 2/9/21
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TextAlignment_h
#define hifi_TextAlignment_h

#include "QString"

/*@jsdoc
 * <p>A {@link Entities.EntityProperties-Text|Text} entity may use one of the following alignments:</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"left"</code></td><td>Text is aligned to the left side.</td></tr>
 *     <tr><td><code>"center"</code></td><td>Text is centered.</td></tr>
 *     <tr><td><code>"right"</code></td><td>Text is aligned to the right side.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} Entities.TextAlignment
 */

enum class TextAlignment {
    LEFT = 0,
    CENTER,
    RIGHT
};

class TextAlignmentHelpers {
public:
    static QString getNameForTextAlignment(TextAlignment alignment);
};

#endif // hifi_TextAlignment_h