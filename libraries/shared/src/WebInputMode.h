//
//  Created by Sam Gondelman on 1/9/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WebInputMode_h
#define hifi_WebInputMode_h

#include "QString"

/*@jsdoc
 * <p>Specifies how a web surface processes events.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"touch"</code></td><td>Events are processed as touch events.</td></tr>
 *     <tr><td><code>"mouse"</code></td><td>Events are processed as mouse events.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} WebInputMode
 */

enum class WebInputMode {
    TOUCH = 0,
    MOUSE,
};

class WebInputModeHelpers {
public:
    static QString getNameForWebInputMode(WebInputMode mode);
};

#endif // hifi_WebInputMode_h

