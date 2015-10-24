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
// make arrows more visible
// make arrow rotate toward ground as it flies
// different model?  compound bow will look better in the HMD, and be easier to aim. this is what HTC uses in the longbow demo: http://www.turbosquid.com/3d-models/3d-model-bow-arrow/773106
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

    Bow.prototype = {
        isGrabbed: false,
        stringDrawn: false,
        hasArrow: false,
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

        createStrings: function() {
            this.createTopString();
            this.createBottomString();
        },

        deleteStrings: function() {
            Entities.deleteEntity(this.topString);
            Entities.deleteEntity(this.bottomString);
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


        },

        startNearGrab: function() {
            if (this.isGrabbed === true) {
                return false;
            }
            this.isGrabbed = true;
            this.initialHand = this.hand;
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
            this.checkStringHand();

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

        releaseArrow: function() {
            var handToHand = Vec3.subtract(this.stringData.grabHandPosition, this.stringData.handPosition);

            var arrowRotation = this.orientationOf(handToHand);

            // var forwardVec = Quat.getFront(this.arrow.rotation);
            // forwardVec = Vec3.normalize(forwardVec);
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

        },
        getLocalLineVectors: function() {
            var topVector = Vec3.subtract(this.stringData.handPosition, this.topStringPosition);
            var bottomVector = Vec3.subtract(this.stringData.handPosition, this.bottomStringPosition);
            return [topVector, bottomVector];
        },
        scaleArrowShotStrength: function(value, min1, max1, min2, max2) {
            return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
        }
    };

    return new Bow();
});