//
//  uit.js
//
//  Created by David Rowe on 22 Aug 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global UIT:true */

UIT = (function () {
    // User Interface Toolkit. Global object.

    return {
        colors: {
            white: { red: 0xff, green: 0xff, blue: 0xff },
            faintGray: { red: 0xe3, green: 0xe3, blue: 0xe3 },
            lightGrayText: { red: 0xaf, green: 0xaf, blue: 0xaf },
            lightGray: { red: 0x6a, green: 0x6a, blue: 0x6a },
            baseGray: { red: 0x40, green: 0x40, blue: 0x40 },
            baseGrayShadow: { red: 0x25, green: 0x25, blue: 0x25 },
            darkGray: { red: 0x12, green: 0x12, blue: 0x12 },
            redAccent: { red: 0xc6, green: 0x21, blue: 0x47 },
            redHighlight: { red: 0xea, green: 0x4c, blue: 0x5f },
            greenShadow: { red: 0x35, green: 0x9d, blue: 0x85 },
            greenHighlight: { red: 0x1f, green: 0xc6, blue: 0xa6 },
            blueHighlight: { red: 0x00, green: 0xbf, blue: 0xef }
        },

        // Coordinate system: UI lies in x-y plane with the front surface being +z.
        // Offsets are relative to parents' centers.
        dimensions: {
            canvas: { x: 0.24, y: 0.296 }, // Overall UI size.
            canvasSeparation: 0.004, // Gap between Tools menu and Create panel.
            handOffset: 0.085, // Distance from hand (wrist) joint to center of canvas.
            handLateralOffset: 0.01, // Offset of UI in direction of palm normal.

            topMargin: 0.010,
            leftMargin: 0.0118,
            rightMargin: 0.0118,

            header: { x: 0.24, y: 0.048, z: 0.012 },
            headerHeading: { x: 0.24, y: 0.044, z: 0.012 },
            headerBar: { x: 0.24, y: 0.004, z: 0.012 },
            panel: { x: 0.24, y: 0.236, z: 0.008 },
            footerHeight: 0.056,

            itemCollisionZone: { x: 0.0481, y: 0.0480, z: 0.0040 }, // Cursor intersection zone for Tools and Create items.

            buttonDimensions: { x: 0.2164, y: 0.0840, z: 0.0040 }, // Default size of large single options button.

            menuButtonDimensions: { x: 0.0267, y: 0.0267, z: 0.0040 },
            menuButtonIconOffset: { x: 0, y: 0.00935, z: -0.0040 }, // Non-hovered position relative to itemCollisionZone.
            menuButtonLabelYOffset: -0.00915, // Relative to itemCollisionZone.
            menuButtonSublabelYOffset: -0.01775, // Relative to itemCollisionZone.

            paletteItemButtonDimensions: { x: 0.0481, y: 0.0480, z: 0.0020 },
            paletteItemButtonOffset: { x: 0, y: 0, z: -0.0020 }, // Non-hovered position relative to itemCollisionZone.
            paletteItemButtonHoveredOffset: { x: 0, y: 0, z: -0.0010 },
            paletteItemIconDimensions: { x: 0.024, y: 0.024, z: 0.024 },
            paletteItemIconOffset: { x: 0, y: 0, z: 0.015 }, // Non-hovered position relative to palette button.

            horizontalRuleHeight : 0.0004,

            imageOverlayOffset: 0.001 // Raise image above surface.
        }
    };
}());
