//
//  Created by Seth Alves on 2016-9-7
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/* global MyAvatar, Vec3, Controller, Quat */

var GRAB_COMMUNICATIONS_SETTING = "io.highfidelity.isFarGrabbing";
setGrabCommunications = function setFarGrabCommunications(on) {
    Settings.setValue(GRAB_COMMUNICATIONS_SETTING, on ? "on" : "");
}
getGrabCommunications = function getFarGrabCommunications() {
    return !!Settings.getValue(GRAB_COMMUNICATIONS_SETTING, "");
}

// this offset needs to match the one in libraries/display-plugins/src/display-plugins/hmd/HmdDisplayPlugin.cpp:378
var GRAB_POINT_SPHERE_OFFSET = { x: 0.04, y: 0.13, z: 0.039 };  // x = upward, y = forward, z = lateral

getGrabPointSphereOffset = function(handController) {
    if (handController === Controller.Standard.RightHand) {
        return GRAB_POINT_SPHERE_OFFSET;
    }
    return {
        x: GRAB_POINT_SPHERE_OFFSET.x * -1,
        y: GRAB_POINT_SPHERE_OFFSET.y,
        z: GRAB_POINT_SPHERE_OFFSET.z
    };
};

// controllerWorldLocation is where the controller would be, in-world, with an added offset
getControllerWorldLocation = function (handController, doOffset) {
    var orientation;
    var position;
    var pose = Controller.getPoseValue(handController);
    var valid = pose.valid;
    var controllerJointIndex;
    if (pose.valid) {
        if (handController === Controller.Standard.RightHand) {
            controllerJointIndex = MyAvatar.getJointIndex("_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND");
        } else {
            controllerJointIndex = MyAvatar.getJointIndex("_CAMERA_RELATIVE_CONTROLLER_LEFTHAND");
        }
        orientation = Quat.multiply(MyAvatar.orientation, MyAvatar.getAbsoluteJointRotationInObjectFrame(controllerJointIndex));
        position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, MyAvatar.getAbsoluteJointTranslationInObjectFrame(controllerJointIndex)));

        // add to the real position so the grab-point is out in front of the hand, a bit
        if (doOffset) {
            var offset = getGrabPointSphereOffset(handController);
            position = Vec3.sum(position, Vec3.multiplyQbyV(orientation, offset));
        }

    } else if (!HMD.isHandControllerAvailable()) {
        // NOTE: keep this offset in sync with scripts/system/controllers/handControllerPointer.js:493
        var VERTICAL_HEAD_LASER_OFFSET = 0.1;
        position = Vec3.sum(Camera.position, Vec3.multiplyQbyV(Camera.orientation, {x: 0, y: VERTICAL_HEAD_LASER_OFFSET, z: 0}));
        orientation = Quat.multiply(Camera.orientation, Quat.angleAxis(-90, { x: 1, y: 0, z: 0 }));
        valid = true;
    }

    return {position: position,
            translation: position,
            orientation: orientation,
            rotation: orientation,
            valid: valid};
};



//
//
//
//
//
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


function getControllerLocation(controllerHand) {
    var standardControllerValue =
        (controllerHand === "right") ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
    return getControllerWorldLocation(standardControllerValue, true);
};

(function() {

    Script.include("/~/system/libraries/utils.js");

    const NULL_UUID = "{00000000-0000-0000-0000-000000000000}";

    const NOTCH_ARROW_SOUND_URL = Script.resolvePath('notch.wav');
    const SHOOT_ARROW_SOUND_URL = Script.resolvePath('String_release2.L.wav');
    const STRING_PULL_SOUND_URL = Script.resolvePath('Bow_draw.1.L.wav');
    const ARROW_HIT_SOUND_URL = Script.resolvePath('Arrow_impact1.L.wav');

    const ARROW_MODEL_URL = Script.resolvePath('models/arrow.baked.fbx');
    const ARROW_DIMENSIONS = {
        x: 0.20,
        y: 0.19,
        z: 0.93
    };

    const MIN_ARROW_SPEED = 3.0;
    const MAX_ARROW_SPEED = 30.0;

    const ARROW_TIP_OFFSET = 0.47;
    const ARROW_GRAVITY = {
        x: 0,
        y: -9.8,
        z: 0
    };


    const ARROW_LIFETIME = 15; // seconds
    const ARROW_PARTICLE_LIFESPAN = 2; // seconds

    const TOP_NOTCH_OFFSET = 0.6;
    const BOTTOM_NOTCH_OFFSET = 0.6;

    const LINE_DIMENSIONS = {
        x: 5.0,
        y: 5.0,
        z: 5.0
    };

    const DRAW_STRING_THRESHOLD = 0.80;
    const DRAW_STRING_PULL_DELTA_HAPTIC_PULSE = 0.09;
    const DRAW_STRING_MAX_DRAW = 0.7;


    const MIN_ARROW_DISTANCE_FROM_BOW_REST = 0.2;
    const MAX_ARROW_DISTANCE_FROM_BOW_REST = ARROW_DIMENSIONS.z - 0.2;
    const MIN_HAND_DISTANCE_FROM_BOW_TO_KNOCK_ARROW = 0.25;
    const MIN_ARROW_DISTANCE_FROM_BOW_REST_TO_SHOOT = 0.30;

    const NOTCH_OFFSET_FORWARD = 0.08;
    const NOTCH_OFFSET_UP = 0.035;

    const SHOT_SCALE = {
        min1: 0.0,
        max1: 0.6,
        min2: 1.0,
        max2: 15.0
    };

    const USE_DEBOUNCE = false;

    const TRIGGER_CONTROLS = [
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
    const ARROW_NAME = 'Hifi-Arrow-projectile';

    const STATE_IDLE = 0;
    const STATE_ARROW_GRABBED = 1;

    Bow.prototype = {
        topString: null,
        aiming: false,
        arrowTipPosition: null,
        preNotchString: null,
        stringID: null,
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
            print(userData);
            this.userData = JSON.parse(userData);
            this.stringID = null;
        },

        unload: function() {
            Messages.sendLocalMessage('Hifi-Hand-Disabler', "none");
            Entities.deleteEntity(this.arrow);
        },

        startEquip: function(entityID, args) {
            this.hand = args[0];
            this.bowHand = args[0];
            this.stringHand = this.bowHand === "right" ? "left" : "right";

            Entities.editEntity(_this.entityID, {
                collidesWith: "",
            });

            var data = getEntityCustomData('grabbableKey', this.entityID, {});
            data.grabbable = false;
            setEntityCustomData('grabbableKey', this.entityID, data);

            this.initString();

            var self = this;
            this.updateIntervalID = Script.setInterval(function() { self.update(); }, 11);
        },

        getStringHandPosition: function() {
            return getControllerLocation(this.stringHand).position;
        },

        releaseEquip: function(entityID, args) {
            Script.clearInterval(this.updateIntervalID);
            this.updateIntervalID = null;

            Messages.sendLocalMessage('Hifi-Hand-Disabler', "none");

            var data = getEntityCustomData('grabbableKey', this.entityID, {});
            data.grabbable = true;
            setEntityCustomData('grabbableKey', this.entityID, data);
            Entities.deleteEntity(this.arrow);
            this.resetStringToIdlePosition();
            this.destroyArrow();
            Entities.editEntity(_this.entityID, {
                collidesWith: "static,dynamic,kinematic,otherAvatar,myAvatar"
            });
        },

        update: function(entityID) {
            var self = this;
            self.deltaTime = checkInterval();
            //debounce during debugging -- maybe we're updating too fast?
            if (USE_DEBOUNCE === true) {
                self.sinceLastUpdate = self.sinceLastUpdate + self.deltaTime;

                if (self.sinceLastUpdate > 60) {
                    self.sinceLastUpdate = 0;
                } else {
                    return;
                }
            }

            //invert the hands because our string will be held with the opposite hand of the first one we pick up the bow with
            this.triggerValue = Controller.getValue(TRIGGER_CONTROLS[(this.hand === 'left') ? 1 : 0]);

            this.bowProperties = Entities.getEntityProperties(this.entityID, ['position', 'rotation']);
            var notchPosition = this.getNotchPosition(this.bowProperties);
            var stringHandPosition = this.getStringHandPosition();
            var handToNotch = Vec3.subtract(notchPosition, stringHandPosition);
            var pullBackDistance = Vec3.length(handToNotch);

            if (this.state === STATE_IDLE) {
                this.pullBackDistance = 0;

                this.resetStringToIdlePosition();
                if (this.triggerValue >= DRAW_STRING_THRESHOLD && pullBackDistance < MIN_HAND_DISTANCE_FROM_BOW_TO_KNOCK_ARROW && !this.backHandBusy) {
                    //the first time aiming the arrow
                    var handToDisable = (this.hand === 'right' ? 'left' : 'right');
                    this.state = STATE_ARROW_GRABBED;
                }
            }

            if (this.state === STATE_ARROW_GRABBED) {
                if (!this.arrow) {
                    var handToDisable = (this.hand === 'right' ? 'left' : 'right');
                    Messages.sendLocalMessage('Hifi-Hand-Disabler', handToDisable);
                    this.playArrowNotchSound();
                    this.arrow = this.createArrow();
                    this.playStringPullSound();
                }

                if (this.triggerValue < DRAW_STRING_THRESHOLD) {
                    if (pullBackDistance >= (MIN_ARROW_DISTANCE_FROM_BOW_REST_TO_SHOOT)) {
                        // The arrow has been pulled far enough back that we can release it
                        Messages.sendLocalMessage('Hifi-Hand-Disabler', "none");
                        this.updateArrowPositionInNotch(true, true);
                        this.arrow = null;
                        this.state = STATE_IDLE;
                        this.resetStringToIdlePosition();
                    } else {
                        // The arrow has not been pulled far enough back so we just remove the arrow
                        Messages.sendLocalMessage('Hifi-Hand-Disabler', "none");
                        Entities.deleteEntity(this.arrow);
                        this.arrow = null;
                        this.state = STATE_IDLE;
                        this.resetStringToIdlePosition();
                    }
                } else {
                    this.updateArrowPositionInNotch(false, true);
                    this.updateString();
                }
            }
        },

        destroyArrow: function() {
            var children = Entities.getChildrenIDs(this.entityID);
            children.forEach(function(childID) {
                var childName = Entities.getEntityProperties(childID, ["name"]).name;
                if (childName == ARROW_NAME) {
                    Entities.deleteEntity(childID);
                    // Continue iterating through children in case we've ended up in
                    // a bad state where there are multiple arrows.
                }
            });
        },

        createArrow: function() {
            this.playArrowNotchSound();

            var arrow = Entities.addEntity({
                name: ARROW_NAME,
                type: 'Model',
                modelURL: ARROW_MODEL_URL,
                shapeType: 'simple-compound',
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

        initString: function() {
            // Check for existence of string
            var children = Entities.getChildrenIDs(this.entityID);
            children.forEach(function(childID) {
                var childName = Entities.getEntityProperties(childID, ["name"]).name;
                if (childName == STRING_NAME) {
                    this.stringID = childID;
                }
            });

            // If thie string wasn't found, create it
            if (this.stringID === null) {
                this.stringID = Entities.addEntity({
                    collisionless: true,
                    dimensions: { "x": 5, "y": 5, "z": 5 },
                    ignoreForCollisions: 1,
                    linePoints: [ { "x": 0, "y": 0, "z": 0 }, { "x": 0, "y": -1.2, "z": 0 } ],
                    lineWidth: 5,
                    color: { red: 153, green: 102, blue: 51 },
                    name: STRING_NAME,
                    parentID: this.entityID,
                    localPosition: { "x": 0, "y": 0.6, "z": 0.1 },
                    localRotation: { "w": 1, "x": 0, "y": 0, "z": 0 },
                    type: 'Line',
                    userData: JSON.stringify({
                        grabbableKey: {
                            grabbable: false
                        }
                    })
                });
            }

            this.resetStringToIdlePosition();
        },

        // This resets the string to a straight line
        resetStringToIdlePosition: function() {
            Entities.editEntity(this.stringID, {
                linePoints: [ { "x": 0, "y": 0, "z": 0 }, { "x": 0, "y": -1.2, "z": 0 } ],
                lineWidth: 5,
                localPosition: { "x": 0, "y": 0.6, "z": 0.1 },
                localRotation: { "w": 1, "x": 0, "y": 0, "z": 0 },
            });
        },

        updateString: function() {
            var upVector = Quat.getUp(this.bowProperties.rotation);
            var upOffset = Vec3.multiply(upVector, TOP_NOTCH_OFFSET);
            var downVector = Vec3.multiply(-1, Quat.getUp(this.bowProperties.rotation));
            var downOffset = Vec3.multiply(downVector, BOTTOM_NOTCH_OFFSET);
            var backOffset = Vec3.multiply(-0.1, Quat.getFront(this.bowProperties.rotation));

            var topStringPosition = Vec3.sum(this.bowProperties.position, upOffset);
            this.topStringPosition = Vec3.sum(topStringPosition, backOffset);
            var bottomStringPosition = Vec3.sum(this.bowProperties.position, downOffset);
            this.bottomStringPosition = Vec3.sum(bottomStringPosition, backOffset);

            var stringProps = Entities.getEntityProperties(this.stringID, ['position', 'rotation']);
            var handPositionLocal = Vec3.subtract(this.arrowRearPosition, stringProps.position);
            handPositionLocal = Vec3.multiplyQbyV(Quat.inverse(stringProps.rotation), handPositionLocal);

            var linePoints = [
                { x: 0, y: 0, z: 0 },
                handPositionLocal,
                { x: 0, y: -1.2, z: 0 },
            ];

            Entities.editEntity(this.stringID, {
                linePoints: linePoints,
            });
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

            if (pullBackDistance > DRAW_STRING_MAX_DRAW) {
                pullBackDistance = DRAW_STRING_MAX_DRAW;
            }

            var handToNotchDistance = Vec3.length(handToNotch);
            var stringToNotchDistance = Math.max(MIN_ARROW_DISTANCE_FROM_BOW_REST, Math.min(MAX_ARROW_DISTANCE_FROM_BOW_REST, handToNotchDistance));
            var halfArrowVec = Vec3.multiply(Vec3.normalize(handToNotch), ARROW_DIMENSIONS.z / 2.0);
            var offset = Vec3.subtract(notchPosition, Vec3.multiply(Vec3.normalize(handToNotch), stringToNotchDistance - ARROW_DIMENSIONS.z / 2.0));

            var arrowPosition = offset;

            // Set arrow rear position
            var frontVector = Quat.getFront(arrowRotation);
            var frontOffset = Vec3.multiply(frontVector, -ARROW_TIP_OFFSET);
            var arrorRearPosition = Vec3.sum(arrowPosition, frontOffset);
            this.arrowRearPosition = arrorRearPosition;

            //if we're not shooting, we're updating the arrow's orientation
            if (shouldReleaseArrow !== true) {
                Entities.editEntity(this.arrow, {
                    position: arrowPosition,
                    rotation: arrowRotation
                });
            } else {
                //shoot the arrow
                var arrowAge = Entities.getEntityProperties(this.arrow, ["age"]).age;

                //scale the shot strength by the distance you've pulled the arrow back and set its release velocity to be
                // in the direction of the v
                var arrowForce = this.scaleArrowShotStrength(stringToNotchDistance);
                var handToNotchNorm = Vec3.normalize(handToNotch);

                var releaseVelocity = Vec3.multiply(handToNotchNorm, arrowForce);

                //make the arrow physical, give it gravity, a lifetime, and set our velocity
                var arrowProperties = {
                    dynamic: true,
                    collisionless: false,
                    collidesWith: "static,dynamic,otherAvatar", // workaround: not with kinematic --> no collision with bow
                    velocity: releaseVelocity,
                    parentID: NULL_UUID,
                    gravity: ARROW_GRAVITY,
                    lifetime: arrowAge + ARROW_LIFETIME,
                };

                // add a particle effect to make the arrow easier to see as it flies
                var arrowParticleProperties = {
                    accelerationSpread: { x: 0, y: 0, z: 0 },
                    alpha: 1,
                    alphaFinish: 0,
                    alphaSpread: 0,
                    alphaStart: 0.3,
                    azimuthFinish: 3.1,
                    azimuthStart: -3.14159,
                    color: { red: 255, green: 255, blue: 255 },
                    colorFinish: { red: 255, green: 255, blue: 255 },
                    colorSpread: { red: 0, green: 0, blue: 0 },
                    colorStart: { red: 255, green: 255, blue: 255 },
                    emitAcceleration: { x: 0, y: 0, z: 0 },
                    emitDimensions: { x: 0, y: 0, z: 0 },
                    emitOrientation: { x: -0.7, y: 0.0, z: 0.0, w: 0.7 },
                    emitRate: 0.01,
                    emitSpeed: 0,
                    emitterShouldTrail: 0,
                    isEmitting: 1,
                    lifespan: ARROW_PARTICLE_LIFESPAN,
                    lifetime: ARROW_PARTICLE_LIFESPAN + 1,
                    maxParticles: 1000,
                    name: 'arrow-particles',
                    parentID: this.arrow,
                    particleRadius: 0.132,
                    polarFinish: 0,
                    polarStart: 0,
                    radiusFinish: 0.35,
                    radiusSpread: 0,
                    radiusStart: 0.132,
                    speedSpread: 0,
                    textures: Script.resolvePath('arrow-sparkle.png'),
                    type: 'ParticleEffect'
                };

                Entities.addEntity(arrowParticleProperties);

                // actually shoot the arrow
                Entities.editEntity(this.arrow, arrowProperties);

                // play the sound of a shooting arrow
                this.playShootArrowSound();

                Entities.addAction("travel-oriented", this.arrow, {
                    forward: { x: 0, y: 0, z: -1 },
                    angularTimeScale: 0.1,
                    tag: "arrow from hifi-bow",
                    ttl: ARROW_LIFETIME
                });


            }
        },

        scaleArrowShotStrength: function(value) {
            var percentage = (value - MIN_ARROW_DISTANCE_FROM_BOW_REST)
                / (MAX_ARROW_DISTANCE_FROM_BOW_REST - MIN_ARROW_DISTANCE_FROM_BOW_REST);
            return MIN_ARROW_SPEED + (percentage * (MAX_ARROW_SPEED - MIN_ARROW_SPEED)) ;
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
