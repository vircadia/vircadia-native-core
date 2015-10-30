//
//  bow.js
//
//  This script attaches to a bow that you can pick up with a hand controller. Load an arrow and then shoot it.
//  Created by James B. Pollack @imgntn on 10/19/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// to do: 
// make arrows more visible
// make arrow rotate toward ground as it flies
// add noise when you release arrow -> add the sound to the arrow and keep it with position so you hear it whizz by 
// add noise when you draw string
// re-enable arrows sticking when they hit
// prepare for haptics 

// from chat w/ ryan
// 5 arrows on table
// pick up arrow entity
// notch it
(function() {

    Script.include("../../libraries/utils.js");

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

    var ARROW_OFFSET = -0.36;

    var ARROW_GRAVITY = {
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

    var DRAW_STRING_THRESHOLD = 0.80;

    var TARGET_LINE_LENGTH = 1;

    var LEFT_TIP = 1;
    var RIGHT_TIP = 3;

    var NOTCH_DETECTOR_OFFSET_FORWARD = 0.08;
    var NOTCH_DETECTOR_OFFSET_UP = 0.035;

    var NOTCH_DETECTOR_DIMENSIONS = {
        x: 0.05,
        y: 0.05,
        z: 0.05
    };

    var NOTCH_DETECTOR_DISTANCE = 0.1;

    var _this;

    function Bow() {
        _this = this;
        return;
    }

    // pick up bow
    // pick up arrow
    // move arrow near notch detector
    // pull back on the trigger to draw the string
    // release to fire

    Bow.prototype = {
        isGrabbed: false,
        stringDrawn: false,
        hasArrow: false,
        arrowTipPosition: null,
        preNotchString: null,
        hasArrowNotched: false,
        notchDetector: null,
        arrow: null,
        arrowIsBurning: false,
        fire: null,
        stringData: {
            currentColor: {
                red: 255,
                green: 255,
                blue: 255
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

            setEntityCustomData('grabbableKey', this.entityID, {
                turnOffOtherHand: true,
                invertSolidWhileHeld: true
            });

        },

        continueNearGrab: function() {
            this.bowProperties = Entities.getEntityProperties(this.entityID, ["position", "rotation", "userData"]);
            this.updateNotchDetectorPosition();

            //check to see if an arrow has notched itself in our notch detector
            var userData = JSON.parse(this.bowProperties.userData);
            if (userData.hasOwnProperty('hifiBowKey')) {
                if (this.hasArrowNotched === false) {
                    //notch the arrow
                    this.hasArrowNotched = userData.hifiBowKey.hasArrowNotched;

                    this.arrow = userData.hifiBowKey.arrowID;

                    setEntityCustomData('grabbableKey', this.entityID, {
                        turnOffOtherHand: true,
                        invertSolidWhileHeld: true
                    });
                }

            }


            //create a string across the bow when we pick it up
            if (this.preNotchString === null) {
                this.createPreNotchString();
            }

            if (this.preNotchString !== null && this.hasArrowNotched === false) {
                this.drawPreNotchStrings();
            }

            // create the notch detector that arrows will look for
            if (this.notchDetector === null) {
                this.createNotchDetector();
            }

            //if we have an arrow notched, then draw some new strings
            if (this.hasArrowNotched === true) {
                if (this.preNotchString !== null) {
                    Entities.deleteEntity(this.preNotchString);
                }

                //only test for strings now that an arrow is notched
                this.checkStringHand();

            } else {
                //otherwise, don't do much of anything.

            }
        },

        releaseGrab: function() {
            if (this.isGrabbed === true && this.hand === this.initialHand) {
                this.isGrabbed = false;
                this.stringDrawn = false;
                this.deleteStrings();
                this.hasArrow = false;
                Entities.deleteEntity(this.arrow);
                setEntityCustomData('grabbableKey', this.entityID, {
                    turnOffOtherHand: false,
                    invertSolidWhileHeld: true
                });
                Entities.deleteEntity(this.notchDetector);
                this.notchDetector = null;
                Entities.deleteEntity(this.preNotchString);
                this.preNotchString = null;

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
                dimensions: LINE_DIMENSIONS,
                collisionsWillMove: false,
                ignoreForCollisions: true,
                userData: JSON.stringify({
                    grabbableKey: {
                        grabbable: false
                    }
                })
            };

            this.topString = Entities.addEntity(stringProperties);
        },

        createBottomString: function() {
            var stringProperties = {
                type: 'Line',
                position: Vec3.sum(this.bowProperties.position, BOTTOM_NOTCH_OFFSET),
                dimensions: LINE_DIMENSIONS,
                collisionsWillMove: false,
                ignoreForCollisions: true,
                userData: JSON.stringify({
                    grabbableKey: {
                        grabbable: false
                    }
                })
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
            var backOffset = Vec3.multiply(-0.1, Quat.getFront(this.bowProperties.rotation));

            var topStringPosition = Vec3.sum(this.bowProperties.position, upOffset);
            this.topStringPosition = Vec3.sum(topStringPosition, backOffset);
            var bottomStringPosition = Vec3.sum(this.bowProperties.position, downOffset);
            this.bottomStringPosition = Vec3.sum(bottomStringPosition, backOffset);

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


        getLocalLineVectors: function() {
            var topVector = Vec3.subtract(this.stringData.handPosition, this.topStringPosition);
            var bottomVector = Vec3.subtract(this.stringData.handPosition, this.bottomStringPosition);
            return [topVector, bottomVector];
        },



        createPreNotchString: function() {
            var stringProperties = {
                type: 'Line',
                position: Vec3.sum(this.bowProperties.position, TOP_NOTCH_OFFSET),
                dimensions: LINE_DIMENSIONS,
                userData: JSON.stringify({
                    grabbableKey: {
                        grabbable: false
                    }
                })
            };

            this.preNotchString = Entities.addEntity(stringProperties);
        },

        drawPreNotchStrings: function() {

            this.updateStringPositions();

            var downVector = Vec3.multiply(-1, Quat.getUp(this.bowProperties.rotation));
            var downOffset = Vec3.multiply(downVector, BOTTOM_NOTCH_OFFSET * 2);


            Entities.editEntity(this.preNotchString, {
                linePoints: [{
                    x: 0,
                    y: 0,
                    z: 0
                }, Vec3.sum({
                    x: 0,
                    y: 0,
                    z: 0
                }, downOffset)],
                lineWidth: 5,
                color: this.stringData.currentColor
            });
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

                // firing the arrow
                print('HIT RELEASE LOOP IN CHECK')
                    // this.stringDrawn = false;
                    // this.deleteStrings();
                    // this.hasArrow = false;
                    // this.releaseArrow();

            } else if (this.triggerValue >= DRAW_STRING_THRESHOLD && this.stringDrawn === true) {
                print('HIT CONTINUE LOOP IN CHECK')
                    //continuing to aim the arrow
                this.stringData.handPosition = this.getStringHandPosition();
                this.stringData.handRotation = this.getStringHandRotation();
                this.initialHand === 'right' ? this.stringData.grabHandPosition = Controller.getSpatialControlPosition(RIGHT_TIP) : this.stringData.grabHandPosition = Controller.getSpatialControlPosition(LEFT_TIP);
                this.stringData.grabHandRotation = this.getGrabHandRotation();
                this.drawStrings();
                this.updateArrowPositionInNotch();

            } else if (this.triggerValue >= DRAW_STRING_THRESHOLD && this.stringDrawn === false) {
                print('HIT START LOOP IN CHECK')
                    //the first time aiming the arrow
                this.stringDrawn = true;
                this.createStrings();
                this.stringData.handPosition = this.getStringHandPosition();
                this.stringData.handRotation = this.getStringHandRotation();
                this.initialHand === 'right' ? this.stringData.grabHandPosition = Controller.getSpatialControlPosition(RIGHT_TIP) : this.stringData.grabHandPosition = Controller.getSpatialControlPosition(LEFT_TIP);
                this.stringData.grabHandRotation = this.getGrabHandRotation();

                this.drawStrings();
                this.updateArrowPositionInNotch();

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


        updateArrowPositionInNotch: function() {

            //move it backwards
            var handToNotch = Vec3.subtract(this.stringData.handPosition, this.notchDetectorPosition);
            var pullBackDistance = Vec3.length(handToNotch);

            if (pullBackDistance >= 0.6) {
                pullBackDistance = 0.6;
            }

            var pullBackOffset = Vec3.multiply(handToNotch, pullBackDistance);
            var arrowPosition = Vec3.sum(this.notchDetectorPosition, pullBackOffset);

            var pushForwardOffset = Vec3.multiply(handToNotch, ARROW_OFFSET);
            var finalArrowPosition = Vec3.sum(arrowPosition, pushForwardOffset);

            var arrowRotation = this.orientationOf(handToNotch);

            Entities.editEntity(this.arrow, {
                position: finalArrowPosition,
                rotation: arrowRotation
            })

            if (this.arrowIsBurning === true) {
                Entities.editEntity(this.fire, {
                    position: arrowTipPosition
                });
            }

        },

        releaseArrow: function() {

            print('RELEASE ARROW!!!')

            var handToNotch = Vec3.subtract(this.notchDetectorPosition, this.stringData.handPosition);
            var pullBackDistance = Vec3.length(handToNotch);

            var arrowRotation = this.orientationOf(handToNotch);

            print('HAND DISTANCE:: ' + pullBackDistance);
            var arrowForce = this.scaleArrowShotStrength(pullBackDistance, 0, 2, 20, 50);
            print('ARROW FORCE::' + arrowForce);
            var forwardVec = Vec3.multiply(handToNotch, arrowForce);

            print('FWD VEC:::' + JSON.stringify(forwardVec));
            var arrowProperties = {
                // rotation:handToHand,

                velocity: forwardVec
            };

            Entities.editEntity(this.arrow, arrowProperties);
            Script.setTimeout(function() {

                Entities.editEntity(this.arrow, {
                    ignoreForCollisions: false,
                    collisionsWillMove: true,
                    gravity: ARROW_GRAVITY
                });

            }, 100)

            print('ARROW collisions??:::' + Entities.getEntityProperties(this.arrow, "collisionsWillMove").collisionsWillMove);

            print('ARROW velocity:::' + JSON.stringify(Entities.getEntityProperties(this.arrow, "velocity").velocity))
            this.arrow = null;
            this.hasArrowNotched = false;
            this.fire = null;
            this.arrowIsBurnning = false;

        },

        scaleArrowShotStrength: function(value, min1, max1, min2, max2) {
            return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
        },

        createNotchDetector: function() {
            var detectorPosition;
            var frontVector = Quat.getFront(this.bowProperties.rotation);
            var notchVectorForward = Vec3.multiply(frontVector, NOTCH_DETECTOR_OFFSET_FORWARD);
            var upVector = Quat.getUp(this.bowProperties.rotation);
            var notchVectorUp = Vec3.multiply(upVector, NOTCH_DETECTOR_OFFSET_UP);

            detectorPosition = Vec3.sum(this.bowProperties.position, notchVectorForward);
            detectorPosition = Vec3.sum(detectorPosition, notchVectorUp);

            var detectorProperties = {
                name: 'Hifi-NotchDetector',
                type: 'Box',
                visible: false,
                collisionsWillMove: false,
                ignoreForCollisions: true,
                dimensions: NOTCH_DETECTOR_DIMENSIONS,
                position: detectorPosition,
                color: {
                    red: 0,
                    green: 255,
                    blue: 0
                },
                userData: JSON.stringify({
                    hifiBowKey: {
                        bowID: this.entityID
                    }
                })
            };

            this.notchDetector = Entities.addEntity(detectorProperties);
        },

        updateNotchDetectorPosition: function() {
            var detectorPosition;
            var frontVector = Quat.getFront(this.bowProperties.rotation);
            var notchVectorForward = Vec3.multiply(frontVector, NOTCH_DETECTOR_OFFSET_FORWARD);
            var upVector = Quat.getUp(this.bowProperties.rotation);
            var notchVectorUp = Vec3.multiply(upVector, NOTCH_DETECTOR_OFFSET_UP);

            detectorPosition = Vec3.sum(this.bowProperties.position, notchVectorForward);
            detectorPosition = Vec3.sum(detectorPosition, notchVectorUp);

            this.notchDetectorPosition = detectorPosition;

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
                lifespan: 1
            });

            setEntityCustomData('hifiFireArrowKey', this.arrow, {
                fire: this.fire

            });


            this.arrowIsBurning = true;
        }

    };

    return new Bow();
});