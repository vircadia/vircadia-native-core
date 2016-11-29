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

/*global Script, Controller, SoundCache, Entities, getEntityCustomData, setEntityCustomData, MyAvatar, Vec3, Quat, Messages */

(function() {

    Script.include("/~/system/libraries/utils.js");

    var NULL_UUID = "{00000000-0000-0000-0000-000000000000}";

    var NOTCH_ARROW_SOUND_URL = Script.resolvePath('notch.wav');
    var SHOOT_ARROW_SOUND_URL = Script.resolvePath('String_release2.L.wav');
    var STRING_PULL_SOUND_URL = Script.resolvePath('Bow_draw.1.L.wav');
    var ARROW_HIT_SOUND_URL = Script.resolvePath('Arrow_impact1.L.wav');

    var ARROW_TIP_OFFSET = 0.47;
    var ARROW_GRAVITY = {
        x: 0,
        y: -4.8,
        z: 0
    };

    var ARROW_MODEL_URL = Script.resolvePath('newarrow_textured.fbx');
    var ARROW_COLLISION_HULL_URL = Script.resolvePath('newarrow_collision_hull.obj');

    var ARROW_DIMENSIONS = {
        x: 0.03,
        y: 0.03,
        z: 0.96
    };

    var ARROW_LIFETIME = 15; // seconds


    var TOP_NOTCH_OFFSET = 0.6;
    var BOTTOM_NOTCH_OFFSET = 0.6;

    var LINE_DIMENSIONS = {
        x: 5,
        y: 5,
        z: 5
    };

    var DRAW_STRING_THRESHOLD = 0.80;
    var DRAW_STRING_PULL_DELTA_HAPTIC_PULSE = 0.09;
    var DRAW_STRING_MAX_DRAW = 0.7;
    var NEAR_TO_RELAXED_KNOCK_DISTANCE = 0.5; // if the hand is this close, rez the arrow
    var NEAR_TO_RELAXED_SCHMITT = 0.05;

    var NOTCH_OFFSET_FORWARD = 0.08;
    var NOTCH_OFFSET_UP = 0.035;

    var SHOT_SCALE = {
        min1: 0,
        max1: 0.6,
        min2: 1,
        max2: 15
    };

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

    const STRING_NAME = 'Hifi-Bow-String';
    const ARROW_NAME = 'Hifi-Arrow';

    const STATE_IDLE = 0;
    const STATE_ARROW_KNOCKED = 1;
    const STATE_ARROW_GRABBED = 2;
    const STATE_ARROW_GRABBED_AND_PULLED = 3;

    Bow.prototype = {
        topString: null,
        aiming: false,
        arrowTipPosition: null,
        preNotchString: null,
        arrow: null,
        stringData: {
            currentColor: {
                red: 255,
                green: 255,
                blue: 255
            }
        },

        state: STATE_IDLE,
        sinceLastUpdate: 0,
        preload: function(entityID) {
            this.entityID = entityID;
            this.stringPullSound = SoundCache.getSound(STRING_PULL_SOUND_URL);
            this.shootArrowSound = SoundCache.getSound(SHOOT_ARROW_SOUND_URL);
            this.arrowHitSound = SoundCache.getSound(ARROW_HIT_SOUND_URL);
            this.arrowNotchSound = SoundCache.getSound(NOTCH_ARROW_SOUND_URL);
            var userData = Entities.getEntityProperties(this.entityID, ["userData"]).userData;
            this.userData = JSON.parse(userData);
            var children = Entities.getChildrenIDs(this.entityID);
            children.forEach(function(childID) {
                var childName = Entities.getEntityProperties(childID, ["name"]).name;
                if (childName == "Hifi-Bow-Pre-Notch-String") {
                    this.preNotchString = children[0];
                }
            });
        },

        unload: function() {
            this.deleteStrings();
            Entities.deleteEntity(this.arrow);
        },

        startEquip: function(entityID, args) {
            this.hand = args[0];
            // var avatarID = args[1];

            if (this.hand === 'left') {
                this.getStringHandPosition = function() { return _this.getControllerLocation("right").position; };
            } else if (this.hand === 'right') {
                this.getStringHandPosition = function() { return _this.getControllerLocation("left").position; };
            }

            var data = getEntityCustomData('grabbableKey', this.entityID, {});
            data.grabbable = false;
            setEntityCustomData('grabbableKey', this.entityID, data);
            Entities.editEntity(_this.entityID, {
                collidesWith: ""
            });

            //make sure the string is ready
            if (!this.preNotchString) {
                this.createPreNotchString();
            }
            var preNotchStringProps = Entities.getEntityProperties(this.preNotchString);
            if (!preNotchStringProps || preNotchStringProps.name != "Hifi-Bow-Pre-Notch-String") {
                this.createPreNotchString();
            }
            Entities.editEntity(this.preNotchString, {
                visible: true
            });
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

            this.checkStringHand();
        },
        releaseEquip: function(entityID, args) {
            Messages.sendMessage('Hifi-Hand-Disabler', "none");

            this.stringDrawn = false;
            this.deleteStrings();

            var data = getEntityCustomData('grabbableKey', this.entityID, {});
            data.grabbable = true;
            setEntityCustomData('grabbableKey', this.entityID, data);
            Entities.deleteEntity(this.arrow);
            Entities.editEntity(_this.entityID, {
                collidesWith: "static,dynamic,kinematic,otherAvatar,myAvatar"
            });
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
                parentID: this.entityID,
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
                    localAngularVelocity: {
                        x: 0,
                        y: 0,
                        z: 0
                    },
                    localVelocity: {
                        x: 0,
                        y: 0,
                        z: 0
                    },
                    gravity: {
                        x: 0,
                        y: 0,
                        z: 0
                    },
                    parentID: entityB,
                    dynamic: false,
                    collisionless: true,
                    collidesWith: ""
                });
                Script.removeEventHandler(arrow, "collisionWithEntity", makeArrowStick);
            };

            Script.addEventHandler(arrow, "collisionWithEntity", makeArrowStick);

            return arrow;
        },

        createPreNotchString: function() {
            this.preNotchString = Entities.addEntity({
                "collisionless": 1,
                "dimensions": { "x": 5, "y": 5, "z": 5 },
                "ignoreForCollisions": 1,
                "linePoints": [ { "x": 0, "y": 0, "z": 0 }, { "x": 0, "y": -1.2, "z": 0 } ],
                "lineWidth": 5,
                "name": "Hifi-Bow-Pre-Notch-String",
                "parentID": this.entityID,
                "localPosition": { "x": 0, "y": 0.6, "z": 0.1 },
                "localRotation": { "w": 1, "x": 0, "y": 0, "z": 0 },
                "type": "Line",
                "userData": "{\"grabbableKey\":{\"grabbable\":false}}"
            });
        },

        createStrings: function() {
            if (!this.topString) {
                this.createTopString();
                Entities.editEntity(this.preNotchString, { visible: false });
            }
        },

        createTopString: function() {
            var stringProperties = {
                name: 'Hifi-Bow-Top-String',
                type: 'Line',
                position: Vec3.sum(this.bowProperties.position, TOP_NOTCH_OFFSET),
                dimensions: LINE_DIMENSIONS,
                dynamic: false,
                collisionless: true,
                lineWidth: 5,
                color: this.stringData.currentColor,
                userData: JSON.stringify({
                    grabbableKey: {
                        grabbable: false
                    }
                })
            };

            this.topString = Entities.addEntity(stringProperties);
        },

        deleteStrings: function() {
            if (this.topString) {
                Entities.deleteEntity(this.topString);
                this.topString = null;
                Entities.editEntity(this.preNotchString, { visible: true });
            }
        },

        drawStrings: function() {
            this.createStrings();

            var upVector = Quat.getUp(this.bowProperties.rotation);
            var upOffset = Vec3.multiply(upVector, TOP_NOTCH_OFFSET);
            var downVector = Vec3.multiply(-1, Quat.getUp(this.bowProperties.rotation));
            var downOffset = Vec3.multiply(downVector, BOTTOM_NOTCH_OFFSET);
            var backOffset = Vec3.multiply(-0.1, Quat.getFront(this.bowProperties.rotation));

            var topStringPosition = Vec3.sum(this.bowProperties.position, upOffset);
            this.topStringPosition = Vec3.sum(topStringPosition, backOffset);
            var bottomStringPosition = Vec3.sum(this.bowProperties.position, downOffset);
            this.bottomStringPosition = Vec3.sum(bottomStringPosition, backOffset);

            var lineVectors = this.getLocalLineVectors();

            Entities.editEntity(this.topString, {
                position: this.topStringPosition,
                linePoints: [{ x: 0, y: 0, z: 0 }, lineVectors[0], lineVectors[1]]
            });
        },

        getLocalLineVectors: function() {
            var topVector = Vec3.subtract(this.arrowRearPosition, this.topStringPosition);
            var bottomVector = Vec3.subtract(this.bottomStringPosition, this.topStringPosition);
            return [topVector, bottomVector];
        },

        getControllerLocation: function (controllerHand) {
            var standardControllerValue =
                (controllerHand === "right") ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
            var pose = Controller.getPoseValue(standardControllerValue);
            var orientation = Quat.multiply(MyAvatar.orientation, pose.rotation);
            var position = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, pose.translation), MyAvatar.position);
            return {position: position, orientation: orientation};
        },

        checkStringHand: function() {
            //invert the hands because our string will be held with the opposite hand of the first one we pick up the bow with
            this.triggerValue = Controller.getValue(TRIGGER_CONTROLS[(this.hand === 'left') ? 1 : 0]);

            this.bowProperties = Entities.getEntityProperties(this.entityID);
            var notchPosition = this.getNotchPosition(this.bowProperties);
            var stringHandPosition = this.getStringHandPosition();
            var handToNotch = Vec3.subtract(notchPosition, stringHandPosition);
            var pullBackDistance = Vec3.length(handToNotch);

            if (this.state === STATE_IDLE) {
                this.pullBackDistance = 0;

                this.resetStringToIdlePosition();
                //this.deleteStrings();
                if (pullBackDistance < (NEAR_TO_RELAXED_KNOCK_DISTANCE - NEAR_TO_RELAXED_SCHMITT) && !this.backHandBusy) {
                    //the first time aiming the arrow
                    var handToDisable = (this.hand === 'right' ? 'left' : 'right');
                    Messages.sendLocalMessage('Hifi-Hand-Disabler', handToDisable);
                    this.arrow = this.createArrow();
                    this.playStringPullSound();
                    this.state = STATE_ARROW_KNOCKED;
                }
            }
            if (this.state === STATE_ARROW_KNOCKED) {

                if (pullBackDistance >= (NEAR_TO_RELAXED_KNOCK_DISTANCE + NEAR_TO_RELAXED_SCHMITT)) {
                    // delete the unpulled arrow
                    Messages.sendLocalMessage('Hifi-Hand-Disabler', "none");
                    Entities.deleteEntity(this.arrow);
                    this.arrow = null;
                    this.state = STATE_IDLE;
                } else if (this.triggerValue >= DRAW_STRING_THRESHOLD) {
                    // they've grabbed the arrow
                    this.pullBackDistance = 0;
                    this.state = STATE_ARROW_GRABBED;
                } else {
                    this.updateString();
                    this.updateArrowPositionInNotch(false, false);
                }
            }
            if (this.state === STATE_ARROW_GRABBED) {
                if (this.triggerValue < DRAW_STRING_THRESHOLD) {
                    // they let go without pulling
                    this.state = STATE_ARROW_KNOCKED;
                } else if (pullBackDistance >= (NEAR_TO_RELAXED_KNOCK_DISTANCE + NEAR_TO_RELAXED_SCHMITT)) {
                    // they've grabbed the arrow and pulled it
                    this.state = STATE_ARROW_GRABBED_AND_PULLED;
                } else {
                    this.updateString();
                    this.updateArrowPositionInNotch(false, true);
                }
            }
            if (this.state === STATE_ARROW_GRABBED_AND_PULLED) {
                if (pullBackDistance < (NEAR_TO_RELAXED_KNOCK_DISTANCE + NEAR_TO_RELAXED_SCHMITT)) {
                    // they unpulled without firing
                    this.state = STATE_ARROW_GRABBED;
                } else if (this.triggerValue < DRAW_STRING_THRESHOLD) {
                    // they've fired the arrow
                    Messages.sendLocalMessage('Hifi-Hand-Disabler', "none");
                    this.updateArrowPositionInNotch(true, true);
                    this.state = STATE_IDLE;
                    this.resetStringToIdlePosition();
                } else {
                    this.updateString();
                    this.updateArrowPositionInNotch(false, true);
                }
            }
        },

        setArrowRearPosition: function(arrowPosition, arrowRotation) {
            var frontVector = Quat.getFront(arrowRotation);
            var frontOffset = Vec3.multiply(frontVector, -ARROW_TIP_OFFSET);
            var arrorRearPosition = Vec3.sum(arrowPosition, frontOffset);
            this.arrowRearPosition = arrorRearPosition;
            return arrorRearPosition;

        },

        getNotchPosition: function(bowProperties) {
            var frontVector = Quat.getFront(bowProperties.rotation);
            var notchVectorForward = Vec3.multiply(frontVector, NOTCH_OFFSET_FORWARD);
            var upVector = Quat.getUp(bowProperties.rotation);
            var notchVectorUp = Vec3.multiply(upVector, NOTCH_OFFSET_UP);
            var notchPosition = Vec3.sum(bowProperties.position, notchVectorForward);
            notchPosition = Vec3.sum(notchPosition, notchVectorUp);
            return notchPosition;
        },

        updateArrowPositionInNotch: function(shouldReleaseArrow, doHapticPulses) {
            //set the notch that the arrow should go through
            var notchPosition = this.getNotchPosition(this.bowProperties);
            //set the arrow rotation to be between the notch and other hand
            var stringHandPosition = this.getStringHandPosition();
            var handToNotch = Vec3.subtract(notchPosition, stringHandPosition);
            var arrowRotation = Quat.rotationBetween(Vec3.FRONT, handToNotch);

            var backHand = this.hand === 'left' ? 1 : 0;
            var pullBackDistance = Vec3.length(handToNotch);
            // pulse as arrow is drawn
            if (doHapticPulses &&
                Math.abs(pullBackDistance - this.pullBackDistance) > DRAW_STRING_PULL_DELTA_HAPTIC_PULSE) {
                Controller.triggerHapticPulse(1, 20, backHand);
                this.pullBackDistance = pullBackDistance;
            }
            // this.changeStringPullSoundVolume(pullBackDistance);

            if (pullBackDistance > DRAW_STRING_MAX_DRAW) {
                pullBackDistance = DRAW_STRING_MAX_DRAW;
            }

            // //pull the arrow back a bit
            // var pullBackOffset = Vec3.multiply(handToNotch, -pullBackDistance);
            // var arrowPosition = Vec3.sum(notchPosition, pullBackOffset);

            // // // move it forward a bit
            // var pushForwardOffset = Vec3.multiply(handToNotch, -ARROW_OFFSET);
            // var finalArrowPosition = Vec3.sum(arrowPosition, pushForwardOffset);

            //we draw strings to the rear of the arrow
            // this.setArrowRearPosition(finalArrowPosition, arrowRotation);

            var halfArrowVec = Vec3.multiply(Vec3.normalize(handToNotch), ARROW_DIMENSIONS.z / 2.0);
            var arrowPosition = Vec3.sum(stringHandPosition, halfArrowVec);
            this.setArrowRearPosition(arrowPosition, arrowRotation);

            //if we're not shooting, we're updating the arrow's orientation
            if (shouldReleaseArrow !== true) {
                Entities.editEntity(this.arrow, {
                    position: arrowPosition,
                    rotation: arrowRotation
                });
            }

            //shoot the arrow
            if (shouldReleaseArrow === true) { // && pullBackDistance >= (NEAR_TO_RELAXED_KNOCK_DISTANCE + NEAR_TO_RELAXED_SCHMITT)) {
                var arrowAge = Entities.getEntityProperties(this.arrow, ["age"]).age;

                //scale the shot strength by the distance you've pulled the arrow back and set its release velocity to be
                // in the direction of the v
                var arrowForce = this.scaleArrowShotStrength(pullBackDistance);
                var handToNotchNorm = Vec3.normalize(handToNotch);

                var releaseVelocity = Vec3.multiply(handToNotchNorm, arrowForce);
                // var releaseVelocity2 = Vec3.multiply()

                //make the arrow physical, give it gravity, a lifetime, and set our velocity
                var arrowProperties = {
                    dynamic: true,
                    collisionless: false,
                    collidesWith: "static,dynamic,otherAvatar", // workaround: not with kinematic --> no collision with bow
                    velocity: releaseVelocity,
                    parentID: NULL_UUID,
                    gravity: ARROW_GRAVITY,
                    lifetime: ARROW_LIFETIME + arrowAge,
                };

                //actually shoot the arrow and play its sound
                Entities.editEntity(this.arrow, arrowProperties);
                this.playShootArrowSound();

                Controller.triggerShortHapticPulse(1, backHand);

                Entities.addAction("travel-oriented", this.arrow, {
                    forward: { x: 0, y: 0, z: -1 },
                    angularTimeScale: 0.1,
                    tag: "arrow from hifi-bow",
                    ttl: ARROW_LIFETIME
                });


            } // else if (shouldReleaseArrow === true) {
                // they released without pulling back; just delete the arrow.
                // Entities.deleteEntity(this.arrow);
                // this.arrow = null;
            // }

            // if (shouldReleaseArrow === true) {
            //     //clear the strings back to only the single straight one
            //     Entities.editEntity(this.preNotchString, {
            //         visible: true
            //     });
            // }

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
            };

            this.stringPullInjector.options = audioProperties;
        },
        scaleSoundVolume: function(value) {
            var min1 = SHOT_SCALE.min1;
            var max1 = SHOT_SCALE.max1;
            var min2 = 0;
            var max2 = 0.2;
            return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
        },

        handleMessages: function(channel, message, sender) {
            if (sender !== MyAvatar.sessionUUID) {
                return;
            }
            if (channel !== 'Hifi-Object-Manipulation') {
                return;
            }
            try {
                var data = JSON.parse(message);
                var action = data.action;
                var hand = data.joint;
                var isBackHand = ((_this.hand == "left" && hand == "RightHand") ||
                                  (_this.hand == "right" && hand == "LeftHand"));
                if ((action == "equip" || action == "grab") && isBackHand) {
                    _this.backHandBusy = true;
                }
                if (action == "release" && isBackHand) {
                    _this.backHandBusy = false;
                }
            }  catch (e) {
                print("WARNING: bow.js -- error parsing Hifi-Object-Manipulation message: " + message);
            }
        }
    };

    var bow = new Bow();

    Messages.subscribe('Hifi-Object-Manipulation');
    Messages.messageReceived.connect(bow.handleMessages);

    return bow;
});
