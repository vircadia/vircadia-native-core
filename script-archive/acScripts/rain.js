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
        dropSpinMax = 0,                                        // Maximum angular velocity per axis; deg/s
        debug = true,                                          // Display origin circle; don't use running on Stack Manager
        // Other
        squallCircle,
        SQUALL_CIRCLE_COLOR = { red: 255, green: 0, blue: 0 },
        SQUALL_CIRCLE_ALPHA = 0.5,
        SQUALL_CIRCLE_ROTATION = Quat.fromPitchYawRollDegrees(90, 0, 0),
        raindropProperties,
        RAINDROP_MODEL_URL = "https://s3.amazonaws.com/hifi-public/ozan/Polyworld/Sets/sky/tetrahedron_v1-Faceted2.fbx",
        raindropTimer,
        raindrops = [],                                         // HACK: Work around raindrops not always getting velocities
        raindropVelocities = [],                                // HACK: Work around raindrops not always getting velocities
        DEGREES_TO_RADIANS = Math.PI / 180;

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

        if (properties.hasOwnProperty("dropSpinMax")) {
            dropSpinMax = properties.dropSpinMax;
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
            angularDamping: 0,
            collisionless: true
        };
    }

    function createRaindrop() {
        var angle,
            radius,
            offset,
            drop,
            spin = { x: 0, y: 0, z: 0 },
            maxSpinRadians = properties.dropSpinMax * DEGREES_TO_RADIANS,
            i;

        // HACK: Work around rain drops not always getting velocities set at creation
        for (i = 0; i < raindrops.length; i += 1) {
            Entities.editEntity(raindrops[i], raindropVelocities[i]);
        }

        angle = Math.random() * 360;
        radius = Math.random() * squallRadius;
        offset = Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(0, angle, 0), { x: 0, y: -0.1, z: radius });
        raindropProperties.position = Vec3.sum(squallOrigin, offset);
        if (properties.dropSpinMax > 0) {
            spin = { 
                x: Math.random() * maxSpinRadians,
                y: Math.random() * maxSpinRadians,
                z: Math.random() * maxSpinRadians
            };
            raindropProperties.angularVelocity = spin;
        }
        drop = Entities.addEntity(raindropProperties);

        // HACK: Work around rain drops not always getting velocities set at creation
        raindrops.push(drop);
        raindropVelocities.push({
            velocity: raindropProperties.velocity,
            angularVelocity: spin
        });
        if (raindrops.length > 5) {
            raindrops.shift();
            raindropVelocities.shift();
        }
    }

    function setUp() {
        if (debug) {
            squallCircle = Overlays.addOverlay("circle3d", {
                size: { x: squallRadius, y: squallRadius },
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

var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));
center.y += 10;
var rainSquall1 = new RainSquall({
    origin:center,
    radius: 25,
    dropsPerMinute: 120,
    dropSize: { x: 0.1, y: 0.1, z: 0.1 },
    dropFallSpeed: 3,
    dropLifetime: 30,
    dropSpinMax: 180,
    debug: false
});
