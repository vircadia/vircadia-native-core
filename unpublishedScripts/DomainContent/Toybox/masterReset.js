//  masterReset.js
//  Created by Eric Levin on 9/23/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var utilitiesScript = Script.resolvePath("libraries/utils.js");
Script.include(utilitiesScript);

var gunScriptURL = Script.resolvePath("pistol/pistol.js");
var sprayPaintScriptURL = Script.resolvePath("spray_paint/sprayPaintCan.js");
var catScriptURL = Script.resolvePath("cat/cat.js");
var flashlightScriptURL = Script.resolvePath('flashlight/flashlight.js');
var pingPongScriptURL = Script.resolvePath('ping_pong_gun/pingPongGun.js');
var wandScriptURL = Script.resolvePath("bubblewand/wand.js");
var dollScriptURL = Script.resolvePath("doll/doll.js");
var lightsScriptURL = Script.resolvePath("lights/lightSwitch.js");
var targetsScriptURL = Script.resolvePath('ping_pong_gun/wallTarget.js');
var bowScriptURL = Script.resolvePath('bow/bow.js');
var raveStickEntityScriptURL = Script.resolvePath("flowArts/raveStick/raveStickEntityScript.js");
var basketballResetterScriptURL = Script.resolvePath('basketballsResetter.js');
var targetsResetterScriptURL = Script.resolvePath('targetsResetter.js');


MasterReset = function() {
    var resetKey = "resetMe";


    var shouldDeleteOnEndScript = false;

    //Before creating anything, first search a radius and delete all the things that should be deleted
    deleteAllToys();
    createAllToys();



    function createAllToys() {
        createBlocks({
            x: 548.3,
            y: 495.55,
            z: 504.4
        });

        createBasketBall({
            x: 547.73,
            y: 495.5,
            z: 505.47
        });

        createDoll({
            x: 546.67,
            y: 495.41,
            z: 505.09
        });

        createGun({
            x: 546.2,
            y: 495.5,
            z: 505.2
        });


        createWand({
            x: 546.71,
            y: 495.55,
            z: 506.15
        });

        createDice();

        createFlashlight({
            x: 545.72,
            y: 495.41,
            z: 505.78
        });

        createRaveStick({
            x: 547.4,
            y: 495.4,
            z: 504.5
        });



        createCombinedArmChair({
            x: 549.29,
            y: 494.9,
            z: 508.22
        });

        createPottedPlant({
            x: 554.26,
            y: 495.2,
            z: 504.53
        });



        createPingPongBallGun();
        createTargets();
        createTargetResetter();

        createBasketballRack();
        createBasketballResetter();

        createGates();

        createFire();
        // Handles toggling of all sconce lights 
        createLights();



        createCat({
            x: 551.0,
            y: 495.3,
            z: 503.3
        });

        createSprayCan({
            x: 549.7,
            y: 495.6,
            z: 503.91
        });


        createBow();
    }

    function deleteAllToys() {
        var entities = Entities.findEntities(MyAvatar.position, 100);

        entities.forEach(function(entity) {
            //params: customKey, id, defaultValue
            var shouldReset = getEntityCustomData(resetKey, entity, {}).resetMe;
            if (shouldReset === true) {
                Entities.deleteEntity(entity);
            }
        });
    }

    function createRaveStick(position) {
        var modelURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flowArts/raveStick.fbx";
        var rotation = Quat.fromPitchYawRollDegrees(0, 0, 0);
        var stick = Entities.addEntity({
            type: "Model",
            name: "raveStick",
            modelURL: modelURL,
            rotation: rotation,
            position: position,
            shapeType: 'box',
            dynamic: true,
            script: raveStickEntityScriptURL,
            dimensions: {
                x: 0.06,
                y: 0.06,
                z: 0.31
            },
            gravity: {
                x: 0,
                y: -3,
                z: 0
            },
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                },
                grabbableKey: {
                    spatialKey: {
                        rightRelativePosition: {
                            x: 0.02,
                            y: 0,
                            z: 0
                        },
                        leftRelativePosition: {
                            x: -0.02,
                            y: 0,
                            z: 0
                        },
                        relativeRotation: Quat.fromPitchYawRollDegrees(90, 90, 0)
                    },
                    invertSolidWhileHeld: true
                }
            })
        });

        var forwardVec = Quat.getFront(rotation);
        var forwardQuat = Quat.rotationBetween(Vec3.UNIT_Z, forwardVec);
        var color = {
            red: 150,
            green: 20,
            blue: 100
        }
        var raveGlowEmitter = Entities.addEntity({
            type: "ParticleEffect",
            name: "Rave Stick Glow Emitter",
            position: position,
            parentID: stick,
            isEmitting: true,
            colorStart: color,
            color: {
                red: 200,
                green: 200,
                blue: 255
            },
            colorFinish: color,
            maxParticles: 100000,
            lifespan: 0.8,
            emitRate: 1000,
            emitOrientation: forwardQuat,
            emitSpeed: 0.2,
            speedSpread: 0.0,
            emitDimensions: {
                x: 0,
                y: 0,
                z: 0
            },
            polarStart: 0,
            polarFinish: 0,
            azimuthStart: 0.1,
            azimuthFinish: 0.01,
            emitAcceleration: {
                x: 0,
                y: 0,
                z: 0
            },
            accelerationSpread: {
                x: 0.00,
                y: 0.00,
                z: 0.00
            },
            radiusStart: 0.01,
            radiusFinish: 0.005,
            alpha: 0.7,
            alphaSpread: 0.1,
            alphaStart: 0.1,
            alphaFinish: 0.1,
            textures: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flowArts/beamParticle.png",
            emitterShouldTrail: false,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                }
            })
        });
    }

    function createGun(position) {
        var modelURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/pistol/gun.fbx";


        var pistol = Entities.addEntity({
            type: 'Model',
            name: "pistol",
            modelURL: modelURL,
            position: position,
            dimensions: {
                x: 0.05,
                y: 0.23,
                z: 0.36
            },
            script: gunScriptURL,
            color: {
                red: 200,
                green: 0,
                blue: 20
            },
            shapeType: 'box',
            gravity: {
                x: 0,
                y: -5.0,
                z: 0
            },
            restitution: 0,
            dynamic: true,
            damping: 0.5,
            collisionSoundURL: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/pistol/drop.wav",
            userData: JSON.stringify({
                "wearable": {
                    "joints": {
                        "RightHand": [{
                            "x": 0.07079616189002991,
                            "y": 0.20177987217903137,
                            "z": 0.06374628841876984
                        }, {
                            "x": -0.5863648653030396,
                            "y": -0.46007341146469116,
                            "z": 0.46949487924575806,
                            "w": -0.4733745753765106
                        }],
                        "LeftHand": [{
                            "x": 0.0012094751000404358,
                            "y": 0.1991066336631775,
                            "z": 0.079972043633461
                        }, {
                            "x": 0.29249316453933716,
                            "y": -0.6115763187408447,
                            "z": 0.5668558478355408,
                            "w": 0.46807748079299927
                        }]
                    }
                },
                "grabbableKey": {
                    "invertSolidWhileHeld": true
                },
                "resetMe": {
                    "resetMe": true
                }
            })
        });
    }

    function createBow() {

        var startPosition = {
            x: 546.41,
            y: 495.33,
            z: 506.46
        };

        var SCRIPT_URL = Script.resolvePath('bow.js');
        var BOW_ROTATION = Quat.fromPitchYawRollDegrees(-103.05, -178.60, -87.27);
        var MODEL_URL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/bow/bow-deadly.fbx";
        var COLLISION_HULL_URL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/bow/bow_collision_hull.obj";
        var BOW_DIMENSIONS = {
            x: 0.04,
            y: 1.3,
            z: 0.21
        };

        var BOW_GRAVITY = {
            x: 0,
            y: -9.8,
            z: 0
        };

        var TOP_NOTCH_OFFSET = 0.6;

        var BOTTOM_NOTCH_OFFSET = 0.6;

        var LINE_DIMENSIONS = {
            x: 5,
            y: 5,
            z: 5
        };

        var bow;

        function makeBow() {

            var bowProperties = {
                name: 'Hifi-Bow',
                type: "Model",
                modelURL: MODEL_URL,
                position: startPosition,
                dimensions: BOW_DIMENSIONS,
                dynamic: true,
                gravity: BOW_GRAVITY,
                rotation: BOW_ROTATION,
                shapeType: 'compound',
                compoundShapeURL: COLLISION_HULL_URL,
                script: bowScriptURL,
                userData: JSON.stringify({
                    resetMe: {
                        resetMe: true
                    },
                    grabbableKey: {
                        invertSolidWhileHeld: true
                    },
                    wearable: {
                        joints: {
                            RightHand: [{
                                x: 0.03960523009300232,
                                y: 0.01979270577430725,
                                z: 0.03294898942112923
                            }, {
                                x: -0.7257906794548035,
                                y: -0.4611682891845703,
                                z: 0.4436084032058716,
                                w: -0.25251442193984985
                            }],
                            LeftHand: [{
                                x: 0.0055799782276153564,
                                y: 0.04354757443070412,
                                z: 0.05119767785072327
                            }, {
                                x: -0.14914104342460632,
                                y: 0.6448180079460144,
                                z: -0.2888556718826294,
                                w: -0.6917579770088196
                            }]
                        }
                    }
                })
            }
            bow = Entities.addEntity(bowProperties);
            createPreNotchString();
        }
        var preNotchString;

        function createPreNotchString() {

            var bowProperties = Entities.getEntityProperties(bow, ["position", "rotation", "userData"]);
            var downVector = Vec3.multiply(-1, Quat.getUp(bowProperties.rotation));
            var downOffset = Vec3.multiply(downVector, BOTTOM_NOTCH_OFFSET * 2);
            var upVector = Quat.getUp(bowProperties.rotation);
            var upOffset = Vec3.multiply(upVector, TOP_NOTCH_OFFSET);

            var backOffset = Vec3.multiply(-0.1, Quat.getFront(bowProperties.rotation));
            var topStringPosition = Vec3.sum(bowProperties.position, upOffset);
            topStringPosition = Vec3.sum(topStringPosition, backOffset);

            var stringProperties = {
                name: 'Hifi-Bow-Pre-Notch-String',
                type: 'Line',
                position: topStringPosition,
                rotation: Quat.fromPitchYawRollDegrees(164.6, 164.5, -72),
                linePoints: [{
                    x: 0,
                    y: 0,
                    z: 0
                }, Vec3.sum({
                    x: 0,
                    y: 0,
                    z: 0
                }, downOffset)],
                color: {
                    red: 255,
                    green: 255,
                    blue: 255
                },
                dimensions: LINE_DIMENSIONS,
                visible: true,
                dynamic: false,
                collisionless: true,
                parentID: bow,
                userData: JSON.stringify({
                    grabbableKey: {
                        grabbable: false
                    }
                })
            };

            preNotchString = Entities.addEntity(stringProperties);

            var data = {
                preNotchString: preNotchString
            };

            setEntityCustomData('bowKey', bow, data);
        }

        makeBow();
    }

    function createFire() {


        var myOrientation = Quat.fromPitchYawRollDegrees(-90, 0, 0.0);

        var animationSettings = JSON.stringify({
            fps: 30,
            running: true,
            loop: true,
            firstFrame: 1,
            lastFrame: 10000
        });


        var fire = Entities.addEntity({
            type: "ParticleEffect",
            name: "fire",
            animationSettings: animationSettings,
            textures: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/fire/Particle-Sprite-Smoke-1.png",
            position: {
                x: 551.45,
                y: 494.82,
                z: 502.05
            },
            emitRate: 100,
            colorStart: {
                red: 70,
                green: 70,
                blue: 137
            },
            color: {
                red: 200,
                green: 99,
                blue: 42
            },
            colorFinish: {
                red: 255,
                green: 99,
                blue: 32
            },
            radiusSpread: 0.01,
            radiusStart: 0.02,
            radiusEnd: 0.001,
            particleRadius: 0.05,
            radiusFinish: 0.0,
            emitOrientation: myOrientation,
            emitSpeed: 0.3,
            speedSpread: 0.1,
            alphaStart: 0.05,
            alpha: 0.1,
            alphaFinish: 0.05,
            emitDimensions: {
                x: 1,
                y: 1,
                z: 0.1
            },
            polarFinish: 0.1,
            emitAcceleration: {
                x: 0.0,
                y: 0.0,
                z: 0.0
            },
            accelerationSpread: {
                x: 0.1,
                y: 0.01,
                z: 0.1
            },
            lifespan: 1,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                }
            })
        });
    }


    function createBasketballRack() {
        var NUMBER_OF_BALLS = 4;
        var DIAMETER = 0.30;
        var RESET_DISTANCE = 1;
        var MINIMUM_MOVE_LENGTH = 0.05;
        var basketballURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/basketball/basketball2.fbx";
        var basketballCollisionSoundURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/basketball/basketball.wav";
        var rackURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/basketball/basketball_rack.fbx";
        var rackCollisionHullURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/basketball/rack_collision_hull.obj";

        var rackRotation = Quat.fromPitchYawRollDegrees(0, -90, 0);

        var rackStartPosition = {
            x: 542.86,
            y: 494.84,
            z: 475.06
        };
        var rack = Entities.addEntity({
            name: 'Basketball Rack',
            type: "Model",
            modelURL: rackURL,
            position: rackStartPosition,
            rotation: rackRotation,
            shapeType: 'compound',
            gravity: {
                x: 0,
                y: -9.8,
                z: 0
            },
            damping: 1,
            dimensions: {
                x: 0.4,
                y: 1.37,
                z: 1.73
            },
            dynamic: true,
            collisionless: false,
            compoundShapeURL: rackCollisionHullURL,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                },
                grabbableKey: {
                    grabbable: false,
                    wantsTrigger: true
                }
            })
        });



        function createCollidingBalls() {
            var position = rackStartPosition;
            var collidingBalls = [];

            var i;
            for (i = 0; i < NUMBER_OF_BALLS; i++) {
                var ballPosition = {
                    x: position.x,
                    y: position.y + DIAMETER * 2,
                    z: position.z + (DIAMETER) - (DIAMETER * i)
                };
                var newPosition = {
                    x: position.x + (DIAMETER * 2) - (DIAMETER * i),
                    y: position.y + DIAMETER * 2,
                    z: position.z
                };
                var collidingBall = Entities.addEntity({
                    type: "Model",
                    name: 'Hifi-Basketball',
                    shapeType: 'Sphere',
                    position: newPosition,
                    dimensions: {
                        x: DIAMETER,
                        y: DIAMETER,
                        z: DIAMETER
                    },
                    restitution: 1.0,
                    damping: 0.00001,
                    gravity: {
                        x: 0,
                        y: -9.8,
                        z: 0
                    },
                    dynamic: true,
                    collisionSoundURL: 'https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/basketball/basketball.wav',
                    collisionless: false,
                    modelURL: basketballURL,
                    userData: JSON.stringify({
                        originalPositionKey: {
                            originalPosition: newPosition
                        },
                        resetMe: {
                            resetMe: true
                        },
                        grabbableKey: {
                            invertSolidWhileHeld: true
                        }
                    })
                });

                collidingBalls.push(collidingBall);

            }
        }

        createCollidingBalls();

    }


    function createBasketballResetter() {

        var position = {
            x: 543.58,
            y: 495.47,
            z: 469.59
        };

        var dimensions = {
            x: 1.65,
            y: 1.71,
            z: 1.75
        };

        var resetter = Entities.addEntity({
            type: "Box",
            position: position,
            name: "Basketball Resetter",
            script: basketballResetterScriptURL,
            dimensions: dimensions,
            visible: false,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                },
                grabbableKey: {
                    wantsTrigger: true
                }
            })
        });


    }

    function createTargetResetter() {
        var dimensions = {
            x: 0.21,
            y: 0.61,
            z: 0.21
        };

        var position = {
            x: 548.42,
            y: 496.40,
            z: 509.61
        };

        var resetter = Entities.addEntity({
            type: "Box",
            position: position,
            name: "Target Resetter",
            script: targetsResetterScriptURL,
            dimensions: dimensions,
            visible: false,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                },
                grabbableKey: {
                    wantsTrigger: true
                }
            })

        });
    }



    function createTargets() {


        var MODEL_URL = 'https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/ping_pong_gun/target.fbx';
        var COLLISION_HULL_URL = 'https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/ping_pong_gun/target_collision_hull.obj';

        var MINIMUM_MOVE_LENGTH = 0.05;
        var RESET_DISTANCE = 0.5;
        var TARGET_USER_DATA_KEY = 'hifi-ping_pong_target';
        var NUMBER_OF_TARGETS = 6;
        var TARGETS_PER_ROW = 3;

        var TARGET_DIMENSIONS = {
            x: 0.06,
            y: 0.42,
            z: 0.42
        };

        var VERTICAL_SPACING = TARGET_DIMENSIONS.y + 0.5;
        var HORIZONTAL_SPACING = TARGET_DIMENSIONS.z + 0.5;


        var startPosition = {
            x: 548.68,
            y: 497.30,
            z: 509.74
        };

        var rotation = Quat.fromPitchYawRollDegrees(0, -55.25, 0);

        var targets = [];

        function addTargets() {
            var i;
            var row = -1;
            for (i = 0; i < NUMBER_OF_TARGETS; i++) {

                if (i % TARGETS_PER_ROW === 0) {
                    row++;
                }

                var vHat = Quat.getFront(rotation);
                var spacer = HORIZONTAL_SPACING * (i % TARGETS_PER_ROW) + (row * HORIZONTAL_SPACING / 2);
                var multiplier = Vec3.multiply(spacer, vHat);
                var position = Vec3.sum(startPosition, multiplier);
                position.y = startPosition.y - (row * VERTICAL_SPACING);

                var targetProperties = {
                    name: 'Hifi-Target',
                    type: 'Model',
                    modelURL: MODEL_URL,
                    shapeType: 'compound',
                    dynamic: true,
                    dimensions: TARGET_DIMENSIONS,
                    compoundShapeURL: COLLISION_HULL_URL,
                    position: position,
                    rotation: rotation,
                    script: targetsScriptURL,
                    userData: JSON.stringify({
                        originalPositionKey: {
                            originalPosition: position
                        },
                        resetMe: {
                            resetMe: true
                        },
                        grabbableKey: {
                            grabbable: false
                        }
                    })
                };

                var target = Entities.addEntity(targetProperties);
                targets.push(target);

            }
        }

        addTargets();

    }

    function createCat(position) {

        var modelURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/cat/Dark_Cat.fbx";
        var animationURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/cat/sleeping.fbx";
        var animationSettings = JSON.stringify({
            running: true
        });
        var cat = Entities.addEntity({
            type: "Model",
            modelURL: modelURL,
            name: "cat",
            script: catScriptURL,
            animationURL: animationURL,
            animationSettings: animationSettings,
            position: position,
            rotation: {
                w: 0.35020983219146729,
                x: -4.57763671875e-05,
                y: 0.93664455413818359,
                z: -1.52587890625e-05
            },
            dimensions: {
                x: 0.15723302960395813,
                y: 0.50762706995010376,
                z: 0.90716040134429932
            },
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                },
                grabbableKey: {
                    grabbable: false
                }
            })
        });

    }

    function createFlashlight(position) {
        var modelURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flashlight/flashlight.fbx";

        var flashlight = Entities.addEntity({
            type: "Model",
            modelURL: modelURL,
            name: "flashlight",
            script: flashlightScriptURL,
            position: position,
            dimensions: {
                x: 0.08,
                y: 0.30,
                z: 0.08
            },
            dynamic: true,
            collisionSoundURL: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/flashlight/flashlight_drop.L.wav",
            gravity: {
                x: 0,
                y: -3.5,
                z: 0
            },
            restitution: 0,
            velocity: {
                x: 0,
                y: -0.01,
                z: 0
            },
            shapeType: 'box',
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                },
                grabbableKey: {
                    invertSolidWhileHeld: true
                },
                wearable: {
                    joints: {
                        RightHand: [{
                            x: 0.0717092975974083,
                            y: 0.1166968047618866,
                            z: 0.07085515558719635
                        }, {
                            x: -0.7195770740509033,
                            y: 0.175227552652359,
                            z: 0.5953742265701294,
                            w: 0.31150275468826294
                        }],
                        LeftHand: [{
                            x: 0.0806504637002945,
                            y: 0.09710478782653809,
                            z: 0.08610185235738754
                        }, {
                            x: 0.5630447864532471,
                            y: -0.2545935809612274,
                            z: 0.7855332493782043,
                            w: 0.033170729875564575
                        }]
                    }
                }
            })
        });


    }

    function createLights() {
        var modelURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/lights/lightswitch.fbx";

        var rotation = {
            w: 0.63280689716339111,
            x: 0.63280689716339111,
            y: -0.31551080942153931,
            z: 0.31548023223876953
        };
        var axis = {
            x: 0,
            y: 1,
            z: 0
        };
        var dQ = Quat.angleAxis(180, axis);
        rotation = Quat.multiply(rotation, dQ);

        var lightSwitchHall = Entities.addEntity({
            type: "Model",
            modelURL: modelURL,
            name: "Light Switch Hall",
            script: lightsScriptURL,
            position: {
                x: 543.27764892578125,
                y: 495.67999267578125,
                z: 511.00564575195312
            },
            rotation: rotation,
            dimensions: {
                x: 0.10546875,
                y: 0.032372996211051941,
                z: 0.16242524981498718
            },
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true,
                    on: true,
                    type: "Hall Light"
                },
                grabbableKey: {
                    wantsTrigger: true
                }
            })
        });

        var sconceLight1 = Entities.addEntity({
            type: "Light",
            position: {
                x: 543.75,
                y: 496.24,
                z: 511.13
            },
            name: "Sconce 1 Light",
            dimensions: {
                x: 2.545,
                y: 2.545,
                z: 2.545
            },
            intensity: 1.0,
            falloffRadius: 0.3,
            cutoff: 90,
            color: {
                red: 217,
                green: 146,
                blue: 24
            },
            isSpotlight: false,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true,
                    type: "Hall Light"
                }
            })
        });

        var sconceLight2 = Entities.addEntity({
            type: "Light",
            position: {
                x: 540.1,
                y: 496.24,
                z: 505.57
            },
            name: "Sconce 2 Light",
            dimensions: {
                x: 2.545,
                y: 2.545,
                z: 2.545
            },
            intensity: 1.0,
            falloffRadius: 0.3,
            cutoff: 90,
            color: {
                red: 217,
                green: 146,
                blue: 24
            },
            isSpotlight: false,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true,
                    type: "Hall Light"
                }
            })
        });

        rotation = {
            w: 0.20082402229309082,
            x: 0.20082402229309082,
            y: -0.67800414562225342,
            z: 0.67797362804412842
        };
        axis = {
            x: 0,
            y: 1,
            z: 0
        };
        dQ = Quat.angleAxis(180, axis);
        rotation = Quat.multiply(rotation, dQ);

        var lightSwitchGarage = Entities.addEntity({
            type: "Model",
            modelURL: modelURL,
            name: "Light Switch Garage",
            script: lightsScriptURL,
            position: {
                x: 545.62,
                y: 495.68,
                z: 500.21
            },
            rotation: rotation,
            dimensions: {
                x: 0.10546875,
                y: 0.032372996211051941,
                z: 0.16242524981498718
            },
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true,
                    on: true,
                    type: "Garage Light"
                },
                grabbableKey: {
                    wantsTrigger: true
                }
            })
        });



        var sconceLight3 = Entities.addEntity({
            type: "Light",
            position: {
                x: 545.49468994140625,
                y: 496.24026489257812,
                z: 500.63516235351562
            },

            name: "Sconce 3 Light",
            dimensions: {
                x: 2.545,
                y: 2.545,
                z: 2.545
            },
            intensity: 1.0,
            falloffRadius: 0.3,
            cutoff: 90,
            color: {
                red: 217,
                green: 146,
                blue: 24
            },
            isSpotlight: false,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true,
                    type: "Garage Light"
                }
            })
        });


        var sconceLight4 = Entities.addEntity({
            type: "Light",
            position: {
                x: 550.90399169921875,
                y: 496.24026489257812,
                z: 507.90237426757812
            },
            name: "Sconce 4 Light",
            dimensions: {
                x: 2.545,
                y: 2.545,
                z: 2.545
            },
            intensity: 1.0,
            falloffRadius: 0.3,
            cutoff: 90,
            color: {
                red: 217,
                green: 146,
                blue: 24
            },
            isSpotlight: false,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true,
                    type: "Garage Light"
                }
            })
        });

        var sconceLight5 = Entities.addEntity({
            type: "Light",
            position: {
                x: 548.407958984375,
                y: 496.24026489257812,
                z: 509.5504150390625
            },
            name: "Sconce 5 Light",
            dimensions: {
                x: 2.545,
                y: 2.545,
                z: 2.545
            },
            intensity: 1.0,
            falloffRadius: 0.3,
            cutoff: 90,
            color: {
                red: 217,
                green: 146,
                blue: 24
            },
            isSpotlight: false,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true,
                    type: "Garage Light"
                }
            })
        });

    }



    function createDice() {
        var diceProps = {
            type: "Model",
            modelURL: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/dice/goldDie.fbx",
            collisionSoundURL: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/dice/diceCollide.wav",
            name: "dice",
            position: {
                x: 541.61,
                y: 495.21,
                z: 508.52
            },
            dimensions: {
                x: 0.09,
                y: 0.09,
                z: 0.09
            },
            gravity: {
                x: 0,
                y: -3.5,
                z: 0
            },
            velocity: {
                x: 0,
                y: -0.01,
                z: 0
            },
            shapeType: "box",
            dynamic: true,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                },
                grabbableKey: {
                    invertSolidWhileHeld: true
                }

            })
        };
        var dice1 = Entities.addEntity(diceProps);

        diceProps.position = {
            x: 541.52,
            y: 495.21,
            z: 508.41
        };

        var dice2 = Entities.addEntity(diceProps);

    }

    function createGates() {
        var MODEL_URL = 'https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/gates/fence.fbx';
        var rotation = Quat.fromPitchYawRollDegrees(0, -16, 0);
        var gate = Entities.addEntity({
            name: 'Front Door Fence',
            type: 'Model',
            modelURL: MODEL_URL,
            shapeType: 'box',
            position: {
                x: 531.15,
                y: 495.11,
                z: 520.20
            },
            dimensions: {
                x: 1.42,
                y: 1.13,
                z: 0.2
            },
            rotation: rotation,
            dynamic: true,
            gravity: {
                x: 0,
                y: -100,
                z: 0
            },
            damping: 1,
            angularDamping: 10,
            mass: 10,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                },
                grabbableKey: {
                    grabbable: false
                }
            })
        });
    }

    function createPingPongBallGun() {
        var MODEL_URL = 'https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/ping_pong_gun/Pingpong-Gun-New.fbx';
        var COLLISION_HULL_URL = 'https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/ping_pong_gun/Pingpong-Gun-New.obj';
        var COLLISION_SOUND_URL = 'https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/ping_pong_gun/plastic_impact.L.wav';
        var position = {
            x: 548.6,
            y: 495.4,
            z: 503.39
        };

        var rotation = Quat.fromPitchYawRollDegrees(0, 0, 0);

        var pingPongGun = Entities.addEntity({
            type: "Model",
            modelURL: MODEL_URL,
            shapeType: 'compound',
            compoundShapeURL: COLLISION_HULL_URL,
            script: pingPongScriptURL,
            position: position,
            rotation: rotation,
            gravity: {
                x: 0,
                y: -9.8,
                z: 0
            },
            restitution: 0,
            dimensions: {
                x: 0.125,
                y: 0.3875,
                z: 0.9931
            },
            dynamic: true,
            collisionSoundURL: COLLISION_SOUND_URL,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                },
                grabbableKey: {
                    invertSolidWhileHeld: true
                },
                wearable: {
                    joints: {
                        RightHand: [{
                            x: 0.1177130937576294,
                            y: 0.12922893464565277,
                            z: 0.08307232707738876
                        }, {
                            x: 0.4934672713279724,
                            y: 0.3605862259864807,
                            z: 0.6394805908203125,
                            w: -0.4664038419723511
                        }],
                        LeftHand: [{
                            x: 0.09151676297187805,
                            y: 0.13639454543590546,
                            z: 0.09354984760284424
                        }, {
                            x: -0.19628101587295532,
                            y: 0.6418180465698242,
                            z: 0.2830369472503662,
                            w: 0.6851521730422974
                        }]
                    }
                }
            })
        });
    }

    function createWand(position) {
        var WAND_MODEL = 'https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/bubblewand/wand.fbx';
        var WAND_COLLISION_SHAPE = 'https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/bubblewand/wand_collision_hull.obj';

        var wand = Entities.addEntity({
            name: 'Bubble Wand',
            type: "Model",
            modelURL: WAND_MODEL,
            shapeType: 'compound',
            position: position,
            gravity: {
                x: 0,
                y: -9.8,
                z: 0,
            },
            dimensions: {
                x: 0.05,
                y: 0.25,
                z: 0.05
            },
            //must be enabled to be grabbable in the physics engine
            dynamic: true,
            compoundShapeURL: WAND_COLLISION_SHAPE,
            script: wandScriptURL,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                },
                grabbableKey: {
                    invertSolidWhileHeld: true
                },
                "wearable": {
                    "joints": {
                        "RightHand": [{
                            "x": 0.11421211808919907,
                            "y": 0.06508062779903412,
                            "z": 0.06317152827978134
                        }, {
                            "x": -0.7886992692947388,
                            "y": -0.6108893156051636,
                            "z": -0.05003821849822998,
                            "w": 0.047579944133758545
                        }],
                        "LeftHand": [{
                            "x": 0.03530977666378021,
                            "y": 0.11278322339057922,
                            "z": 0.049768272787332535
                        }, {
                            "x": -0.050609711557626724,
                            "y": -0.11595471203327179,
                            "z": 0.3554558753967285,
                            "w": 0.9260908961296082
                        }]
                    }
                }
            })
        });

    }

    function createBasketBall(position) {

        var modelURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/basketball/basketball2.fbx";

        var entity = Entities.addEntity({
            type: "Model",
            modelURL: modelURL,
            position: position,
            dynamic: true,
            shapeType: "sphere",
            name: "basketball",
            dimensions: {
                x: 0.25,
                y: 0.26,
                z: 0.25
            },
            gravity: {
                x: 0,
                y: -7,
                z: 0
            },
            restitution: 10,
            damping: 0.01,
            velocity: {
                x: 0,
                y: -0.01,
                z: 0
            },
            collisionSoundURL: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/basketball/basketball.wav",
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                },
                grabbableKey: {
                    invertSolidWhileHeld: true
                }
            })
        });

    }

    function createDoll(position) {
        var modelURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/doll/bboy2.fbx";

        var naturalDimensions = {
            x: 1.63,
            y: 1.67,
            z: 0.26
        };
        var desiredDimensions = Vec3.multiply(naturalDimensions, 0.15);
        var entity = Entities.addEntity({
            type: "Model",
            name: "doll",
            modelURL: modelURL,
            script: dollScriptURL,
            position: position,
            shapeType: 'box',
            dimensions: desiredDimensions,
            gravity: {
                x: 0,
                y: -5,
                z: 0
            },
            velocity: {
                x: 0,
                y: -0.1,
                z: 0
            },
            dynamic: true,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                },
                grabbableKey: {
                    invertSolidWhileHeld: true
                }
            })
        });

    }

    function createSprayCan(position) {


        var modelURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/spray_paint/paintcan.fbx";

        var entity = Entities.addEntity({
            type: "Model",
            name: "spraycan",
            script: sprayPaintScriptURL,
            modelURL: modelURL,
            position: position,
            dimensions: {
                x: 0.07,
                y: 0.17,
                z: 0.07
            },
            dynamic: true,
            collisionSoundURL: "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/spray_paint/SpryPntCnDrp1.L.wav",
            shapeType: 'box',
            gravity: {
                x: 0,
                y: -3.0,
                z: 0
            },
            restitution: 0,
            velocity: {
                x: 0,
                y: -1,
                z: 0
            },
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                },
                grabbableKey: {
                    invertSolidWhileHeld: true
                }
            })
        });

    }

    function createPottedPlant(position) {
        var modelURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/potted_plant/potted_plant.fbx";

        var entity = Entities.addEntity({
            type: "Model",
            name: "Potted Plant",
            modelURL: modelURL,
            position: position,
            dimensions: {
                x: 1.10,
                y: 2.18,
                z: 1.07
            },
            dynamic: true,
            shapeType: 'box',
            gravity: {
                x: 0,
                y: -9.8,
                z: 0
            },
            velocity: {
                x: 0,
                y: 0,
                z: 0
            },
            damping: 0.4,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                },
                grabbableKey: {
                    grabbable: false
                }
            })
        });
    }


    function createCombinedArmChair(position) {
        var modelURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/armchair/combined_chair.fbx";
        var RED_ARM_CHAIR_COLLISION_HULL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/armchair/red_arm_chair_collision_hull.obj";

        var rotation = Quat.fromPitchYawRollDegrees(0, -143, 0);

        var entity = Entities.addEntity({
            type: "Model",
            name: "Red Arm Chair",
            modelURL: modelURL,
            shapeType: 'compound',
            compoundShapeURL: RED_ARM_CHAIR_COLLISION_HULL,
            position: position,
            rotation: rotation,
            dimensions: {
                x: 1.26,
                y: 1.56,
                z: 1.35
            },
            dynamic: true,
            gravity: {
                x: 0,
                y: -0.8,
                z: 0
            },
            velocity: {
                x: 0,
                y: 0,
                z: 0
            },
            damping: 0.2,
            userData: JSON.stringify({
                resetMe: {
                    resetMe: true
                },
                grabbableKey: {
                    grabbable: false
                }
            })
        });

    }

    function createBlocks(position) {
        var baseURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/planky/";
        var collisionSoundURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/planky/ToyWoodBlock.L.wav";
        var NUM_BLOCKS_PER_COLOR = 4;
        var i, j;

        var blockTypes = [{
            url: "planky_blue.fbx",
            dimensions: {
                x: 0.05,
                y: 0.05,
                z: 0.25
            }
        }, {
            url: "planky_green.fbx",
            dimensions: {
                x: 0.1,
                y: 0.1,
                z: 0.25
            }
        }, {
            url: "planky_natural.fbx",
            dimensions: {
                x: 0.05,
                y: 0.05,
                z: 0.05
            }
        }, {
            url: "planky_yellow.fbx",
            dimensions: {
                x: 0.03,
                y: 0.05,
                z: 0.25
            }
        }, {
            url: "planky_red.fbx",
            dimensions: {
                x: 0.1,
                y: 0.05,
                z: 0.25
            }
        }];

        var modelURL, entity;
        for (i = 0; i < blockTypes.length; i++) {
            for (j = 0; j < NUM_BLOCKS_PER_COLOR; j++) {
                modelURL = baseURL + blockTypes[i].url;
                entity = Entities.addEntity({
                    type: "Model",
                    modelURL: modelURL,
                    position: Vec3.sum(position, {
                        x: j / 10,
                        y: i / 10,
                        z: 0
                    }),
                    shapeType: 'box',
                    name: "block",
                    dimensions: blockTypes[i].dimensions,
                    dynamic: true,
                    collisionSoundURL: collisionSoundURL,
                    gravity: {
                        x: 0,
                        y: -2.5,
                        z: 0
                    },
                    velocity: {
                        x: 0,
                        y: -0.01,
                        z: 0
                    },
                    userData: JSON.stringify({
                        resetMe: {
                            resetMe: true
                        },
                        grabbableKey: {
                            invertSolidWhileHeld: true
                        }
                    })
                });

            }
        }
    }

    function cleanup() {
        deleteAllToys();
    }

    if (shouldDeleteOnEndScript) {

        Script.scriptEnding.connect(cleanup);
    }
};