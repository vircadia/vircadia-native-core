//
//  rain.js
//  examples
//
//  Created by David Rowe on 9 Apr 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var RainSquall = function (properties) {
    var // Properties
        origin,
        radius,
        debug = false,                                          // Display origin circle
        // Other
        squallCircle,
        SQUALL_CIRCLE_COLOR = { red: 255, green: 0, blue: 0 },
        SQUALL_CIRCLE_ALPHA = 0.3,
        SQUALL_CIRCLE_ROTATION = Quat.fromPitchYawRollDegrees(90, 0, 0);

    function processProperties() {
        if (!properties.hasOwnProperty("origin")) {
            print("ERROR: Rain squall origin must be specified");
            return;
        }
        origin = properties.origin;

        if (!properties.hasOwnProperty("radius")) {
            print("ERROR: Rain squall radius must be specified");
            return;
        }
        radius = properties.radius;

        if (properties.hasOwnProperty("debug")) {
            debug = properties.debug;
        }
    }

    function setUp() {
        squallCircle = Overlays.addOverlay("circle3d", {
            size: { x: radius, y: radius },
            color: SQUALL_CIRCLE_COLOR,
            alpha: SQUALL_CIRCLE_ALPHA,
            solid: true,
            visible: debug,
            position: origin,
            rotation: SQUALL_CIRCLE_ROTATION
        });
    }

    function tearDown() {
        Overlays.deleteOverlay(squallCircle);
    }

    processProperties();
    setUp();
    Script.scriptEnding.connect(tearDown);

    return {
    };
};

var rainSquall1 = new RainSquall({
    origin: { x: 8192, y: 8200, z: 8192 },
    radius: 2.5,
    debug: true
});