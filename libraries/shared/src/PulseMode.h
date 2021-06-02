//
//  Created by Sam Gondelman on 1/15/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PulseMode_h
#define hifi_PulseMode_h

#include "QString"

/*@jsdoc
 * <p>Pulse modes for color and alpha pulsing.</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"none"</code></td><td>No pulsing.</td></tr>
 *     <tr><td><code>"in"</code></td><td>Pulse in phase with the pulse period.</td></tr>
 *     <tr><td><code>"out"</code></td><td>Pulse out of phase with the pulse period.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {string} Entities.PulseMode
 */

enum class PulseMode {
    NONE = 0,
    IN_PHASE,
    OUT_PHASE
};

class PulseModeHelpers {
public:
    static QString getNameForPulseMode(PulseMode mode);
};

#endif // hifi_PulseMode_h

