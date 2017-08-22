//
//  uit.js
//
//  Created by David Rowe on 22 Aug 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global UIT */

UIT = (function () {
    // User Interface Toolkit. Global object.

    return {
        colors: {
            baseGray: { red: 0x40, green: 0x40, blue: 0x40 },
            blueHighlight: { red: 0x00, green: 0xbf, blue: 0xef },
            white: { red: 0xff, green: 0xff, blue: 0xff }
        },

        // Coordinate system: UI lies in x-y plane with the front surface being +z.
        dimensions: {
            canvas: { x: 0.24, y: 0.24 },  // Overall UI size.
            canvasSeparation: 0.01,  // Gap between Tools menu and Create panel.
            handOffset: 0.085,  // Distance from hand (wrist) joint to center of canvas.
            handLateralOffset: 0.01,  // Offset of UI in direction of palm normal.

            header: { x: 0.24, y: 0.044, z: 0.012 },
            headerBar: { x: 0.24, y: 0.004, z: 0.012 },
            panel: { x: 0.24, y: 0.18, z: 0.008 },

            imageOffset: 0.001  // Raise image above surface.
        }
    };
}());
