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
        squallOrigin,
        squallRadius,
        dropsPerMinute = 60,
        dropSize = { x: 0.1, y: 0.1, z: 0.1 },
        dropFallSpeed = 1,                                      // m/s
        dropLifetime = 60,                                      // Seconds
        debug = false,                                          // Display origin circle; don't use running on Stack Manager
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
        squallOrigin = properties.origin;

        if (!properties.hasOwnProperty("radius")) {
            print("ERROR: Rain squall radius must be specified");
            return;
        }
        squallRadius = properties.radius;

        if (properties.hasOwnProperty("dropsPerMinute")) {
            dropsPerMinute = properties.dropsPerMinute;
        }

        if (properties.hasOwnProperty("dropSize")) {
            dropSize = properties.dropSize;
        }

        if (properties.hasOwnProperty("dropFallSpeed")) {
            dropFallSpeed = properties.dropFallSpeed;
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
            dimensions: dropSize,
            velocity: { x: 0, y: -dropFallSpeed, z: 0 },
            damping: 0,
            ignoreForCollisions: true
        };
    }

    function createRaindrop() {
        var angle,
            radius,
            offset;

        angle = Math.random() * 360;
        radius = Math.random() * squallRadius;
        offset = Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(0, angle, 0), { x: 0, y: -0.1, z: radius });
        raindropProperties.position = Vec3.sum(squallOrigin, offset);
        Entities.addEntity(raindropProperties);
    }

    function setUp() {
        if (debug) {
            squallCircle = Overlays.addOverlay("circle3d", {
                size: { x: 2 * squallRadius, y: 2 * squallRadius },
                color: SQUALL_CIRCLE_COLOR,
                alpha: SQUALL_CIRCLE_ALPHA,
                solid: true,
                visible: debug,
                position: squallOrigin,
                rotation: SQUALL_CIRCLE_ROTATION
            });
        }

        raindropTimer = Script.setInterval(createRaindrop, 60000 / dropsPerMinute);
    }

    function tearDown() {
        Script.clearInterval(raindropTimer);
        if (debug) {
            Overlays.deleteOverlay(squallCircle);
        }
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
    dropsPerMinute: 120,
    dropSize: { x: 0.1, y: 0.1, z: 0.1 },
    dropFallSpeed: 2,  // m/s
    dropLifetime: 10,
    debug: true
});