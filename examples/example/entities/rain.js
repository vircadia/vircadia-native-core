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
        dropSize = { x: 0.1, y: 0.1, z: 0.1 },
        dropLifetime = 60,                                      // Seconds
        debug = false,                                          // Display origin circle
        // Other
        squallCircle,
        SQUALL_CIRCLE_COLOR = { red: 255, green: 0, blue: 0 },
        SQUALL_CIRCLE_ALPHA = 0.5,
        SQUALL_CIRCLE_ROTATION = Quat.fromPitchYawRollDegrees(90, 0, 0),
        raindropProperties,
        RAINDROP_MODEL_URL = "https://s3.amazonaws.com/hifi-public/ozan/Polyworld/Sets/sky/tetrahedron_v1-Faceted2.fbx",
        raindropTimer;

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

        if (properties.hasOwnProperty("dropSize")) {
            dropSize = properties.dropSize;
        }

        if (properties.hasOwnProperty("dropLifetime")) {
            dropLifetime = properties.dropLifetime;
        }

        if (properties.hasOwnProperty("debug")) {
            debug = properties.debug;
        }

        raindropProperties = {
            type: "Model",
            modelURL: RAINDROP_MODEL_URL,
            lifetime: dropLifetime,
            dimensions: dropSize
        };
    }

    function createRaindrop() {
        raindropProperties.position = Vec3.sum(origin, { x: 0, y: -0.1, z: 0 });
        Entities.addEntity(raindropProperties);
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

        raindropTimer = Script.setInterval(createRaindrop, 3000);
    }

    function tearDown() {
        Script.clearInterval(raindropTimer);
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
    dropSize: { x: 0.1, y: 0.1, z: 0.1 },
    dropLifetime: 2,
    debug: true
});