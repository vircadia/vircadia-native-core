//
//  Leaves.js
//  examples
//
//  Created by Bing Shearer on 14 Jul 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
var leafName = "scriptLeaf";
var leafSquall = function (properties) {
    var // Properties
    squallOrigin,
    squallRadius,
    leavesPerMinute = 60,
        leafSize = {
            x: 0.1,
            y: 0.1,
            z: 0.1
        },
        leafFallSpeed = 1, // m/s
        leafLifetime = 60, // Seconds
        leafSpinMax = 0, // Maximum angular velocity per axis; deg/s
        debug = false, // Display origin circle; don't use running on Stack Manager
        // Other
        squallCircle,
        SQUALL_CIRCLE_COLOR = {
            red: 255,
            green: 0,
            blue: 0
        },
        SQUALL_CIRCLE_ALPHA = 0.5,
        SQUALL_CIRCLE_ROTATION = Quat.fromPitchYawRollDegrees(90, 0, 0),
        leafProperties,
        leaf_MODEL_URL = "https://hifi-public.s3.amazonaws.com/ozan/support/forBing/palmLeaf.fbx",
        leafTimer,
        leaves = [], // HACK: Work around leaves not always getting velocities
        leafVelocities = [], // HACK: Work around leaves not always getting velocities
        DEGREES_TO_RADIANS = Math.PI / 180,
        leafDeleteOnTearDown = true,
        maxLeaves,
        leafCount,
        nearbyEntities,
        complexMovement = false,
        movementTime = 0,
        maxSpinRadians = properties.leafSpinMax * DEGREES_TO_RADIANS,
        windFactor,
        leafDeleteOnGround = false,
        floorHeight = null;


    function processProperties() {
        if (!properties.hasOwnProperty("origin")) {
            print("ERROR: Leaf squall origin must be specified");
            return;
        }
        squallOrigin = properties.origin;

        if (!properties.hasOwnProperty("radius")) {
            print("ERROR: Leaf squall radius must be specified");
            return;
        }
        squallRadius = properties.radius;

        if (properties.hasOwnProperty("leavesPerMinute")) {
            leavesPerMinute = properties.leavesPerMinute;
        }

        if (properties.hasOwnProperty("leafSize")) {
            leafSize = properties.leafSize;
        }

        if (properties.hasOwnProperty("leafFallSpeed")) {
            leafFallSpeed = properties.leafFallSpeed;
        }

        if (properties.hasOwnProperty("leafLifetime")) {
            leafLifetime = properties.leafLifetime;
        }

        if (properties.hasOwnProperty("leafSpinMax")) {
            leafSpinMax = properties.leafSpinMax;
        }

        if (properties.hasOwnProperty("debug")) {
            debug = properties.debug;
        }
        if (properties.hasOwnProperty("floorHeight")) {
            floorHeight = properties.floorHeight;
        }
        if (properties.hasOwnProperty("maxLeaves")) {
            maxLeaves = properties.maxLeaves;
        }
        if (properties.hasOwnProperty("complexMovement")) {
            complexMovement = properties.complexMovement;
        }
        if (properties.hasOwnProperty("leafDeleteOnGround")) {
            leafDeleteOnGround = properties.leafDeleteOnGround;
        }
        if (properties.hasOwnProperty("windFactor")) {
            windFactor = properties.windFactor;
        } else if (complexMovement == true){
            print("ERROR: Wind Factor must be defined for complex movement")
        }

        leafProperties = {
            type: "Model",
            name: leafName,
            modelURL: leaf_MODEL_URL,
            lifetime: leafLifetime,
            dimensions: leafSize,
            velocity: {
                x: 0,
                y: -leafFallSpeed,
                z: 0
            },
            damping: 0,
            angularDamping: 0,
            collisionless: true

        };
    }

    function createleaf() {
        var angle,
        radius,
        offset,
        leaf,
        spin = {
            x: 0,
            y: 0,
            z: 0
        },
        i;

        // HACK: Work around leaves not always getting velocities set at creation
        for (i = 0; i < leaves.length; i++) {
            Entities.editEntity(leaves[i], leafVelocities[i]);
        }

        angle = Math.random() * leafSpinMax;
        radius = Math.random() * squallRadius;
        offset = Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(0, angle, 0), {
            x: 0,
            y: -0.1,
            z: radius
        });
        leafProperties.position = Vec3.sum(squallOrigin, offset);
        if (properties.leafSpinMax > 0 && !complexMovement) {
            spin = {
                x: Math.random() * maxSpinRadians,
                y: Math.random() * maxSpinRadians,
                z: Math.random() * maxSpinRadians
            };
            leafProperties.angularVelocity = spin;
        } else if (complexMovement) {
            spin = {
                x: 0,
                y: 0,
                z: 0
            };
            leafProperties.angularVelocity = spin
        }
        leaf = Entities.addEntity(leafProperties);

        // HACK: Work around leaves not always getting velocities set at creation
        leaves.push(leaf);
        leafVelocities.push({
            velocity: leafProperties.velocity,
            angularVelocity: spin
        });
        if (leaves.length > 5) {
            leaves.shift();
            leafVelocities.shift();
        }
    }

    function setUp() {
        if (debug) {
            squallCircle = Overlays.addOverlay("circle3d", {
                size: {
                    x: 2 * squallRadius,
                    y: 2 * squallRadius
                },
                color: SQUALL_CIRCLE_COLOR,
                alpha: SQUALL_CIRCLE_ALPHA,
                solid: true,
                visible: debug,
                position: squallOrigin,
                rotation: SQUALL_CIRCLE_ROTATION
            });
        }

        leafTimer = Script.setInterval(function () {
            if (leafCount <= maxLeaves - 1) {
                createleaf()
            }
        }, 60000 / leavesPerMinute);
    }
    Script.setInterval(function () {
        nearbyEntities = Entities.findEntities(squallOrigin, squallRadius);
        newLeafMovement()
    }, 100);

    function newLeafMovement() { //new additions to leaf code. Operates at 10 Hz or every 100 ms
        movementTime += 0.1;
        var currentLeaf,
        randomRotationSpeed = {
            x: maxSpinRadians * Math.sin(movementTime),
            y: maxSpinRadians * Math.random(),
            z: maxSpinRadians * Math.sin(movementTime / 7)
        };
        for (var i = 0; i < nearbyEntities.length; i++) {
            var entityProperties = Entities.getEntityProperties(nearbyEntities[i]);
            var entityName = entityProperties.name;
            if (leafName === entityName) {
                currentLeaf = nearbyEntities[i];
                var leafHeight = entityProperties.position.y;
                if (complexMovement && leafHeight > floorHeight || complexMovement && floorHeight == null) { //actual new movement code;
                    var leafCurrentVel = entityProperties.velocity,
                        leafCurrentRot = entityProperties.rotation,
                        yVec = {
                            x: 0,
                            y: 1,
                            z: 0
                        },
                        leafYinWFVec = Vec3.multiplyQbyV(leafCurrentRot, yVec),
                        leafLocalHorVec = Vec3.cross(leafYinWFVec, yVec),
                        leafMostDownVec = Vec3.cross(leafYinWFVec, leafLocalHorVec),
                        leafDesiredVel = Vec3.multiply(leafMostDownVec, windFactor),
                        leafVelDelt = Vec3.subtract(leafDesiredVel, leafCurrentVel),
                        leafNewVel = Vec3.sum(leafCurrentVel, Vec3.multiply(leafVelDelt, windFactor));
                    Entities.editEntity(currentLeaf, {
                        angularVelocity: randomRotationSpeed,
                        velocity: leafNewVel
                    })
                } else if (leafHeight <= floorHeight) {
                    if (!leafDeleteOnGround) {
                        Entities.editEntity(nearbyEntities[i], {
                            locked: false,
                            velocity: {
                                x: 0,
                                y: 0,
                                z: 0
                            },
                            angularVelocity: {
                                x: 0,
                                y: 0,
                                z: 0
                            }
                        })
                    } else {
                        Entity.deleteEntity(currentLeaf);
                    }
                }
            }
        }
    }



    getLeafCount = Script.setInterval(function () {
        leafCount = 0
        for (var i = 0; i < nearbyEntities.length; i++) {
            var entityName = Entities.getEntityProperties(nearbyEntities[i]).name;
            //Stop Leaves at floorHeight
            if (leafName === entityName) {
                leafCount++;
                if (i == nearbyEntities.length - 1) {
                    //print(leafCount);
                }
            }
        }
    }, 1000)



    function tearDown() {
        Script.clearInterval(leafTimer);
        Overlays.deleteOverlay(squallCircle);
        if (leafDeleteOnTearDown) {
            for (var i = 0; i < nearbyEntities.length; i++) {
                var entityName = Entities.getEntityProperties(nearbyEntities[i]).name;
                if (leafName === entityName) {
                    //We have a match - delete this entity
                    Entities.editEntity(nearbyEntities[i], {
                        locked: false
                    });
                    Entities.deleteEntity(nearbyEntities[i]);
                }
            }
        }
    }



    processProperties();
    setUp();
    Script.scriptEnding.connect(tearDown);

    return {};
};

var leafSquall1 = new leafSquall({
    origin: {
        x: 3071.5,
        y: 2170,
        z: 6765.3
    },
    radius: 100,
    leavesPerMinute: 30,
    leafSize: {
        x: 0.3,
        y: 0.00,
        z: 0.3
    },
    leafFallSpeed: 0.4,
    leafLifetime: 100,
    leafSpinMax: 30,
    debug: false,
    maxLeaves: 100,
    leafDeleteOnTearDown: true,
    complexMovement: true,
    floorHeight: 2143.5,
    windFactor: 0.5,
    leafDeleteOnGround: false
});

// todo
//deal with depth issue
