//
//  bow.js
//
//  This script attaches to a bow that you can pick up with a hand controller.  Use your other hand and press the trigger to grab the line, and release the trigger to fire.
//
//  Created by James B. Pollack @imgntn on 10/19/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// TODO: 
// make it so you can shoot without blocking your view with your hand
// notching system
// make arrows more visible
// make arrow rotate toward ground as it flies
// different model?  compound bow will look better in the HMD, and be easier to aim. http://www.turbosquid.com/3d-models/3d-model-bow-arrow/773106
// add noise when you release arrow -> add the sound to the arrow and keep it with position so you hear it whizz by 
// add noise when you draw string
// re-enable arrows sticking when they hit
// prepare for haptics 

(function() {

    var ZERO_VEC = {
        x: 0,
        y: 0,
        z: 0
    };

    var LINE_ENTITY_DIMENSIONS = {
        x: 1000,
        y: 1000,
        z: 1000
    };


    var ARROW_MODEL_URL = "https://hifi-public.s3.amazonaws.com/models/bow/arrow_good.fbx";
    var ARROW_COLLISION_HULL_URL = "https://hifi-public.s3.amazonaws.com/models/bow/arrow_good_collision_hull.obj";
    var ARROW_SCRIPT_URL = Script.resolvePath('arrow.js');
    var ARROW_OFFSET = 0.32;
    var ARROW_FORCE = 1.25;
    var ARROW_DIMENSIONS = {
        x: 0.02,
        y: 0.02,
        z: 0.64
    };

    var ARROW_GRAVITY = {
        x: 0,
        y: -9.8,
        z: 0
    };

    var ARROW_TIP_OFFSET = 0.32;

    var TOP_NOTCH_OFFSET = 0.5;
    var BOTTOM_NOTCH_OFFSET = 0.5;

    var LINE_DIMENSIONS = {
        x: 5,
        y: 5,
        z: 5
    };

    var DRAW_STRING_THRESHOLD = 0.80;

    var TARGET_LINE_LENGTH = 1;

    var LEFT_TIP = 1;
    var RIGHT_TIP = 3;

    var NOTCH_DETECTOR_OFFSET = {
        x: 0,
        y: 0,
        z: 0
    };

    var NOTCH_DETECTOR_DIMENSIONS = {
        x: 0.25,
        y: 0.25,
        z: 0.25
    };

    var NOTCH_DETECTOR_DISTANCE = 0.1;

    var RELOAD_DETECTOR_OFFSET = 0.3;

    var RELOAD_DETECTOR_DIMENSIONS = {
        x: 0.5,
        y: 0.5,
        z: 0.5
    };

    var RELOAD_DETECTOR_DISTANCE = 0.1;

    var _this;

    function Bow() {
        _this = this;
        return;
    }

    // create bow
    // on pickup, wait for other hand to start trigger pull
    // then create strings that start at top and bottom of bow
    // extend them to the other hand position
    // on trigger release, 
    // create arrow 
    // shoot arrow with velocity relative to distance between hand position and bow
    // delete lines

    // with notching system (for reloading):
    // pick up bow
    // move other hand near reload detector
    // move arrow near notch detector
    // pull back on the trigger to draw the string
    // release to fire

    Bow.prototype = {
        useNotching: true,
        isGrabbed: false,
        stringDrawn: false,
        hasArrow: false,
        arrowTipPosition: null,
        hasArrowLoaded: false,
        reloadDetector: null,
        hasArrowNotched: false,
        notchDetector: null,
        arrow: null,
        arrowIsBurning: false,
        fire: null,
        stringData: {
            currentColor: {
                red: 0,
                green: 255,
                blue: 0
            }
        },

        preload: function(entityID) {
            this.entityID = entityID;
        },

        setLeftHand: function() {
            if (this.isGrabbed === true) {
                return false;
            }
            this.hand = 'left';
        },

        setRightHand: function() {
            if (this.isGrabbed === true) {
                return false;
            }
            this.hand = 'right';
        },

        startNearGrab: function() {
            if (this.isGrabbed === true) {
                return false;
            }
            this.isGrabbed = true;
            this.initialHand = this.hand;
            this.createReloadDetector();

            Entities.editEntity(this.entityID, {
                userData: JSON.stringify({
                    grabbableKey: {
                        turnOffOtherHand: true,
                        turnOffOppositeBeam: true,
                        invertSolidWhileHeld: true
                    }
                })
            });
        },

        continueNearGrab: function() {
            this.bowProperties = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);
            if (this.notchDetector === null) {
                this.createNotchDetector();
            }

            this.updateNotchDetectorPosition();
            this.updateReloadDetectorPosition();

            this.checkStringHand();

            if (this.useNotching === true) {
         this.checkArrowHand();

                if (this.hasArrowLoaded === false) {
                    this.updateArrowPositionPreNotch();

                }

                if (this.hasArrowLoaded === true) {
                    this.updateNotchDetectorPosition();
                }

       
                if (this.hasArrowNotched === true) {
                    //only test for strings now that an arrow is notched
                  //  this.checkStringHand();
                    //should probably draw a string straight across the bow until its notched
                }
            }

        },

        releaseGrab: function() {
            if (this.isGrabbed === true && this.hand === this.initialHand) {
                this.isGrabbed = false;
                this.stringDrawn = false;
                this.deleteStrings();
                this.hasArrow = false;
                Entities.deleteEntity(this.arrow);
                Entities.editEntity(this.entityID, {
                    userData: JSON.stringify({
                        grabbableKey: {
                            turnOffOtherHand: false,
                            turnOffOppositeBeam: true,
                            invertSolidWhileHeld: true
                        }
                    })
                });
                Entities.deleteEntity(this.reloadDetector);
                Entities.deleteEntity(this.notchDetector);
                this.notchDetector = null;
                this.reloadDetector = null;
                // if(this.useNotching===true){

                // }
            }
        },

        createStrings: function() {
            this.createTopString();
            this.createBottomString();
        },

        createTopString: function() {
            var stringProperties = {
                type: 'Line',
                position: Vec3.sum(this.bowProperties.position, TOP_NOTCH_OFFSET),
                dimensions: LINE_DIMENSIONS
            };

            this.topString = Entities.addEntity(stringProperties);
        },

        createBottomString: function() {
            var stringProperties = {
                type: 'Line',
                position: Vec3.sum(this.bowProperties.position, BOTTOM_NOTCH_OFFSET),
                dimensions: LINE_DIMENSIONS
            };

            this.bottomString = Entities.addEntity(stringProperties);
        },

        deleteStrings: function() {
            Entities.deleteEntity(this.topString);
            Entities.deleteEntity(this.bottomString);
        },

        updateStringPositions: function() {

            var upVector = Quat.getUp(this.bowProperties.rotation);
            var upOffset = Vec3.multiply(upVector, TOP_NOTCH_OFFSET);
            var downVector = Vec3.multiply(-1, Quat.getUp(this.bowProperties.rotation));
            var downOffset = Vec3.multiply(downVector, BOTTOM_NOTCH_OFFSET);

            this.topStringPosition = Vec3.sum(this.bowProperties.position, upOffset);
            this.bottomStringPosition = Vec3.sum(this.bowProperties.position, downOffset);

            Entities.editEntity(this.topString, {
                position: this.topStringPosition
            });

            Entities.editEntity(this.bottomString, {
                position: this.bottomStringPosition
            });

            Entities.editEntity(this.preNotchString, {
                position: this.topStringPosition
            });


        },

        drawStrings: function() {

            this.updateStringPositions();
            var lineVectors = this.getLocalLineVectors();

            Entities.editEntity(this.topString, {
                linePoints: [{
                    x: 0,
                    y: 0,
                    z: 0
                }, lineVectors[0]],
                lineWidth: 5,
                color: this.stringData.currentColor
            });

            Entities.editEntity(this.bottomString, {
                linePoints: [{
                    x: 0,
                    y: 0,
                    z: 0
                }, lineVectors[1]],
                lineWidth: 5,
                color: this.stringData.currentColor
            });

        },


        createPreNotchString: function() {
            var stringProperties = {
                type: 'Line',
                position: Vec3.sum(this.bowProperties.position, TOP_NOTCH_OFFSET),
                dimensions: LINE_DIMENSIONS
            };

            this.preNotchString = Entities.addEntity(stringProperties);
        },

        drawPreNotchStrings: function() {

            this.updateStringPositions();

            Entities.editEntity(this.preNotchString, {
                linePoints: [{
                    x: 0,
                    y: 0,
                    z: 0
                }, Vec3.sum({
                    x: 0,
                    y: 0,
                    z: 0
                }, BOTTOM_NOTCH_OFFSET * 2)],
                lineWidth: 5,
                color: this.stringData.currentColor
            });
        },

        deletePreNotchString: function() {
            Entities.deleteEntity(this.preNotchString);
        },

        checkArrowHand: function() {
            if (this.initialHand === 'left') {
                this.getArrowHandPosition = MyAvatar.getRightPalmPosition;
                this.getArrowHandRotation = MyAvatar.getRightPalmRotation;
            } else if (this.initialHand === 'right') {
                this.getArrowHandPosition = MyAvatar.getLeftPalmPosition;
                this.getArrowHandRotation = MyAvatar.getLeftPalmRotation;
            }
            if (this.hasArrowLoaded === false) {
                this.testForHandInReloadDetector();
            }

            if (this.hasArrowLoaded === true) {
                this.testForHandInNotchDetector();
            }

        },

        testForHandInReloadDetector: function() {
            var arrowHandPosition = this.getArrowHandPosition();
            var reloadDetectorPosition = Entities.getEntityProperties(this.reloadDetector, "position");
            var fromArrowHandToReloadDetector = Vec3.subtract(arrowHandPosition, reloadDetectorPosition);
            var distance = Vec3.length(fromArrowHandToReloadDetector);
            print('fromArrowHandToReloadDetector distance :: '+distance);
            if (fromArrowHandToReloadDetector < RELOAD_DETECTOR_DISTANCE) {
                print('ARROW LOADED');
                this.hasArrowLoaded = true;
            }
        },

        testForHandInNotchDetector: function() {
            var arrowHandPosition = this.getArrowHandPosition();
            var notchDetectorPosition = Entities.getEntityProperties(this.notchDetector, "position");
            var fromArrowHandToNotchDetector = Vec3.subtract(arrowHandPosition, notchDetectorPosition);
            var distance = Vec3.length(fromArrowHandToNotchDetector);
            if (distance < NOTCH_DETECTOR_DISTANCE) {
                print('ARROW NOTCHED');
                this.hasArrowNotched = true;
                this.notchArrow();
            }
        },

        checkStringHand: function() {
            //invert the hands because our string will be held with the opposite hand of the first one we pick up the bow with
            if (this.initialHand === 'left') {
                this.getStringHandPosition = MyAvatar.getRightPalmPosition;
                this.getStringHandRotation = MyAvatar.getRightPalmRotation;
                this.getGrabHandPosition = MyAvatar.getLeftPalmPosition;
                this.getGrabHandRotation = MyAvatar.getLeftPalmRotation;
                this.stringTriggerAction = Controller.findAction("RIGHT_HAND_CLICK");
            } else if (this.initialHand === 'right') {
                this.getStringHandPosition = MyAvatar.getLeftPalmPosition;
                this.getStringHandRotation = MyAvatar.getLeftPalmRotation;
                this.getGrabHandPosition = MyAvatar.getRightPalmPosition;
                this.getGrabHandRotation = MyAvatar.getRightPalmRotation;
                this.stringTriggerAction = Controller.findAction("LEFT_HAND_CLICK");
            }

            this.triggerValue = Controller.getActionValue(this.stringTriggerAction);

            if (this.triggerValue < DRAW_STRING_THRESHOLD && this.stringDrawn === true) {
                this.stringDrawn = false;
                this.deleteStrings();
                this.hasArrow = false;
                this.releaseArrow();

            } else if (this.triggerValue >= DRAW_STRING_THRESHOLD && this.stringDrawn === true) {
                this.stringData.handPosition = this.getStringHandPosition();
                this.stringData.handRotation = this.getStringHandRotation();
                this.initialHand === 'right' ? this.stringData.grabHandPosition = Controller.getSpatialControlPosition(RIGHT_TIP) : this.stringData.grabHandPosition = Controller.getSpatialControlPosition(LEFT_TIP);
                //   this.stringData.grabHandPosition = this.getGrabHandPosition();
                this.stringData.grabHandRotation = this.getGrabHandRotation();


                this.drawStrings();
                this.updateArrowPosition();

            } else if (this.triggerValue >= DRAW_STRING_THRESHOLD && this.stringDrawn === false) {
                this.stringDrawn = true;
                this.createStrings();
                this.stringData.handPosition = this.getStringHandPosition();
                this.stringData.handRotation = this.getStringHandRotation();
                // this.stringData.grabHandPosition = this.getGrabHandPosition();
                this.initialHand === 'right' ? this.stringData.grabHandPosition = Controller.getSpatialControlPosition(RIGHT_TIP) : this.stringData.grabHandPosition = Controller.getSpatialControlPosition(LEFT_TIP);
                this.stringData.grabHandRotation = this.getGrabHandRotation();
                if (this.hasArrow === false) {
                    this.createArrow();
                    this.hasArrow = true;
                }

                this.drawStrings();
                this.updateArrowPosition();

            }
        },

        getArrowPosition: function() {
            var arrowVector = Vec3.subtract(this.stringData.handPosition, this.stringData.grabHandPosition);
            arrowVector = Vec3.normalize(arrowVector);
            arrowVector = Vec3.multiply(arrowVector, ARROW_OFFSET);
            var arrowPosition = Vec3.sum(this.stringData.grabHandPosition, arrowVector);
            return arrowPosition;
        },

        orientationOf: function(vector) {
            var Y_AXIS = {
                x: 0,
                y: 1,
                z: 0
            };
            var X_AXIS = {
                x: 1,
                y: 0,
                z: 0
            };

            var theta = 0.0;

            var RAD_TO_DEG = 180.0 / Math.PI;
            var direction, yaw, pitch;
            direction = Vec3.normalize(vector);
            yaw = Quat.angleAxis(Math.atan2(direction.x, direction.z) * RAD_TO_DEG, Y_AXIS);
            pitch = Quat.angleAxis(Math.asin(-direction.y) * RAD_TO_DEG, X_AXIS);
            return Quat.multiply(yaw, pitch);
        },

        updateArrowPosition: function() {
            var arrowPosition = this.getArrowPosition();

            var handToHand = Vec3.subtract(this.stringData.handPosition, this.stringData.grabHandPosition);

            var arrowRotation = this.orientationOf(handToHand);

            Entities.editEntity(this.arrow, {
                position: arrowPosition,
                rotation: arrowRotation
            });

            if (this.arrowIsBurning === true) {
                Entities.editEntity(this.fire, {
                    position: this.arrow.position
                });
            }
        },

        updateArrowPositionPreNotch: function() {
            Entities.editEntity(this.arrow, {
                position: this.getArrowHandPosition(),
                rotatin: this.getArrowHandRotation()
            });

            if (this.arrowIsBurning === true) {
                Entities.editEntity(this.fire, {
                    position: arrowHandPosition
                });
            }
        },

        updateArrowPositionInNotch: function() {
            var notchPosition = this.notchDetectorPosition;

            Entities.editEntityProperties(this.arrow, {
                position: notchPosition
            })

            if (this.arrowIsBurning === true) {
                Entities.editEntity(this.fire, {
                    position: arrowTipPosition
                });
            }

        },

        createArrow: function() {
            //  print('CREATING ARROW');
            var arrowProperties = {
                name: 'Hifi-Arrow',
                type: 'Model',
                shapeType: 'box',
                modelURL: ARROW_MODEL_URL,
                dimensions: ARROW_DIMENSIONS,
                position: this.getArrowPosition(),
                rotation: this.bowProperties.rotation,
                collisionsWillMove: false,
                ignoreForCollisions: true,
                gravity: ARROW_GRAVITY,
                // script: ARROW_SCRIPT_URL,
                lifetime: 40,
                userData: JSON.stringify({
                    grabbableKey: {
                        grabbable: false,

                    }
                })
            };

            this.arrow = Entities.addEntity(arrowProperties);

        },

        getArrowTipPosition: function() {
            var arrowPosition = this.getArrowPosition();
            var arrowTipPosition = Vec3.sum(arrowPosition, ARROW_TIP_OFFSET);
            return arrowTipPosition
        },

        releaseArrow: function() {
            var handToHand = Vec3.subtract(this.stringData.grabHandPosition, this.stringData.handPosition);

            var arrowRotation = this.orientationOf(handToHand);

            var handDistanceAtRelease = Vec3.distance(this.stringData.grabHandPosition, this.stringData.handPosition);
            print('HAND DISTANCE:: ' + handDistanceAtRelease);
            var arrowForce = this.scaleArrowShotStrength(handDistanceAtRelease, 0, 2, 20, 50);
            print('ARROW FORCE::' + arrowForce);
            var forwardVec = Vec3.multiply(handToHand, arrowForce);

            var arrowProperties = {
                // rotation:handToHand,
                ignoreForCollisions: false,
                collisionsWillMove: true,
                gravity: ARROW_GRAVITY,
                velocity: forwardVec,
                lifetime: 20
            };

            Entities.editEntity(this.arrow, arrowProperties);
            Script.setTimeout(function() {
                Entities.editEntity(this.arrow, {
                    ignoreForCollisions: false,
                    collisionsWillMove: true,
                    gravity: ARROW_GRAVITY,
                });
            }, 100)

            this.arrow = null;
            this.fire = null;
            this.arrowIsBurnning = false;

        },

        getLocalLineVectors: function() {
            var topVector = Vec3.subtract(this.stringData.handPosition, this.topStringPosition);
            var bottomVector = Vec3.subtract(this.stringData.handPosition, this.bottomStringPosition);
            return [topVector, bottomVector];
        },

        scaleArrowShotStrength: function(value, min1, max1, min2, max2) {
            return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
        },

        getReloadDetectorPosition: function() {
            var avatarHeadPosition = MyAvatar.getHeadPosition();
            var offsetDirection;
            this.initialHand === 'left' ? offsetDirection = Quat.getRight(MyAvatar.orientation) : offsetDirection = Vec3.multiply(-1, Quat.getRight(MyAvatar.orientation));
            var offset = Vec3.multiply(offsetDirection, RELOAD_DETECTOR_OFFSET);
            var detectorPosition = Vec3.sum(avatarHeadPosition, offset);

            return detectorPosition
        },

        createReloadDetector: function() {

            var detectorPosition = this.getReloadDetectorPosition();

            var detectorProperties = {
                type: 'Box',
                visible: true,
                ignoreForCollisions: true,
                dimensions: RELOAD_DETECTOR_DIMENSIONS,
                position: detectorPosition,
                color: {
                    red: 255,
                    green: 0,
                    blue: 0
                }
            };

            this.reloadDetector = Entities.addEntity(detectorProperties);
        },

        createNotchDetector: function() {
            var detectorPosition = Vec3.sum(this.bowProperties.position, NOTCH_DETECTOR_OFFSET);

            var detectorProperties = {
                type: 'Box',
                visible: true,
                ignoreForCollisions: true,
                dimensions: NOTCH_DETECTOR_DIMENSIONS,
                position: detectorPosition,
                color: {
                    red: 0,
                    green: 255,
                    blue: 0
                }
            };

            this.notchDetector = Entities.addEntity(detectorProperties);
        },

        updateReloadDetectorPosition: function() {
            this.reloadDetectorPosition = this.getReloadDetectorPosition();
            Entities.editEntity(this.reloadDetector, {
                position: this.reloadDetectorPosition
            });
        },

        updateNotchDetectorPosition: function() {
            this.notchDetectorPosition = Vec3.sum(this.bowProperties.position, NOTCH_DETECTOR_OFFSET);
            Entities.editEntity(this.notchDetector, {
                position: this.notchDetectorPosition
            });
        },

        setArrowOnFire: function() {

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
                name: "Hifi-Arrow-Fire",
                animationSettings: animationSettings,
                textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
                position: this.arrowTipPosition,
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
            });

            Entites.editEntityProperties(this.arrow, {
                userData: JSON.stringify({
                    hifiFireArrowKey: {
                        fire: this.fire
                    }
                })
            })

            this.arrowIsBurning = true;
        }

    };

    return new Bow();
});