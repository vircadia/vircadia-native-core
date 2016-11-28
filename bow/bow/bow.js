//
//  bow.js
//
//  This script attaches to a bow that you can pick up with a hand controller.
//  Created by James B. Pollack @imgntn on 10/19/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    Script.include("utils.js");

    var NOTCH_ARROW_SOUND_URL = 'http://mpassets.highfidelity.com/32fc6d32-27a2-428e-937e-869f3f05e8e1-v1/notch.wav';
    var SHOOT_ARROW_SOUND_URL = 'http://mpassets.highfidelity.com/32fc6d32-27a2-428e-937e-869f3f05e8e1-v1/String_release2.L.wav';
    var STRING_PULL_SOUND_URL = 'http://mpassets.highfidelity.com/32fc6d32-27a2-428e-937e-869f3f05e8e1-v1/Bow_draw.1.L.wav';
    var ARROW_HIT_SOUND_URL = 'http://mpassets.highfidelity.com/32fc6d32-27a2-428e-937e-869f3f05e8e1-v1/Arrow_impact1.L.wav'

    var ARROW_DIMENSIONS = {
        x: 0.02,
        y: 0.02,
        z: 0.72
    };

    var ARROW_OFFSET = -0.44;
    var ARROW_TIP_OFFSET = 0.32;
    var ARROW_GRAVITY = {
        x: 0,
        y: -4.8,
        z: 0
    };

    var ARROW_MODEL_URL = "http://mpassets.highfidelity.com/32fc6d32-27a2-428e-937e-869f3f05e8e1-v1/newarrow_textured.fbx";
    var ARROW_COLLISION_HULL_URL =
        "http://mpassets.highfidelity.com/32fc6d32-27a2-428e-937e-869f3f05e8e1-v1/newarrow_collision_hull.obj";

    var ARROW_DIMENSIONS = {
        x: 0.02,
        y: 0.02,
        z: 0.64
    };


    var TOP_NOTCH_OFFSET = 0.6;
    var BOTTOM_NOTCH_OFFSET = 0.6;

    var LINE_DIMENSIONS = {
        x: 5,
        y: 5,
        z: 5
    };

    var DRAW_STRING_THRESHOLD = 0.80;

    var LEFT_TIP = 1;
    var RIGHT_TIP = 3;

    var NOTCH_OFFSET_FORWARD = 0.08;
    var NOTCH_OFFSET_UP = 0.035;

    var SHOT_SCALE = {
        min1: 0,
        max1: 0.6,
        min2: 1,
        max2: 15
    }

    var USE_DEBOUNCE = false;

    var TRIGGER_CONTROLS = [
        Controller.Standard.LT,
        Controller.Standard.RT,
    ];

    function interval() {
        var lastTime = new Date().getTime();

        return function getInterval() {
            var newTime = new Date().getTime();
            var delta = newTime - lastTime;
            lastTime = newTime;
            return delta;
        };
    }

    var checkInterval = interval();

    var _this;

    function Bow() {
        _this = this;
        return;
    }

    Bow.prototype = {
        stringDrawn: false,
        aiming: false,
        arrowTipPosition: null,
        preNotchString: null,
        hasArrowNotched: false,
        arrow: null,
        stringData: {
            currentColor: {
                red: 255,
                green: 255,
                blue: 255
            }
        },
        sinceLastUpdate: 0,
        preload: function(entityID) {
            this.entityID = entityID;
            this.stringPullSound = SoundCache.getSound(STRING_PULL_SOUND_URL);
            this.shootArrowSound = SoundCache.getSound(SHOOT_ARROW_SOUND_URL);
            this.arrowHitSound = SoundCache.getSound(ARROW_HIT_SOUND_URL);
            this.arrowNotchSound = SoundCache.getSound(NOTCH_ARROW_SOUND_URL);
            var userData = Entities.getEntityProperties(this.entityID).userData;
            this.userData = JSON.parse(userData);
            var children = Entities.getChildrenIDs(this.entityID);
            this.preNotchString = children[0];

        },

        unload: function() {
            this.deleteStrings();
            Entities.deleteEntity(this.arrow);
        },

        startNearGrab: function(entityID, args) {
            _this.startEquip(entityID, args);
        },

        continueNearGrab: function(entityID, args) {
            _this.continueEquip(entityID, args);
        },

        releaseGrab: function(entityID, args) {
            _this.releaseEquip(entityID, args);
        },

        startEquip: function(entityID, args) {
            this.hand = args[0];
            avatarID = args[1];

            //disable the opposite hand in handControllerGrab.js by message
            var handToDisable = this.hand === 'right' ? 'left' : 'right';
            Messages.sendMessage('Hifi-Hand-Disabler', handToDisable);

            var data = getEntityCustomData('grabbableKey', this.entityID, {});
            data.grabbable = false;
            setEntityCustomData('grabbableKey', this.entityID, data);
            Entities.editEntity(_this.entityID, {
                collidesWith: ""
            })

        },
        continueEquip: function(entityID, args) {
            this.deltaTime = checkInterval();

            //debounce during debugging -- maybe we're updating too fast?
            if (USE_DEBOUNCE === true) {
                this.sinceLastUpdate = this.sinceLastUpdate + this.deltaTime;

                if (this.sinceLastUpdate > 60) {
                    this.sinceLastUpdate = 0;
                } else {
                    return;
                }
            }

            this.bowProperties = Entities.getEntityProperties(this.entityID);

            if (this.aiming === true) {
                Entities.editEntity(this.preNotchString, {
                    visible: false
                })
            } else {
                Entities.editEntity(this.preNotchString, {
                    visible: true
                })
            }

            this.checkStringHand();

        },
        releaseEquip: function(entityID, args) {
            Messages.sendMessage('Hifi-Hand-Disabler', "none")

            this.stringDrawn = false;
            this.deleteStrings();

            var data = getEntityCustomData('grabbableKey', this.entityID, {});
            data.grabbable = true;
            setEntityCustomData('grabbableKey', this.entityID, data);
            Entities.deleteEntity(this.arrow);
            this.aiming = false;
            this.hasArrowNotched = false;
            Entities.editEntity(_this.entityID, {
                collidesWith: "static,dynamic,kinematic,otherAvatar,myAvatar"
            })
        },

        createArrow: function() {
            this.playArrowNotchSound();

            var arrow = Entities.addEntity({
                name: 'Hifi-Arrow',
                type: 'Model',
                modelURL: ARROW_MODEL_URL,
                shapeType: 'compound',
                compoundShapeURL: ARROW_COLLISION_HULL_URL,
                dimensions: ARROW_DIMENSIONS,
                position: this.bowProperties.position,
                dynamic: false,
                collisionless: true,
                collisionSoundURL: ARROW_HIT_SOUND_URL,
                damping: 0.01,
                userData: JSON.stringify({
                    grabbableKey: {
                        grabbable: false
                    },
                    creatorSessionUUID: MyAvatar.sessionUUID
                })
            });

            var makeArrowStick = function(entityA, entityB, collision) {
                Entities.editEntity(entityA, {
                    angularVelocity: {
                        x: 0,
                        y: 0,
                        z: 0
                    },
                    velocity: {
                        x: 0,
                        y: 0,
                        z: 0
                    },
                    gravity: {
                        x: 0,
                        y: 0,
                        z: 0
                    },
                    position: collision.contactPoint,
                    dynamic: false
                })
                Script.removeEventHandler(arrow, "collisionWithEntity", makeArrowStick)
            }

            Script.addEventHandler(arrow, "collisionWithEntity", makeArrowStick);

            return arrow
        },

        createStrings: function() {
            this.createTopString();
            this.createBottomString();
        },

        createTopString: function() {
            var stringProperties = {
                name: 'Hifi-Bow-Top-String',
                type: 'Line',
                position: Vec3.sum(this.bowProperties.position, TOP_NOTCH_OFFSET),
                dimensions: LINE_DIMENSIONS,
                dynamic: false,
                collisionless: true,
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
                name: 'Hifi-Bow-Bottom-String',
                type: 'Line',
                position: Vec3.sum(this.bowProperties.position, BOTTOM_NOTCH_OFFSET),
                dimensions: LINE_DIMENSIONS,
                dynamic: false,
                collisionless: true,
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
            var topVector = Vec3.subtract(this.arrowRearPosition, this.topStringPosition);
            var bottomVector = Vec3.subtract(this.arrowRearPosition, this.bottomStringPosition);
            return [topVector, bottomVector];
        },

        checkStringHand: function() {
            //invert the hands because our string will be held with the opposite hand of the first one we pick up the bow with
            var triggerLookup;
            if (this.hand === 'left') {
                triggerLookup = 1;
                this.getStringHandPosition = MyAvatar.getRightPalmPosition;
            } else if (this.hand === 'right') {
                this.getStringHandPosition = MyAvatar.getLeftPalmPosition;
                triggerLookup = 0;
            }

            this.triggerValue = Controller.getValue(TRIGGER_CONTROLS[triggerLookup]);


            if (this.triggerValue < DRAW_STRING_THRESHOLD && this.stringDrawn === true) {
                // firing the arrow

                this.drawStrings();
                this.hasArrowNotched = false;
                this.aiming = false;
                this.stringDrawn = false;
                this.updateArrowPositionInNotch(true);


            } else if (this.triggerValue > DRAW_STRING_THRESHOLD && this.stringDrawn === true) {
                //continuing to aim the arrow

                this.aiming = true;
                this.drawStrings();
                this.updateArrowPositionInNotch();

            } else if (this.triggerValue > DRAW_STRING_THRESHOLD && this.stringDrawn === false) {
                this.arrow = this.createArrow();
                this.playStringPullSound();

                //the first time aiming the arrow
                this.stringDrawn = true;
                this.createStrings();
                this.drawStrings();
                this.updateArrowPositionInNotch();

            }
        },

        setArrowRearPosition: function(arrowPosition, arrowRotation) {
            var frontVector = Quat.getFront(arrowRotation);
            var frontOffset = Vec3.multiply(frontVector, -ARROW_TIP_OFFSET);
            var arrorRearPosition = Vec3.sum(arrowPosition, frontOffset);
            this.arrowRearPosition = arrorRearPosition;
            return arrorRearPosition;

        },

        updateArrowPositionInNotch: function(shouldReleaseArrow) {
            var bowProperties = Entities.getEntityProperties(this.entityID);
            //set the notch that the arrow should go through
            var frontVector = Quat.getFront(bowProperties.rotation);
            var notchVectorForward = Vec3.multiply(frontVector, NOTCH_OFFSET_FORWARD);
            var upVector = Quat.getUp(bowProperties.rotation);
            var notchVectorUp = Vec3.multiply(upVector, NOTCH_OFFSET_UP);
            var notchPosition;
            notchPosition = Vec3.sum(bowProperties.position, notchVectorForward);
            notchPosition = Vec3.sum(notchPosition, notchVectorUp);

            //set the arrow rotation to be between the notch and other hand
            var stringHandPosition = this.getStringHandPosition();
            var handToNotch = Vec3.subtract(notchPosition, stringHandPosition);
            var arrowRotation = Quat.rotationBetween(Vec3.FRONT, handToNotch);



            var pullBackDistance = Vec3.length(handToNotch);
            // this.changeStringPullSoundVolume(pullBackDistance);

            if (pullBackDistance > 0.6) {
                pullBackDistance = 0.6;
            }

            // //pull the arrow back a bit
            var pullBackOffset = Vec3.multiply(handToNotch, -pullBackDistance);
            var arrowPosition = Vec3.sum(notchPosition, pullBackOffset);

            // // move it forward a bit
            var pushForwardOffset = Vec3.multiply(handToNotch, -ARROW_OFFSET);
            var finalArrowPosition = Vec3.sum(arrowPosition, pushForwardOffset);

            //we draw strings to the rear of the arrow
            this.setArrowRearPosition(finalArrowPosition, arrowRotation);

            //if we're not shooting, we're updating the arrow's orientation
            if (shouldReleaseArrow !== true) {
                Entities.editEntity(this.arrow, {
                    position: finalArrowPosition,
                    rotation: arrowRotation
                })
            }

            //shoot the arrow
            if (shouldReleaseArrow === true) {
                var arrowProps = Entities.getEntityProperties(this.arrow);

                //scale the shot strength by the distance you've pulled the arrow back and set its release velocity to be in the direction of the v
                var arrowForce = this.scaleArrowShotStrength(pullBackDistance);
                var handToNotch = Vec3.normalize(handToNotch);

                var releaseVelocity = Vec3.multiply(handToNotch, arrowForce);
                // var releaseVelocity2 = Vec3.multiply()

                //make the arrow physical, give it gravity, a lifetime, and set our velocity
                var arrowProperties = {
                    dynamic: true,
                    collisionless: false,
                    collidesWith: "static,dynamic,otherAvatar", // workaround: not with kinematic --> no collision with bow
                    velocity: releaseVelocity,
                    gravity: ARROW_GRAVITY,
                    lifetime: 10,
                    // position: arrowProps.position,
                    // rotation: arrowProps.rotation
                };


                //actually shoot the arrow and play its sound
                Entities.editEntity(this.arrow, arrowProperties);
                this.playShootArrowSound();

                var backHand = this.hand === 'left' ? 1 : 0;
                var haptic = Controller.triggerShortHapticPulse(1, backHand);

                //clear the strings back to only the single straight one
                this.deleteStrings();
                Entities.editEntity(this.preNotchString, {
                    visible: true
                });

            }

        },

        scaleArrowShotStrength: function(value) {
            var min1 = SHOT_SCALE.min1;
            var max1 = SHOT_SCALE.max1;
            var min2 = SHOT_SCALE.min2;
            var max2 = SHOT_SCALE.max2;
            return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
        },

        playStringPullSound: function() {
            var audioProperties = {
                volume: 0.10,
                position: this.bowProperties.position
            };
            this.stringPullInjector = Audio.playSound(this.stringPullSound, audioProperties);
        },

        playShootArrowSound: function(sound) {
            var audioProperties = {
                volume: 0.15,
                position: this.bowProperties.position
            };
            Audio.playSound(this.shootArrowSound, audioProperties);
        },

        playArrowNotchSound: function() {
            var audioProperties = {
                volume: 0.15,
                position: this.bowProperties.position
            };
            Audio.playSound(this.arrowNotchSound, audioProperties);
        },

        changeStringPullSoundVolume: function(pullBackDistance) {
            var audioProperties = {
                volume: this.scaleSoundVolume(pullBackDistance),
                position: this.bowProperties.position
            }

            this.stringPullInjector.options = audioProperties;
        },
        scaleSoundVolume: function(value) {
            var min1 = SHOT_SCALE.min1;
            var max1 = SHOT_SCALE.max1;
            var min2 = 0;
            var max2 = 0.2;
            return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
        }

    };

    return new Bow();
});
