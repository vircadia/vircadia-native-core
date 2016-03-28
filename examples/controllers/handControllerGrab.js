"use strict";
//  handControllerGrab.js
//
//  Created by Eric Levin on  9/2/15
//  Additions by James B. Pollack @imgntn on 9/24/2015
//  Additions By Seth Alves on 10/20/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Grabs physically moveable entities with hydra-like controllers; it works for either near or far objects.
//  Also supports touch and equipping objects.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt, pointInExtents, vec3equal, setEntityCustomData, getEntityCustomData */

Script.include("../libraries/utils.js");


//
// add lines where the hand ray picking is happening
//
var WANT_DEBUG = false;
var WANT_DEBUG_STATE = false;
var WANT_DEBUG_SEARCH_NAME = null;

//
// these tune time-averaging and "on" value for analog trigger
//

var TRIGGER_SMOOTH_RATIO = 0.1; //  Time averaging of trigger - 0.0 disables smoothing
var TRIGGER_ON_VALUE = 0.4; //  Squeezed just enough to activate search or near grab
var TRIGGER_GRAB_VALUE = 0.85; //  Squeezed far enough to complete distant grab
var TRIGGER_OFF_VALUE = 0.15;

var BUMPER_ON_VALUE = 0.5;

var HAND_HEAD_MIX_RATIO = 0.0; //  0 = only use hands for search/move.  1 = only use head for search/move.

var PICK_WITH_HAND_RAY = true;

//
// distant manipulation
//

var DISTANCE_HOLDING_RADIUS_FACTOR = 3.5; // multiplied by distance between hand and object
var DISTANCE_HOLDING_ACTION_TIMEFRAME = 0.1; // how quickly objects move to their new position
var DISTANCE_HOLDING_UNITY_MASS = 1200; //  The mass at which the distance holding action timeframe is unmodified
var DISTANCE_HOLDING_UNITY_DISTANCE = 6; //  The distance at which the distance holding action timeframe is unmodified
var DISTANCE_HOLDING_ROTATION_EXAGGERATION_FACTOR = 2.0; // object rotates this much more than hand did
var MOVE_WITH_HEAD = true; // experimental head-control of distantly held objects
var FAR_TO_NEAR_GRAB_PADDING_FACTOR = 1.2;

var NO_INTERSECT_COLOR = {
    red: 10,
    green: 10,
    blue: 255
}; // line color when pick misses
var INTERSECT_COLOR = {
    red: 250,
    green: 10,
    blue: 10
}; // line color when pick hits
var LINE_ENTITY_DIMENSIONS = {
    x: 1000,
    y: 1000,
    z: 1000
};

var LINE_LENGTH = 500;
var PICK_MAX_DISTANCE = 500; // max length of pick-ray
//
// near grabbing
//

var GRAB_RADIUS = 0.06; // if the ray misses but an object is this close, it will still be selected
var NEAR_GRABBING_ACTION_TIMEFRAME = 0.05; // how quickly objects move to their new position
var NEAR_PICK_MAX_DISTANCE = 0.3; // max length of pick-ray for close grabbing to be selected
var PICK_BACKOFF_DISTANCE = 0.2; // helps when hand is intersecting the grabble object
var NEAR_GRABBING_KINEMATIC = true; // force objects to be kinematic when near-grabbed
var SHOW_GRAB_SPHERE = false; // draw a green sphere to show the grab search position and size

//
// equip
//

var EQUIP_SPRING_SHUTOFF_DISTANCE = 0.05;
var EQUIP_SPRING_TIMEFRAME = 0.4; // how quickly objects move to their new position

//
// other constants
//

var RIGHT_HAND = 1;
var LEFT_HAND = 0;

var ZERO_VEC = {
    x: 0,
    y: 0,
    z: 0
};

var NULL_UUID = "{00000000-0000-0000-0000-000000000000}";
var MSEC_PER_SEC = 1000.0;

// these control how long an abandoned pointer line or action will hang around
var LIFETIME = 10;
var ACTION_TTL = 15; // seconds
var ACTION_TTL_REFRESH = 5;
var PICKS_PER_SECOND_PER_HAND = 60;
var MSECS_PER_SEC = 1000.0;
var GRABBABLE_PROPERTIES = [
    "position",
    "rotation",
    "gravity",
    "collidesWith",
    "dynamic",
    "collisionless",
    "locked",
    "name",
    "shapeType",
    "parentID",
    "parentJointIndex",
    "density",
    "dimensions"
];

var GRABBABLE_DATA_KEY = "grabbableKey"; // shared with grab.js
var GRAB_USER_DATA_KEY = "grabKey"; // shared with grab.js
var GRAB_CONSTRAINTS_USER_DATA_KEY = "grabConstraintsKey"

var DEFAULT_GRABBABLE_DATA = {
    disableReleaseVelocity: false
};



// sometimes we want to exclude objects from being picked
var USE_BLACKLIST = true;
var blacklist = [];

//we've created various ways of visualizing looking for and moving distant objects
var USE_ENTITY_LINES_FOR_SEARCHING = false;
var USE_OVERLAY_LINES_FOR_SEARCHING = true;

var USE_ENTITY_LINES_FOR_MOVING = false;
var USE_OVERLAY_LINES_FOR_MOVING = false;
var USE_PARTICLE_BEAM_FOR_MOVING = true;


var USE_SPOTLIGHT = false;
var USE_POINTLIGHT = false;

// states for the state machine
var STATE_OFF = 0;
var STATE_SEARCHING = 1;
var STATE_DISTANCE_HOLDING = 2;
var STATE_CONTINUE_DISTANCE_HOLDING = 3;
var STATE_NEAR_GRABBING = 4;
var STATE_CONTINUE_NEAR_GRABBING = 5;
var STATE_NEAR_TRIGGER = 6;
var STATE_CONTINUE_NEAR_TRIGGER = 7;
var STATE_FAR_TRIGGER = 8;
var STATE_CONTINUE_FAR_TRIGGER = 9;
var STATE_RELEASE = 10;
var STATE_EQUIP_SEARCHING = 11;
var STATE_EQUIP = 12
var STATE_CONTINUE_EQUIP_BD = 13; // equip while bumper is still held down
var STATE_CONTINUE_EQUIP = 14;
var STATE_WAITING_FOR_BUMPER_RELEASE = 15;

// "collidesWith" is specified by comma-separated list of group names
// the possible group names are:  static, dynamic, kinematic, myAvatar, otherAvatar
var COLLIDES_WITH_WHILE_GRABBED = "dynamic,otherAvatar";
var COLLIDES_WITH_WHILE_MULTI_GRABBED = "dynamic";

var HEART_BEAT_INTERVAL = 5 * MSECS_PER_SEC;
var HEART_BEAT_TIMEOUT = 15 * MSECS_PER_SEC;

function stateToName(state) {
    switch (state) {
        case STATE_OFF:
            return "off";
        case STATE_SEARCHING:
            return "searching";
        case STATE_DISTANCE_HOLDING:
            return "distance_holding";
        case STATE_CONTINUE_DISTANCE_HOLDING:
            return "continue_distance_holding";
        case STATE_NEAR_GRABBING:
            return "near_grabbing";
        case STATE_CONTINUE_NEAR_GRABBING:
            return "continue_near_grabbing";
        case STATE_NEAR_TRIGGER:
            return "near_trigger";
        case STATE_CONTINUE_NEAR_TRIGGER:
            return "continue_near_trigger";
        case STATE_FAR_TRIGGER:
            return "far_trigger";
        case STATE_CONTINUE_FAR_TRIGGER:
            return "continue_far_trigger";
        case STATE_RELEASE:
            return "release";
        case STATE_EQUIP_SEARCHING:
            return "equip_searching";
        case STATE_EQUIP:
            return "equip";
        case STATE_CONTINUE_EQUIP_BD:
            return "continue_equip_bd";
        case STATE_CONTINUE_EQUIP:
            return "continue_equip";
        case STATE_WAITING_FOR_BUMPER_RELEASE:
            return "waiting_for_bumper_release";
    }

    return "unknown";
}

function getTag() {
    return "grab-" + MyAvatar.sessionUUID;
}

function entityHasActions(entityID) {
    return Entities.getActionIDs(entityID).length > 0;
}

function entityIsGrabbedByOther(entityID) {
    // by convention, a distance grab sets the tag of its action to be grab-*owner-session-id*.
    var actionIDs = Entities.getActionIDs(entityID);
    for (var actionIndex = 0; actionIndex < actionIDs.length; actionIndex++) {
        var actionID = actionIDs[actionIndex];
        var actionArguments = Entities.getActionArguments(entityID, actionID);
        var tag = actionArguments["tag"];
        if (tag == getTag()) {
            // we see a grab-*uuid* shaped tag, but it's our tag, so that's okay.
            continue;
        }
        if (tag.slice(0, 5) == "grab-") {
            // we see a grab-*uuid* shaped tag and it's not ours, so someone else is grabbing it.
            return true;
        }
    }
    return false;
}

function MyController(hand) {
    this.hand = hand;
    if (this.hand === RIGHT_HAND) {
        this.getHandPosition = MyAvatar.getRightPalmPosition;
        // this.getHandRotation = MyAvatar.getRightPalmRotation;
    } else {
        this.getHandPosition = MyAvatar.getLeftPalmPosition;
        // this.getHandRotation = MyAvatar.getLeftPalmRotation;
    }
    this.getHandRotation = function() {
        var controllerHandInput = (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        return Quat.multiply(MyAvatar.orientation, Controller.getPoseValue(controllerHandInput).rotation);
    }

    this.actionID = null; // action this script created...
    this.grabbedEntity = null; // on this entity.
    this.state = STATE_OFF;
    this.pointer = null; // entity-id of line object
    this.triggerValue = 0; // rolling average of trigger value
    this.rawTriggerValue = 0;
    this.rawBumperValue = 0;
    //for visualizations
    this.overlayLine = null;
    this.particleBeamObject = null;
    this.grabSphere = null;

    //for lights
    this.spotlight = null;
    this.pointlight = null;
    this.overlayLine = null;
    this.searchSphere = null;

    // how far from camera to search intersection?
    var DEFAULT_SEARCH_SPHERE_DISTANCE = 1000;
    this.intersectionDistance = 0.0;
    this.searchSphereDistance = DEFAULT_SEARCH_SPHERE_DISTANCE;


    this.ignoreIK = false;
    this.offsetPosition = Vec3.ZERO;
    this.offsetRotation = Quat.IDENTITY;

    var _this = this;

    this.update = function() {

        this.updateSmoothedTrigger();

        switch (this.state) {
            case STATE_OFF:
                this.off();
                this.touchTest();
                break;
            case STATE_SEARCHING:
                this.search();
                break;
            case STATE_EQUIP_SEARCHING:
                this.search();
                break;
            case STATE_DISTANCE_HOLDING:
                this.distanceHolding();
                break;
            case STATE_CONTINUE_DISTANCE_HOLDING:
                this.continueDistanceHolding();
                break;
            case STATE_NEAR_GRABBING:
            case STATE_EQUIP:
                this.nearGrabbing();
                break;
            case STATE_WAITING_FOR_BUMPER_RELEASE:
                this.waitingForBumperRelease();
                break;
            case STATE_CONTINUE_NEAR_GRABBING:
            case STATE_CONTINUE_EQUIP_BD:
            case STATE_CONTINUE_EQUIP:
                this.continueNearGrabbing();
                break;
            case STATE_NEAR_TRIGGER:
                this.nearTrigger();
                break;
            case STATE_CONTINUE_NEAR_TRIGGER:
                this.continueNearTrigger();
                break;
            case STATE_FAR_TRIGGER:
                this.farTrigger();
                break;
            case STATE_CONTINUE_FAR_TRIGGER:
                this.continueFarTrigger();
                break;
            case STATE_RELEASE:
                this.release();
                break;
        }
    };

    this.callEntityMethodOnGrabbed = function(entityMethodName) {
        var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
        Entities.callEntityMethod(this.grabbedEntity, entityMethodName, args);
    }

    this.setState = function(newState) {
        this.grabSphereOff();
        if (WANT_DEBUG || WANT_DEBUG_STATE) {
            print("STATE (" + this.hand + "): " + stateToName(this.state) + " --> " +
                stateToName(newState) + ", hand: " + this.hand);
        }
        this.state = newState;
    };

    this.debugLine = function(closePoint, farPoint, color) {
        Entities.addEntity({
            type: "Line",
            name: "Grab Debug Entity",
            dimensions: LINE_ENTITY_DIMENSIONS,
            visible: true,
            position: closePoint,
            linePoints: [ZERO_VEC, farPoint],
            color: color,
            lifetime: 0.1,
            dynamic: false,
            ignoreForCollisions: true,
            userData: JSON.stringify({
                grabbableKey: {
                    grabbable: false
                }
            })
        });
    };

    this.lineOn = function(closePoint, farPoint, color) {
        // draw a line
        if (this.pointer === null) {
            this.pointer = Entities.addEntity({
                type: "Line",
                name: "grab pointer",
                dimensions: LINE_ENTITY_DIMENSIONS,
                visible: true,
                position: closePoint,
                linePoints: [ZERO_VEC, farPoint],
                color: color,
                lifetime: LIFETIME,
                dynamic: false,
                ignoreForCollisions: true,
                userData: JSON.stringify({
                    grabbableKey: {
                        grabbable: false
                    }
                })
            });
        } else {
            var age = Entities.getEntityProperties(this.pointer, "age").age;
            this.pointer = Entities.editEntity(this.pointer, {
                position: closePoint,
                linePoints: [ZERO_VEC, farPoint],
                color: color,
                lifetime: age + LIFETIME
            });
        }
    };

    var SEARCH_SPHERE_ALPHA = 0.5;
    this.searchSphereOn = function(location, size, color) {
        if (this.searchSphere === null) {
            var sphereProperties = {
                position: location,
                size: size,
                color: color,
                alpha: SEARCH_SPHERE_ALPHA,
                solid: true,
                visible: true
            }
            this.searchSphere = Overlays.addOverlay("sphere", sphereProperties);
        } else {
            Overlays.editOverlay(this.searchSphere, {
                position: location,
                size: size,
                color: color,
                visible: true
            });
        }
    }

    this.grabSphereOn = function() {
        var color = {
            red: 0,
            green: 255,
            blue: 0
        };
        if (this.grabSphere === null) {
            var sphereProperties = {
                position: this.getHandPosition(),
                size: GRAB_RADIUS * 2,
                color: color,
                alpha: 0.1,
                solid: true,
                visible: true
            }
            this.grabSphere = Overlays.addOverlay("sphere", sphereProperties);
        } else {
            Overlays.editOverlay(this.grabSphere, {
                position: this.getHandPosition(),
                size: GRAB_RADIUS * 2,
                color: color,
                alpha: 0.1,
                solid: true,
                visible: true
            });
        }
    }

    this.grabSphereOff = function() {
        if (this.grabSphere !== null) {
            Overlays.deleteOverlay(this.grabSphere);
            this.grabSphere = null;
        }
    };

    this.overlayLineOn = function(closePoint, farPoint, color) {
        if (this.overlayLine === null) {
            var lineProperties = {
                lineWidth: 5,
                start: closePoint,
                end: farPoint,
                color: color,
                ignoreRayIntersection: true, // always ignore this
                visible: true,
                alpha: 1
            };
            this.overlayLine = Overlays.addOverlay("line3d", lineProperties);

        } else {
            var success = Overlays.editOverlay(this.overlayLine, {
                lineWidth: 5,
                start: closePoint,
                end: farPoint,
                color: color,
                visible: true,
                ignoreRayIntersection: true, // always ignore this
                alpha: 1
            });
        }
    };

    this.searchIndicatorOn = function(handPosition, distantPickRay) {
        var SEARCH_SPHERE_SIZE = 0.011;
        var SEARCH_SPHERE_FOLLOW_RATE = 0.50;

        if (this.intersectionDistance > 0) {
            //  If we hit something with our pick ray, move the search sphere toward that distance
            this.searchSphereDistance = this.searchSphereDistance * SEARCH_SPHERE_FOLLOW_RATE +
                this.intersectionDistance * (1.0 - SEARCH_SPHERE_FOLLOW_RATE);
        }

        var searchSphereLocation = Vec3.sum(distantPickRay.origin,
            Vec3.multiply(distantPickRay.direction, this.searchSphereDistance));
        this.searchSphereOn(searchSphereLocation, SEARCH_SPHERE_SIZE * this.searchSphereDistance, (this.triggerSmoothedGrab() || this.bumperSqueezed()) ? INTERSECT_COLOR : NO_INTERSECT_COLOR);
        if ((USE_OVERLAY_LINES_FOR_SEARCHING === true) && PICK_WITH_HAND_RAY) {
            this.overlayLineOn(handPosition, searchSphereLocation, (this.triggerSmoothedGrab() || this.bumperSqueezed()) ? INTERSECT_COLOR : NO_INTERSECT_COLOR);
        }
    }

    this.handleDistantParticleBeam = function(handPosition, objectPosition, color) {

        var handToObject = Vec3.subtract(objectPosition, handPosition);
        var finalRotationObject = Quat.rotationBetween(Vec3.multiply(-1, Vec3.UP), handToObject);
        var distance = Vec3.distance(handPosition, objectPosition);
        var speed = distance * 3;
        var spread = 0;
        var lifespan = distance / speed;

        if (this.particleBeamObject === null) {
            this.createParticleBeam(objectPosition, finalRotationObject, color, speed, spread, lifespan);
        } else {
            this.updateParticleBeam(objectPosition, finalRotationObject, color, speed, spread, lifespan);
        }
    };

    this.createParticleBeam = function(positionObject, orientationObject, color, speed, spread, lifespan) {

        var particleBeamPropertiesObject = {
            type: "ParticleEffect",
            isEmitting: true,
            position: positionObject,
            visible: false,
            lifetime: 60,
            "name": "Particle Beam",
            "color": color,
            "maxParticles": 2000,
            "lifespan": lifespan,
            "emitRate": 1000,
            "emitSpeed": speed,
            "speedSpread": spread,
            "emitOrientation": {
                "x": -1,
                "y": 0,
                "z": 0,
                "w": 1
            },
            "emitDimensions": {
                "x": 0,
                "y": 0,
                "z": 0
            },
            "emitRadiusStart": 0.5,
            "polarStart": 0,
            "polarFinish": 0,
            "azimuthStart": -3.1415927410125732,
            "azimuthFinish": 3.1415927410125732,
            "emitAcceleration": {
                x: 0,
                y: 0,
                z: 0
            },
            "accelerationSpread": {
                "x": 0,
                "y": 0,
                "z": 0
            },
            "particleRadius": 0.015,
            "radiusSpread": 0.005,
            "alpha": 1,
            "alphaSpread": 0,
            "alphaStart": 1,
            "alphaFinish": 1,
            "additiveBlending": 0,
            "textures": "https://hifi-content.s3.amazonaws.com/alan/dev/textures/grabsprite-3.png"
        }

        this.particleBeamObject = Entities.addEntity(particleBeamPropertiesObject);
    };

    this.updateParticleBeam = function(positionObject, orientationObject, color, speed, spread, lifespan) {
        Entities.editEntity(this.particleBeamObject, {
            rotation: orientationObject,
            position: positionObject,
            visible: true,
            color: color,
            emitSpeed: speed,
            speedSpread: spread,
            lifespan: lifespan
        })
    };

    this.renewParticleBeamLifetime = function() {
        var props = Entities.getEntityProperties(this.particleBeamObject, "age");
        Entities.editEntity(this.particleBeamObject, {
            lifetime: TEMPORARY_PARTICLE_BEAM_LIFETIME + props.age // renew lifetime
        })
    }

    this.evalLightWorldTransform = function(modelPos, modelRot) {

        var MODEL_LIGHT_POSITION = {
            x: 0,
            y: -0.3,
            z: 0
        };

        var MODEL_LIGHT_ROTATION = Quat.angleAxis(-90, {
            x: 1,
            y: 0,
            z: 0
        });

        return {
            p: Vec3.sum(modelPos, Vec3.multiplyQbyV(modelRot, MODEL_LIGHT_POSITION)),
            q: Quat.multiply(modelRot, MODEL_LIGHT_ROTATION)
        };
    };

    this.handleSpotlight = function(parentID, position) {
        var LIFETIME = 100;

        var modelProperties = Entities.getEntityProperties(parentID, ['position', 'rotation']);

        var lightTransform = this.evalLightWorldTransform(modelProperties.position, modelProperties.rotation);
        var lightProperties = {
            type: "Light",
            isSpotlight: true,
            dimensions: {
                x: 2,
                y: 2,
                z: 20
            },
            parentID: parentID,
            color: {
                red: 255,
                green: 255,
                blue: 255
            },
            intensity: 2,
            exponent: 0.3,
            cutoff: 20,
            lifetime: LIFETIME,
            position: lightTransform.p,
        };

        if (this.spotlight === null) {
            this.spotlight = Entities.addEntity(lightProperties);
        } else {
            Entities.editEntity(this.spotlight, {
                //without this, this light would maintain rotation with its parent
                rotation: Quat.fromPitchYawRollDegrees(-90, 0, 0),
            })
        }
    };

    this.handlePointLight = function(parentID, position) {
        var LIFETIME = 100;

        var modelProperties = Entities.getEntityProperties(parentID, ['position', 'rotation']);
        var lightTransform = this.evalLightWorldTransform(modelProperties.position, modelProperties.rotation);

        var lightProperties = {
            type: "Light",
            isSpotlight: false,
            dimensions: {
                x: 2,
                y: 2,
                z: 20
            },
            parentID: parentID,
            color: {
                red: 255,
                green: 255,
                blue: 255
            },
            intensity: 2,
            exponent: 0.3,
            cutoff: 20,
            lifetime: LIFETIME,
            position: lightTransform.p,
        };

        if (this.pointlight === null) {
            this.pointlight = Entities.addEntity(lightProperties);
        } else {

        }
    };

    this.lineOff = function() {
        if (this.pointer !== null) {
            Entities.deleteEntity(this.pointer);
        }
        this.pointer = null;
    };

    this.overlayLineOff = function() {
        if (this.overlayLine !== null) {
            Overlays.deleteOverlay(this.overlayLine);
        }
        this.overlayLine = null;
    };

    this.searchSphereOff = function() {
        if (this.searchSphere !== null) {
            Overlays.deleteOverlay(this.searchSphere);
            this.searchSphere = null;
            this.searchSphereDistance = DEFAULT_SEARCH_SPHERE_DISTANCE;
            this.intersectionDistance = 0.0;
        }
    };

    this.particleBeamOff = function() {
        if (this.particleBeamObject !== null) {
            Entities.deleteEntity(this.particleBeamObject);
            this.particleBeamObject = null;
        }
    }

    this.turnLightsOff = function() {
        if (this.spotlight !== null) {
            Entities.deleteEntity(this.spotlight);
            this.spotlight = null;
        }

        if (this.pointlight !== null) {
            Entities.deleteEntity(this.pointlight);
            this.pointlight = null;
        }
    };

    this.propsArePhysical = function(props) {
        if (!props.dynamic && props.parentID != MyAvatar.sessionUUID) {
            // if we have parented something, don't do this check on dynamic.
            return false;
        }
        var isPhysical = (props.shapeType && props.shapeType != 'none');
        return isPhysical;
    }

    this.turnOffVisualizations = function() {
        if (USE_ENTITY_LINES_FOR_SEARCHING === true || USE_ENTITY_LINES_FOR_MOVING === true) {
            this.lineOff();
        }

        if (USE_OVERLAY_LINES_FOR_SEARCHING === true || USE_OVERLAY_LINES_FOR_MOVING === true) {
            this.overlayLineOff();
        }

        if (USE_PARTICLE_BEAM_FOR_MOVING === true) {
            this.particleBeamOff();
        }
        this.searchSphereOff();

        Reticle.setVisible(true);

    };

    this.triggerPress = function(value) {
        _this.rawTriggerValue = value;
    };

    this.bumperPress = function(value) {
        _this.rawBumperValue = value;
    };

    this.updateSmoothedTrigger = function() {
        var triggerValue = this.rawTriggerValue;
        // smooth out trigger value
        this.triggerValue = (this.triggerValue * TRIGGER_SMOOTH_RATIO) +
            (triggerValue * (1.0 - TRIGGER_SMOOTH_RATIO));
    };

    this.triggerSmoothedGrab = function() {
        return this.triggerValue > TRIGGER_GRAB_VALUE;
    };

    this.triggerSmoothedSqueezed = function() {
        return this.triggerValue > TRIGGER_ON_VALUE;
    };

    this.triggerSmoothedReleased = function() {
        return this.triggerValue < TRIGGER_OFF_VALUE;
    };

    this.bumperSqueezed = function() {
        return _this.rawBumperValue > BUMPER_ON_VALUE;
    };

    this.bumperReleased = function() {
        return _this.rawBumperValue < BUMPER_ON_VALUE;
    };

    this.off = function() {
        if (this.triggerSmoothedSqueezed() || this.bumperSqueezed()) {
            this.lastPickTime = 0;
            var controllerHandInput = (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
            this.startingHandRotation = Controller.getPoseValue(controllerHandInput).rotation;
            if (this.triggerSmoothedSqueezed()) {
                this.setState(STATE_SEARCHING);
            } else {
                this.setState(STATE_EQUIP_SEARCHING);
            }
        }
    };

    this.search = function() {
        this.grabbedEntity = null;
        this.isInitialGrab = false;
        this.doubleParentGrab = false;

        this.checkForStrayChildren();

        if (this.state == STATE_SEARCHING ? this.triggerSmoothedReleased() : this.bumperReleased()) {
            this.setState(STATE_RELEASE);
            return;
        }

        // the trigger is being pressed, so do a ray test to see what we are hitting
        var handPosition = this.getHandPosition();

        if (SHOW_GRAB_SPHERE) {
            this.grabSphereOn();
        }

        var controllerHandInput = (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        var currentHandRotation = Controller.getPoseValue(controllerHandInput).rotation;
        var handDeltaRotation = Quat.multiply(currentHandRotation, Quat.inverse(this.startingHandRotation));

        var distantPickRay = {
            origin: PICK_WITH_HAND_RAY ? handPosition : Camera.position,
            direction: PICK_WITH_HAND_RAY ? Quat.getUp(this.getHandRotation()) : Vec3.mix(Quat.getUp(this.getHandRotation()),
                Quat.getFront(Camera.orientation),
                HAND_HEAD_MIX_RATIO),
            length: PICK_MAX_DISTANCE
        };

        var searchVisualizationPickRay = {
            origin: handPosition,
            direction: Quat.getUp(this.getHandRotation()),
            length: PICK_MAX_DISTANCE
        };

        // Pick at some maximum rate, not always
        var pickRays = [];
        var now = Date.now();
        if (now - this.lastPickTime > MSECS_PER_SEC / PICKS_PER_SECOND_PER_HAND) {
            pickRays = [distantPickRay];
            this.lastPickTime = now;
        }

        rayPickedCandidateEntities = []; // the list of candidates to consider grabbing

        this.intersectionDistance = 0.0;
        for (var index = 0; index < pickRays.length; ++index) {
            var pickRay = pickRays[index];
            var directionNormalized = Vec3.normalize(pickRay.direction);
            var directionBacked = Vec3.multiply(directionNormalized, PICK_BACKOFF_DISTANCE);
            var pickRayBacked = {
                origin: Vec3.subtract(pickRay.origin, directionBacked),
                direction: pickRay.direction
            };

            var intersection;

            if (USE_BLACKLIST === true && blacklist.length !== 0) {
                intersection = Entities.findRayIntersection(pickRayBacked, true, [], blacklist);
            } else {
                intersection = Entities.findRayIntersection(pickRayBacked, true);
            }

            if (intersection.intersects) {
                rayPickedCandidateEntities.push(intersection.entityID);
                this.intersectionDistance = Vec3.distance(pickRay.origin, intersection.intersection);
            }
        }

        nearPickedCandidateEntities = Entities.findEntities(handPosition, GRAB_RADIUS);
        candidateEntities = rayPickedCandidateEntities.concat(nearPickedCandidateEntities);

        var forbiddenNames = ["Grab Debug Entity", "grab pointer"];
        var forbiddenTypes = ['Unknown', 'Light', 'ParticleEffect', 'PolyLine', 'Zone'];

        var minDistance = PICK_MAX_DISTANCE;
        var i, props, distance, grabbableData;
        this.grabbedEntity = null;
        for (i = 0; i < candidateEntities.length; i++) {
            var grabbableDataForCandidate =
                getEntityCustomData(GRABBABLE_DATA_KEY, candidateEntities[i], DEFAULT_GRABBABLE_DATA);
            var grabDataForCandidate = getEntityCustomData(GRAB_USER_DATA_KEY, candidateEntities[i], {});
            var propsForCandidate = Entities.getEntityProperties(candidateEntities[i], GRABBABLE_PROPERTIES);
            var near = (nearPickedCandidateEntities.indexOf(candidateEntities[i]) >= 0);

            var isPhysical = this.propsArePhysical(propsForCandidate);
            var grabbable;
            if (isPhysical) {
                // physical things default to grabbable
                grabbable = true;
            } else {
                // non-physical things default to non-grabbable unless they are already grabbed
                if ("refCount" in grabDataForCandidate && grabDataForCandidate.refCount > 0) {
                    grabbable = true;
                } else {
                    grabbable = false;
                }
            }

            if ("grabbable" in grabbableDataForCandidate) {
                // if userData indicates that this is grabbable or not, override the default.
                grabbable = grabbableDataForCandidate.grabbable;
            }

            if (!grabbable && !grabbableDataForCandidate.wantsTrigger) {
                if (WANT_DEBUG_SEARCH_NAME && propsForCandidate.name == WANT_DEBUG_SEARCH_NAME) {
                    print("grab is skipping '" + WANT_DEBUG_SEARCH_NAME + "': not grabbable.");
                }
                continue;
            }
            if (forbiddenTypes.indexOf(propsForCandidate.type) >= 0) {
                if (WANT_DEBUG_SEARCH_NAME && propsForCandidate.name == WANT_DEBUG_SEARCH_NAME) {
                    print("grab is skipping '" + WANT_DEBUG_SEARCH_NAME + "': forbidden entity type.");
                }
                continue;
            }
            if (propsForCandidate.locked && !grabbableDataForCandidate.wantsTrigger) {
                if (WANT_DEBUG_SEARCH_NAME && propsForCandidate.name == WANT_DEBUG_SEARCH_NAME) {
                    print("grab is skipping '" + WANT_DEBUG_SEARCH_NAME + "': locked and not triggerable.");
                }
                continue;
            }
            if (forbiddenNames.indexOf(propsForCandidate.name) >= 0) {
                if (WANT_DEBUG_SEARCH_NAME && propsForCandidate.name == WANT_DEBUG_SEARCH_NAME) {
                    print("grab is skipping '" + WANT_DEBUG_SEARCH_NAME + "': forbidden name.");
                }
                continue;
            }

            distance = Vec3.distance(propsForCandidate.position, handPosition);
            if (distance > PICK_MAX_DISTANCE) {
                // too far away, don't grab
                if (WANT_DEBUG_SEARCH_NAME && propsForCandidate.name == WANT_DEBUG_SEARCH_NAME) {
                    print("grab is skipping '" + WANT_DEBUG_SEARCH_NAME + "': too far away.");
                }
                continue;
            }
            if (propsForCandidate.parentID != NULL_UUID && this.state == STATE_EQUIP_SEARCHING) {
                // don't allow a double-equip
                if (WANT_DEBUG_SEARCH_NAME && propsForCandidate.name == WANT_DEBUG_SEARCH_NAME) {
                    print("grab is skipping '" + WANT_DEBUG_SEARCH_NAME + "': it's a child");
                }
                continue;
            }

            if (this.state == STATE_SEARCHING &&
                !isPhysical && distance > NEAR_PICK_MAX_DISTANCE && !near && !grabbableDataForCandidate.wantsTrigger) {
                // we can't distance-grab non-physical
                if (WANT_DEBUG_SEARCH_NAME && propsForCandidate.name == WANT_DEBUG_SEARCH_NAME) {
                    print("grab is skipping '" + WANT_DEBUG_SEARCH_NAME + "': not physical and too far for near-grab");
                }
                continue;
            }

            if (distance < minDistance) {
                this.grabbedEntity = candidateEntities[i];
                minDistance = distance;
                props = propsForCandidate;
                grabbableData = grabbableDataForCandidate;
            }
        }
        if ((this.grabbedEntity !== null) && (this.triggerSmoothedGrab() || this.bumperSqueezed())) {
            // We are squeezing enough to grab, and we've found an entity that we'll try to do something with.
            var near = (nearPickedCandidateEntities.indexOf(this.grabbedEntity) >= 0) || minDistance <= NEAR_PICK_MAX_DISTANCE;
            var isPhysical = this.propsArePhysical(props);

            // near or far trigger
            if (grabbableData.wantsTrigger) {
                this.setState(near ? STATE_NEAR_TRIGGER : STATE_FAR_TRIGGER);
                return;
            }
            // near grab or equip with action
            var grabData = getEntityCustomData(GRAB_USER_DATA_KEY, this.grabbedEntity, {});
            var refCount = ("refCount" in grabData) ? grabData.refCount : 0;
            if (near && (refCount < 1 || entityHasActions(this.grabbedEntity))) {
                this.setState(this.state == STATE_SEARCHING ? STATE_NEAR_GRABBING : STATE_EQUIP);
                return;
            }
            // far grab or equip with action
            if ((isPhysical || this.state == STATE_EQUIP_SEARCHING) && !near) {
                if (entityIsGrabbedByOther(this.grabbedEntity)) {
                    // don't distance grab something that is already grabbed.
                    if (WANT_DEBUG_SEARCH_NAME && props.name == WANT_DEBUG_SEARCH_NAME) {
                        print("grab is skipping '" + WANT_DEBUG_SEARCH_NAME + "': already grabbed by another.");
                    }
                    return;
                }
                this.temporaryPositionOffset = null;
                if (!this.hasPresetOffsets()) {
                    // We want to give a temporary position offset to this object so it is pulled close to hand
                    var intersectionPointToCenterDistance = Vec3.length(Vec3.subtract(intersection.intersection,
                        intersection.properties.position));
                    var handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
                    var handJointPosition = MyAvatar.getJointPosition(handJointIndex);
                    this.temporaryPositionOffset =
                        Vec3.normalize(Vec3.subtract(intersection.properties.position, handJointPosition));
                    this.temporaryPositionOffset = Vec3.multiply(this.temporaryPositionOffset,
                        intersectionPointToCenterDistance *
                        FAR_TO_NEAR_GRAB_PADDING_FACTOR);
                }


                this.setState(this.state == STATE_SEARCHING ? STATE_DISTANCE_HOLDING : STATE_EQUIP);
                this.searchSphereOff();
                return;
            }

            // else this thing isn't physical.  grab it by reparenting it (but not if we've already
            // grabbed it).
            if (refCount < 1) {
                this.setState(this.state == STATE_SEARCHING ? STATE_NEAR_GRABBING : STATE_EQUIP);
                return;
            } else {
                // it's not physical and it's already held via parenting.  go ahead and grab it, but
                // save off the current parent and joint.  this wont always be right if there are more than
                // two grabs and the order of release isn't opposite of the order of grabs.
                this.doubleParentGrab = true;
                this.previousParentID = props.parentID;
                this.previousParentJointIndex = props.parentJointIndex;
                this.setState(this.state == STATE_SEARCHING ? STATE_NEAR_GRABBING : STATE_EQUIP);
                return;
            }
            if (WANT_DEBUG_SEARCH_NAME && props.name == WANT_DEBUG_SEARCH_NAME) {
                print("grab is skipping '" + WANT_DEBUG_SEARCH_NAME + "': fell through.");
            }
        }

        //search line visualizations
        if (USE_ENTITY_LINES_FOR_SEARCHING === true) {
            this.lineOn(distantPickRay.origin, Vec3.multiply(distantPickRay.direction, LINE_LENGTH), NO_INTERSECT_COLOR);
        }

        this.searchIndicatorOn(handPosition, distantPickRay);
        Reticle.setVisible(false);

    };

    this.distanceGrabTimescale = function(mass, distance) {
        var timeScale = DISTANCE_HOLDING_ACTION_TIMEFRAME * mass /
            DISTANCE_HOLDING_UNITY_MASS * distance /
            DISTANCE_HOLDING_UNITY_DISTANCE;
        if (timeScale < DISTANCE_HOLDING_ACTION_TIMEFRAME) {
            timeScale = DISTANCE_HOLDING_ACTION_TIMEFRAME;
        }
        return timeScale;
    }

    this.getMass = function(dimensions, density) {
        return (dimensions.x * dimensions.y * dimensions.z) * density;
    }

    this.distanceHolding = function() {

        // controller pose is in avatar frame
        var avatarControllerPose = Controller.getPoseValue((this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand);

        // transform it into world frame
        var controllerPosition = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, avatarControllerPose.translation));
        var controllerRotation = Quat.multiply(MyAvatar.orientation, avatarControllerPose.rotation);

        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
        var now = Date.now();

        // add the action and initialize some variables
        this.currentObjectPosition = grabbedProperties.position;
        this.currentObjectRotation = grabbedProperties.rotation;
        this.currentObjectTime = now;
        this.currentCameraOrientation = Camera.orientation;

        // compute a constant based on the initial conditions which we use below to exagerate hand motion onto the held object
        this.radiusScalar = Math.log(Vec3.distance(this.currentObjectPosition, controllerPosition) + 1.0);
        if (this.radiusScalar < 1.0) {
            this.radiusScalar = 1.0;
        }

        // compute the mass for the purpose of energy and how quickly to move object
        this.mass = this.getMass(grabbedProperties.dimensions, grabbedProperties.density);
        var distanceToObject = Vec3.length(Vec3.subtract(MyAvatar.position, grabbedProperties.position));
        var timeScale = this.distanceGrabTimescale(this.mass, distanceToObject);

        this.actionID = NULL_UUID;
        this.actionID = Entities.addAction("spring", this.grabbedEntity, {
            targetPosition: this.currentObjectPosition,
            linearTimeScale: timeScale,
            targetRotation: this.currentObjectRotation,
            angularTimeScale: timeScale,
            tag: getTag(),
            ttl: ACTION_TTL
        });
        if (this.actionID === NULL_UUID) {
            this.actionID = null;
        }
        this.actionTimeout = now + (ACTION_TTL * MSEC_PER_SEC);

        if (this.actionID !== null) {
            this.setState(STATE_CONTINUE_DISTANCE_HOLDING);
            this.activateEntity(this.grabbedEntity, grabbedProperties, false);
            this.callEntityMethodOnGrabbed("startDistanceGrab");
        }

        this.turnOffVisualizations();

        this.previousControllerPosition = controllerPosition;
        this.previousControllerRotation = controllerRotation;
    };

    this.continueDistanceHolding = function() {
        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            this.callEntityMethodOnGrabbed("releaseGrab");
            return;
        }

        this.heartBeat(this.grabbedEntity);

        // controller pose is in avatar frame
        var avatarControllerPose = Controller.getPoseValue((this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand);

        // transform it into world frame
        var controllerPosition = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, avatarControllerPose.translation));
        var controllerRotation = Quat.multiply(MyAvatar.orientation, avatarControllerPose.rotation);

        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);

        if (this.state == STATE_CONTINUE_DISTANCE_HOLDING && this.bumperSqueezed() &&
            this.hasPresetOffsets()) {
            var saveGrabbedID = this.grabbedEntity;
            this.release();
            this.setState(STATE_EQUIP);
            this.grabbedEntity = saveGrabbedID;
            return;
        }

        var now = Date.now();
        this.currentObjectTime = now;

        // the action was set up on a previous call.  update the targets.
        var radius = Vec3.distance(this.currentObjectPosition, controllerPosition) *
            this.radiusScalar * DISTANCE_HOLDING_RADIUS_FACTOR;
        if (radius < 1.0) {
            radius = 1.0;
        }

        // scale delta controller hand movement by radius.
        var handMoved = Vec3.multiply(Vec3.subtract(controllerPosition, this.previousControllerPosition), radius);

        // double delta controller rotation
        var handChange = Quat.multiply(Quat.slerp(this.previousControllerRotation,
                controllerRotation,
                DISTANCE_HOLDING_ROTATION_EXAGGERATION_FACTOR),
            Quat.inverse(this.previousControllerRotation));

        // update the currentObject position and rotation.
        this.currentObjectPosition = Vec3.sum(this.currentObjectPosition, handMoved);
        this.currentObjectRotation = Quat.multiply(handChange, this.currentObjectRotation);

        this.callEntityMethodOnGrabbed("continueDistantGrab");

        var defaultMoveWithHeadData = {
            disableMoveWithHead: false
        };

        var handControllerData = getEntityCustomData('handControllerKey', this.grabbedEntity, defaultMoveWithHeadData);

        var objectToAvatar = Vec3.subtract(this.currentObjectPosition, MyAvatar.position);
        if (handControllerData.disableMoveWithHead !== true) {
            // mix in head motion
            if (MOVE_WITH_HEAD) {
                var objDistance = Vec3.length(objectToAvatar);
                var before = Vec3.multiplyQbyV(this.currentCameraOrientation, {
                    x: 0.0,
                    y: 0.0,
                    z: objDistance
                });
                var after = Vec3.multiplyQbyV(Camera.orientation, {
                    x: 0.0,
                    y: 0.0,
                    z: objDistance
                });
                var change = Vec3.multiply(Vec3.subtract(before, after), HAND_HEAD_MIX_RATIO);
                this.currentCameraOrientation = Camera.orientation;
                this.currentObjectPosition = Vec3.sum(this.currentObjectPosition, change);
            }
        }

        var defaultConstraintData = {
            axisStart: false,
            axisEnd: false,
        }

        var constraintData = getEntityCustomData('lightModifierKey', this.grabbedEntity, defaultConstraintData);
        var clampedVector;
        var targetPosition;
        if (constraintData.axisStart !== false) {
            clampedVector = this.projectVectorAlongAxis(this.currentObjectPosition, constraintData.axisStart, constraintData.axisEnd);
            targetPosition = clampedVector;
        } else {
            targetPosition = {
                x: this.currentObjectPosition.x,
                y: this.currentObjectPosition.y,
                z: this.currentObjectPosition.z
            }
        }

        var handPosition = this.getHandPosition();

        //visualizations
        if (USE_ENTITY_LINES_FOR_MOVING === true) {
            this.lineOn(handPosition, Vec3.subtract(grabbedProperties.position, handPosition), INTERSECT_COLOR);
        }
        if (USE_OVERLAY_LINES_FOR_MOVING === true) {
            this.overlayLineOn(handPosition, grabbedProperties.position, INTERSECT_COLOR);
        }
        if (USE_PARTICLE_BEAM_FOR_MOVING === true) {
            this.handleDistantParticleBeam(handPosition, grabbedProperties.position, INTERSECT_COLOR)
        }
        if (USE_POINTLIGHT === true) {
            this.handlePointLight(this.grabbedEntity);
        }
        if (USE_SPOTLIGHT === true) {
            this.handleSpotlight(this.grabbedEntity);
        }

        var distanceToObject = Vec3.length(Vec3.subtract(MyAvatar.position, this.currentObjectPosition));
        var success = Entities.updateAction(this.grabbedEntity, this.actionID, {
            targetPosition: targetPosition,
            linearTimeScale: this.distanceGrabTimescale(this.mass, distanceToObject),
            targetRotation: this.currentObjectRotation,
            angularTimeScale: this.distanceGrabTimescale(this.mass, distanceToObject),
            ttl: ACTION_TTL
        });
        if (success) {
            this.actionTimeout = now + (ACTION_TTL * MSEC_PER_SEC);
        } else {
            print("continueDistanceHolding -- updateAction failed");
        }

        this.previousControllerPosition = controllerPosition;
        this.previousControllerRotation = controllerRotation;
    };

    this.setupHoldAction = function() {
        this.actionID = Entities.addAction("hold", this.grabbedEntity, {
            hand: this.hand === RIGHT_HAND ? "right" : "left",
            timeScale: NEAR_GRABBING_ACTION_TIMEFRAME,
            relativePosition: this.offsetPosition,
            relativeRotation: this.offsetRotation,
            ttl: ACTION_TTL,
            kinematic: NEAR_GRABBING_KINEMATIC,
            kinematicSetVelocity: true,
            ignoreIK: this.ignoreIK
        });
        if (this.actionID === NULL_UUID) {
            this.actionID = null;
            return false;
        }
        var now = Date.now();
        this.actionTimeout = now + (ACTION_TTL * MSEC_PER_SEC);
        return true;
    };

    this.projectVectorAlongAxis = function(position, axisStart, axisEnd) {
        var aPrime = Vec3.subtract(position, axisStart);
        var bPrime = Vec3.subtract(axisEnd, axisStart);
        var bPrimeMagnitude = Vec3.length(bPrime);
        var dotProduct = Vec3.dot(aPrime, bPrime);
        var scalar = dotProduct / bPrimeMagnitude;
        if (scalar < 0) {
            scalar = 0;
        }
        if (scalar > 1) {
            scalar = 1;
        }
        var projection = Vec3.sum(axisStart, Vec3.multiply(scalar, Vec3.normalize(bPrime)));
        return projection
    };

    this.hasPresetOffsets = function() {
        var wearableData = getEntityCustomData('wearable', this.grabbedEntity, {
            joints: {}
        });
        if ("joints" in wearableData) {
            var allowedJoints = wearableData.joints;
            var handJointName = this.hand === RIGHT_HAND ? "RightHand" : "LeftHand";
            if (handJointName in allowedJoints) {
                return true;
            }
        }
        return false;
    }

    this.getPresetPosition = function() {
        var wearableData = getEntityCustomData('wearable', this.grabbedEntity, {
            joints: {}
        });
        var allowedJoints = wearableData.joints;
        var handJointName = this.hand === RIGHT_HAND ? "RightHand" : "LeftHand";
        if (handJointName in allowedJoints) {
            return allowedJoints[handJointName][0];
        }
    }

    this.getPresetRotation = function() {
        var wearableData = getEntityCustomData('wearable', this.grabbedEntity, {
            joints: {}
        });
        var allowedJoints = wearableData.joints;
        var handJointName = this.hand === RIGHT_HAND ? "RightHand" : "LeftHand";
        if (handJointName in allowedJoints) {
            return allowedJoints[handJointName][1];
        }
    }

    this.getGrabConstraints = function() {
        var defaultConstraints = {
            positionLocked: false,
            rotationLocked: false,
            positionMod: false,
            rotationMod: {
                pitch: false,
                yaw: false,
                roll: false
            }
        }
        var constraints = getEntityCustomData(GRAB_CONSTRAINTS_USER_DATA_KEY, this.grabbedEntity, defaultConstraints);
        return constraints;
    }

    this.nearGrabbing = function() {
        print('NEAR GRAB')
        var now = Date.now();

        if (this.state == STATE_NEAR_GRABBING && this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            this.callEntityMethodOnGrabbed("releaseGrab");
            return;
        }

        this.lineOff();
        this.overlayLineOff();

        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
        this.activateEntity(this.grabbedEntity, grabbedProperties, false);
        if (grabbedProperties.dynamic && NEAR_GRABBING_KINEMATIC) {
            Entities.editEntity(this.grabbedEntity, {
                dynamic: false
            });
        }

        // var handRotation = this.getHandRotation();
        var handRotation = (this.hand === RIGHT_HAND) ? MyAvatar.getRightPalmRotation() : MyAvatar.getLeftPalmRotation();
        var handPosition = this.getHandPosition();

        var hasPresetPosition = false;
        if (this.state != STATE_NEAR_GRABBING && this.hasPresetOffsets()) {
            var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, this.grabbedEntity, DEFAULT_GRABBABLE_DATA);
            // if an object is "equipped" and has a predefined offset, use it.
            this.ignoreIK = grabbableData.ignoreIK ? grabbableData.ignoreIK : false;
            this.offsetPosition = this.getPresetPosition();
            this.offsetRotation = this.getPresetRotation();
            hasPresetPosition = true;
        } else {
            this.ignoreIK = false;

            var objectRotation = grabbedProperties.rotation;
            this.offsetRotation = Quat.multiply(Quat.inverse(handRotation), objectRotation);

            var currentObjectPosition = grabbedProperties.position;
            var offset = Vec3.subtract(currentObjectPosition, handPosition);
            this.offsetPosition = Vec3.multiplyQbyV(Quat.inverse(Quat.multiply(handRotation, this.offsetRotation)), offset);
            if (this.temporaryPositionOffset && this.state != STATE_NEAR_GRABBING) {
                this.offsetPosition = this.temporaryPositionOffset;
                hasPresetPosition = true;
            }
        }

        var isPhysical = this.propsArePhysical(grabbedProperties) || entityHasActions(this.grabbedEntity);
        if (isPhysical && this.state == STATE_NEAR_GRABBING) {
            // grab entity via action
            if (!this.setupHoldAction()) {
                return;
            }
        } else {
            // grab entity via parenting
            this.actionID = null;
            var handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
            reparentProps = {
                parentID: MyAvatar.sessionUUID,
                parentJointIndex: handJointIndex
            }
            if (hasPresetPosition) {
                reparentProps["localPosition"] = this.offsetPosition;
                reparentProps["localRotation"] = this.offsetRotation;
            }
            Entities.editEntity(this.grabbedEntity, reparentProps);

            Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
                action: 'equip',
                grabbedEntity: this.grabbedEntity
            }));
        }

        this.callEntityMethodOnGrabbed(this.state == STATE_NEAR_GRABBING ? "startNearGrab" : "startEquip");

        if (this.state == STATE_NEAR_GRABBING) {
            // near grabbing
            this.setState(STATE_CONTINUE_NEAR_GRABBING);
        } else {
            // equipping
            this.setState(STATE_CONTINUE_EQUIP_BD);
        }

        this.currentHandControllerTipPosition =
            (this.hand === RIGHT_HAND) ? MyAvatar.rightHandTipPosition : MyAvatar.leftHandTipPosition;
        this.currentObjectTime = Date.now();
    };

    this.continueNearGrabbing = function() {
        print('CONTINUE NEAR GRAB')
        if (this.state == STATE_CONTINUE_NEAR_GRABBING && this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            this.callEntityMethodOnGrabbed("releaseGrab");
            return;
        }
        if (this.state == STATE_CONTINUE_EQUIP_BD && this.bumperReleased()) {
            this.setState(STATE_CONTINUE_EQUIP);
            return;
        }
        if (this.state == STATE_CONTINUE_EQUIP && this.bumperSqueezed()) {
            this.setState(STATE_WAITING_FOR_BUMPER_RELEASE);
            this.callEntityMethodOnGrabbed("releaseEquip");
            return;
        }
        if (this.state == STATE_CONTINUE_NEAR_GRABBING && this.bumperSqueezed()) {
            this.setState(STATE_CONTINUE_EQUIP_BD);
            this.callEntityMethodOnGrabbed("releaseGrab");
            this.callEntityMethodOnGrabbed("startEquip");
            return;
        }

        this.heartBeat(this.grabbedEntity);

        var props = Entities.getEntityProperties(this.grabbedEntity, ["localPosition", "parentID", "position"]);
        if (!props.position) {
            // server may have reset, taking our equipped entity with it.  move back to "off" stte
            this.setState(STATE_RELEASE);
            this.callEntityMethodOnGrabbed("releaseGrab");
            return;
        }

        if (props.parentID == MyAvatar.sessionUUID &&
            Vec3.length(props.localPosition) > NEAR_PICK_MAX_DISTANCE * 2.0) {
            // for whatever reason, the held/equipped entity has been pulled away.  ungrab or unequip.
            print("handControllerGrab -- autoreleasing held or equipped item because it is far from hand." +
                props.parentID + " " + vec3toStr(props.position));
            this.setState(STATE_RELEASE);
            this.callEntityMethodOnGrabbed(this.state == STATE_NEAR_GRABBING ? "releaseGrab" : "releaseEquip");
            return;
        }

        // Keep track of the fingertip velocity to impart when we release the object.
        // Note that the idea of using a constant 'tip' velocity regardless of the
        // object's actual held offset is an idea intended to make it easier to throw things:
        // Because we might catch something or transfer it between hands without a good idea
        // of it's actual offset, let's try imparting a velocity which is at a fixed radius
        // from the palm.

        var handControllerPosition = (this.hand === RIGHT_HAND) ? MyAvatar.rightHandPosition : MyAvatar.leftHandPosition;
        var now = Date.now();

        var deltaPosition = Vec3.subtract(handControllerPosition, this.currentHandControllerTipPosition); // meters
        var deltaTime = (now - this.currentObjectTime) / MSEC_PER_SEC; // convert to seconds

        this.currentHandControllerTipPosition = handControllerPosition;
        this.currentObjectTime = now;

        var grabData = getEntityCustomData(GRAB_USER_DATA_KEY, this.grabbedEntity, {});
        if (this.state === STATE_CONTINUE_EQUIP) {
            this.callEntityMethodOnGrabbed("continueEquip");
        }
        if (this.state == STATE_CONTINUE_NEAR_GRABBING) {
            this.callEntityMethodOnGrabbed("continueNearGrab");
        }


        var constraints = this.getGrabConstraints();
        if (constraints.positionLocked === true) {
            print('IT HAS ITS POSITION LOCKED!!')
        }
        if (constraints.rotationMod !== false) {
            print('IT HAS A ROTATION MOD!!!')
        }


        // so it never seems to hit this currently
        if (this.actionID && this.actionTimeout - now < ACTION_TTL_REFRESH * MSEC_PER_SEC) {

            // if less than a 5 seconds left, refresh the actions ttl
            var success = Entities.updateAction(this.grabbedEntity, this.actionID, {
                hand: this.hand === RIGHT_HAND ? "right" : "left",
                timeScale: NEAR_GRABBING_ACTION_TIMEFRAME,
                relativePosition: this.offsetPosition,
                relativeRotation: this.offsetRotation,
                ttl: ACTION_TTL,
                kinematic: NEAR_GRABBING_KINEMATIC,
                kinematicSetVelocity: true,
                ignoreIK: this.ignoreIK
            });
            if (success) {
                this.actionTimeout = now + (ACTION_TTL * MSEC_PER_SEC);
            } else {
                print("continueNearGrabbing -- updateAction failed");
                Entities.deleteAction(this.grabbedEntity, this.actionID);
                this.setupHoldAction();
            }
        }
    };

    this.waitingForBumperRelease = function() {
        if (this.bumperReleased()) {
            this.setState(STATE_RELEASE);
        }
    };

    this.nearTrigger = function() {
        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            this.callEntityMethodOnGrabbed("stopNearTrigger");
            return;
        }
        this.callEntityMethodOnGrabbed("startNearTrigger");
        this.setState(STATE_CONTINUE_NEAR_TRIGGER);
        print('START NEAR TRIGGER')


    };

    this.farTrigger = function() {
        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            this.callEntityMethodOnGrabbed("stopFarTrigger");
            return;
        }
        this.callEntityMethodOnGrabbed("startFarTrigger");
        this.setState(STATE_CONTINUE_FAR_TRIGGER);
    };

    this.continueNearTrigger = function() {
        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            this.callEntityMethodOnGrabbed("stopNearTrigger");
            return;
        }


        var constraints = this.getGrabConstraints();
        if (constraints.rotationMod !== false) {
            //implement the rotation modifier
            var grabbedProps = Entities.getEntityProperties(this.grabbedEntity);

            var handPosition = this.getHandPosition();

            var modTypes = [];

            if (constraints.rotationMod.pitch !== false) {
                modTypes.push('pitch')
            }
            if (constraints.rotationMod.yaw !== false) {
                modTypes.push('yaw')
            }
            if (constraints.rotationMod.roll !== false) {
                modTypes.push('roll')
            }


            finalRotation = {
                x: 0,
                y: 0,
                z: 0
            }
            modTypes.forEach(function(modType) {

                var value = handPosition[constraints.rotationMod[modType].startingAxis];
                var min1 = constraints.rotationMod[modType].startingPoint;

                var finalAngle = scale(value, min1, constraints.rotationMod[modType].distanceToMax, constraints.rotationMod[modType].min, constraints.rotationMod[modType].max)
                    // print('VARS: ')
                    // print('CONSTRAINTS:: ' + JSON.stringify(constraints))
                    // print('value: ' + value)
                    // print('min1:' + min1)
                    // print('max1:' + constraints.rotationMod[modType].distanceToMax)
                    // print('min2: ' + constraints.rotationMod[modType].min)
                    // print('max2: ' + constraints.rotationMod[modType].max)
                    // print('FINAL ANGLE::' + finalAngle)


                if (finalAngle < constraints.rotationMod[modType].min) {
                    finalAngle = constraints.rotationMod[modType].min;
                }

                if (finalAngle > constraints.rotationMod[modType].max) {
                    finalAngle = constraints.rotationMod[modType].max;
                }

                if (modType === 'pitch') {
                    finalRotation.x = finalAngle
                }
                if (modType === 'yaw') {
                    finalRotation.y = finalAngle
                }
                if (modType === 'roll') {
                    finalRotation.z = finalAngle
                }
            });

            Entities.callEntityMethod(this.grabbedEntity, constraints.callback, [JSON.stringify(finalRotation)]);

        }
        this.callEntityMethodOnGrabbed("continueNearTrigger");
    };

    this.continueFarTrigger = function() {
        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            this.callEntityMethodOnGrabbed("stopFarTrigger");
            return;
        }

        var handPosition = this.getHandPosition();
        var pickRay = {
            origin: handPosition,
            direction: Quat.getUp(this.getHandRotation())
        };

        var now = Date.now();
        if (now - this.lastPickTime > MSECS_PER_SEC / PICKS_PER_SECOND_PER_HAND) {
            var intersection = Entities.findRayIntersection(pickRay, true);
            if (intersection.accurate) {
                this.lastPickTime = now;
                if (intersection.entityID != this.grabbedEntity) {
                    this.setState(STATE_RELEASE);
                    this.callEntityMethodOnGrabbed("stopFarTrigger");
                    return;
                }
                if (intersection.intersects) {
                    this.intersectionDistance = Vec3.distance(pickRay.origin, intersection.intersection);
                }
                this.searchIndicatorOn(handPosition, pickRay);
            }
        }

        this.callEntityMethodOnGrabbed("continueFarTrigger");
    };

    _this.allTouchedIDs = {};

    this.touchTest = function() {
        var maxDistance = 0.05;
        var leftHandPosition = MyAvatar.getLeftPalmPosition();
        var rightHandPosition = MyAvatar.getRightPalmPosition();
        var leftEntities = Entities.findEntities(leftHandPosition, maxDistance);
        var rightEntities = Entities.findEntities(rightHandPosition, maxDistance);
        var ids = [];

        if (leftEntities.length !== 0) {
            leftEntities.forEach(function(entity) {
                ids.push(entity);
            });

        }

        if (rightEntities.length !== 0) {
            rightEntities.forEach(function(entity) {
                ids.push(entity);
            });
        }

        ids.forEach(function(id) {
            var props = Entities.getEntityProperties(id, ["boundingBox", "name"]);
            if (!props ||
                !props.boundingBox ||
                props.name === 'pointer') {
                return;
            }
            var entityMinPoint = props.boundingBox.brn;
            var entityMaxPoint = props.boundingBox.tfl;
            var leftIsTouching = pointInExtents(leftHandPosition, entityMinPoint, entityMaxPoint);
            var rightIsTouching = pointInExtents(rightHandPosition, entityMinPoint, entityMaxPoint);

            if ((leftIsTouching || rightIsTouching) && _this.allTouchedIDs[id] === undefined) {
                // we haven't been touched before, but either right or left is touching us now
                _this.allTouchedIDs[id] = true;
                _this.startTouch(id);
            } else if ((leftIsTouching || rightIsTouching) && _this.allTouchedIDs[id]) {
                // we have been touched before and are still being touched
                // continue touch
                _this.continueTouch(id);
            } else if (_this.allTouchedIDs[id]) {
                delete _this.allTouchedIDs[id];
                _this.stopTouch(id);
            }
        });
    };

    this.startTouch = function(entityID) {
        var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
        Entities.callEntityMethod(entityID, "startTouch", args);
    };

    this.continueTouch = function(entityID) {
        var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
        Entities.callEntityMethod(entityID, "continueTouch", args);
    };

    this.stopTouch = function(entityID) {
        var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
        Entities.callEntityMethod(entityID, "stopTouch", args);
    };

    this.release = function() {

        this.turnLightsOff();
        this.turnOffVisualizations();

        var noVelocity = false;
        if (this.grabbedEntity !== null) {
            if (this.actionID !== null) {
                Entities.deleteAction(this.grabbedEntity, this.actionID);
                // sometimes we want things to stay right where they are when we let go.
                var grabData = getEntityCustomData(GRAB_USER_DATA_KEY, this.grabbedEntity, {});
                var releaseVelocityData = getEntityCustomData(GRABBABLE_DATA_KEY, this.grabbedEntity, DEFAULT_GRABBABLE_DATA);
                print('RELEASE DATA::' + JSON.stringify(releaseVelocityData))
                if (releaseVelocityData.disableReleaseVelocity === true ||
                    // this next line allowed both:
                    // (1) far-grab, pull to self, near grab, then throw
                    // (2) equip something physical and adjust it with a other-hand grab without the thing drifting
                    grabData.refCount > 1) {
                    noVelocity = true;
                }
            }
        }

        this.deactivateEntity(this.grabbedEntity, noVelocity);
        this.actionID = null;
        this.setState(STATE_OFF);

        print('HAS VELOCITY AT RELEASE?? ' + noVelocity)
        Messages.sendMessage('Hifi-Object-Manipulation', JSON.stringify({
            action: 'release',
            grabbedEntity: this.grabbedEntity,
            joint: this.hand === RIGHT_HAND ? "RightHand" : "LeftHand"
        }));

        this.grabbedEntity = null;
    };

    this.cleanup = function() {
        this.release();
        Entities.deleteEntity(this.particleBeamObject);
        Entities.deleteEntity(this.spotLight);
        Entities.deleteEntity(this.pointLight);
    };

    this.heartBeat = function(entityID) {
        var now = Date.now();
        if (now - this.lastHeartBeat > HEART_BEAT_INTERVAL) {
            var data = getEntityCustomData(GRAB_USER_DATA_KEY, entityID, {});
            data["heartBeat"] = now;
            setEntityCustomData(GRAB_USER_DATA_KEY, entityID, data);
            this.lastHeartBeat = now;
        }
    };

    this.resetAbandonedGrab = function(entityID) {
        print("cleaning up abandoned grab on " + entityID);
        var data = getEntityCustomData(GRAB_USER_DATA_KEY, entityID, {});
        data["refCount"] = 1;
        setEntityCustomData(GRAB_USER_DATA_KEY, entityID, data);
        this.deactivateEntity(entityID, false);
    };

    this.activateEntity = function(entityID, grabbedProperties, wasLoaded) {
        var data = getEntityCustomData(GRAB_USER_DATA_KEY, entityID, {});
        var now = Date.now();

        if (wasLoaded) {
            data["refCount"] = 1;
        } else {
            data["refCount"] = data["refCount"] ? data["refCount"] + 1 : 1;

            // zero gravity and set ignoreForCollisions in a way that lets us put them back, after all grabs are done
            if (data["refCount"] == 1) {
                data["heartBeat"] = now;
                this.lastHeartBeat = now;

                this.isInitialGrab = true;
                data["gravity"] = grabbedProperties.gravity;
                data["collidesWith"] = grabbedProperties.collidesWith;
                data["collisionless"] = grabbedProperties.collisionless;
                data["dynamic"] = grabbedProperties.dynamic;
                data["parentID"] = wasLoaded ? NULL_UUID : grabbedProperties.parentID;
                data["parentJointIndex"] = grabbedProperties.parentJointIndex;

                var whileHeldProperties = {
                    gravity: {
                        x: 0,
                        y: 0,
                        z: 0
                    },
                    // bummer, it isn't easy to do bitwise collisionMask operations like this:
                    // "collisionMask": COLLISION_MASK_WHILE_GRABBED | grabbedProperties.collisionMask
                    // when using string values
                    "collidesWith": COLLIDES_WITH_WHILE_GRABBED
                };
                print('ACTIVATING ENTITY')
                Entities.editEntity(entityID, whileHeldProperties);
            } else if (data["refCount"] > 1) {
                if (data["heartBeat"] === undefined ||
                    now - data["heartBeat"] > HEART_BEAT_TIMEOUT) {
                    // this entity has userData suggesting it is grabbed, but nobody is updating the hearbeat.
                    // deactivate it before grabbing.
                    this.resetAbandonedGrab(entityID);
                    grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
                    return this.activateEntity(entityID, grabbedProperties, wasLoaded);
                }

                this.isInitialGrab = false;
                // if an object is being grabbed by more than one person (or the same person twice, but nevermind), switch
                // the collision groups so that it wont collide with "other" avatars.  This avoids a situation where two
                // people are holding something and one of them will be able (if the other releases at the right time) to
                // bootstrap themselves with the held object.  This happens because the meaning of "otherAvatar" in
                // the collision mask hinges on who the physics simulation owner is.
                Entities.editEntity(entityID, {
                    "collidesWith": COLLIDES_WITH_WHILE_MULTI_GRABBED
                });
            }
        }
        print('ACTIVATED ENTITY!!!')
        setEntityCustomData(GRAB_USER_DATA_KEY, entityID, data);
        return data;
    };

    this.checkForStrayChildren = function() {
        // sometimes things can get parented to a hand and this script is unaware.  Search for such entities and
        // unhook them.
        var handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
        var children = Entities.getChildrenIDsOfJoint(MyAvatar.sessionUUID, handJointIndex);
        children.forEach(function(childID) {
            print("disconnecting stray child of hand: (" + _this.hand + ") " + childID);
            Entities.editEntity(childID, {
                parentID: NULL_UUID
            });
        });
    }

    this.deactivateEntity = function(entityID, noVelocity) {
        var data = getEntityCustomData(GRAB_USER_DATA_KEY, entityID, {});
        if (data && data["refCount"]) {
            data["refCount"] = data["refCount"] - 1;
            if (data["refCount"] < 1) {
                var deactiveProps = {
                    gravity: data["gravity"],
                    collidesWith: data["collidesWith"],
                    collisionless: data["collisionless"],
                    dynamic: data["dynamic"],
                    parentID: data["parentID"],
                    parentJointIndex: data["parentJointIndex"]
                };

                // things that are held by parenting and dropped with no velocity will end up as "static" in bullet.  If
                // it looks like the dropped thing should fall, give it a little velocity.
                var parentID = Entities.getEntityProperties(entityID, ["parentID"]).parentID;
                var forceVelocity = false;
                if (!noVelocity &&
                    parentID == MyAvatar.sessionUUID &&
                    Vec3.length(data["gravity"]) > 0.0 &&
                    data["dynamic"] &&
                    data["parentID"] == NULL_UUID &&
                    !data["collisionless"]) {
                    deactiveProps["velocity"] = {
                        x: 0.0,
                        y: 0.1,
                        z: 0.0
                    };
                }
                if (noVelocity) {
                    deactiveProps["velocity"] = {
                        x: 0.0,
                        y: 0.0,
                        z: 0.0
                    };
                    deactiveProps["angularVelocity"] = {
                        x: 0.0,
                        y: 0.0,
                        z: 0.0
                    };
                }

                Entities.editEntity(entityID, deactiveProps);
                data = null;
            } else if (this.doubleParentGrab) {
                // we parent-grabbed this from another parent grab.  try to put it back where we found it.
                var deactiveProps = {
                    parentID: this.previousParentID,
                    parentJointIndex: this.previousParentJointIndex,
                    velocity: {
                        x: 0.0,
                        y: 0.0,
                        z: 0.0
                    },
                    angularVelocity: {
                        x: 0.0,
                        y: 0.0,
                        z: 0.0
                    }
                };
                Entities.editEntity(entityID, deactiveProps);
            } else if (noVelocity) {
                Entities.editEntity(entityID, {
                    velocity: {
                        x: 0.0,
                        y: 0.0,
                        z: 0.0
                    },
                    angularVelocity: {
                        x: 0.0,
                        y: 0.0,
                        z: 0.0
                    }
                });
            }
        } else {
            data = null;
        }
        setEntityCustomData(GRAB_USER_DATA_KEY, entityID, data);
    };

    this.checkNewlyLoaded = function(loadedEntityID) {
        if (this.state == STATE_OFF ||
            this.state == STATE_SEARCHING ||
            this.state == STATE_EQUIP_SEARCHING) {
            var loadedProps = Entities.getEntityProperties(loadedEntityID);
            if (loadedProps.parentID != MyAvatar.sessionUUID) {
                return;
            }
            var handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
            if (loadedProps.parentJointIndex != handJointIndex) {
                return;
            }
            print("--- handControllerGrab found loaded entity ---");
            // an entity has been loaded and it's where this script would have equipped something, so switch states.
            this.grabbedEntity = loadedEntityID;
            this.activateEntity(this.grabbedEntity, loadedProps, true);
            this.isInitialGrab = true;
            this.callEntityMethodOnGrabbed("startEquip");
            this.setState(STATE_CONTINUE_EQUIP);
        }
    }
};

var rightController = new MyController(RIGHT_HAND);
var leftController = new MyController(LEFT_HAND);

var MAPPING_NAME = "com.highfidelity.handControllerGrab";

var mapping = Controller.newMapping(MAPPING_NAME);
mapping.from([Controller.Standard.RT]).peek().to(rightController.triggerPress);
mapping.from([Controller.Standard.LT]).peek().to(leftController.triggerPress);

mapping.from([Controller.Standard.RB]).peek().to(rightController.bumperPress);
mapping.from([Controller.Standard.LB]).peek().to(leftController.bumperPress);

Controller.enableMapping(MAPPING_NAME);

//the section below allows the grab script to listen for messages that disable either one or both hands.  useful for two handed items
var handToDisable = 'none';

function update() {
    if (handToDisable !== LEFT_HAND && handToDisable !== 'both') {
        leftController.update();
    }
    if (handToDisable !== RIGHT_HAND && handToDisable !== 'both') {
        rightController.update();
    }
}

Messages.subscribe('Hifi-Hand-Disabler');
Messages.subscribe('Hifi-Hand-Grab');
Messages.subscribe('Hifi-Hand-RayPick-Blacklist');
Messages.subscribe('Hifi-Object-Manipulation');

handleHandMessages = function(channel, message, sender) {
    if (sender === MyAvatar.sessionUUID) {
        if (channel === 'Hifi-Hand-Disabler') {
            if (message === 'left') {
                handToDisable = LEFT_HAND;
            }
            if (message === 'right') {
                handToDisable = RIGHT_HAND;
            }
            if (message === 'both' || message === 'none') {
                handToDisable = message;
            }
        } else if (channel === 'Hifi-Hand-Grab') {
            try {
                var data = JSON.parse(message);
                var selectedController = (data.hand === 'left') ? leftController : rightController;
                selectedController.release();
                selectedController.setState(STATE_EQUIP);
                selectedController.grabbedEntity = data.entityID;

            } catch (e) {}

        } else if (channel === 'Hifi-Hand-RayPick-Blacklist') {
            try {
                var data = JSON.parse(message);
                var action = data.action;
                var id = data.id;
                var index = blacklist.indexOf(id);

                if (action === 'add' && index === -1) {
                    blacklist.push(id);
                }
                if (action === 'remove') {
                    if (index > -1) {
                        blacklist.splice(index, 1);
                    }
                }

            } catch (e) {}
        } else if (channel === 'Hifi-Object-Manipulation') {
            if (sender !== MyAvatar.sessionUUID) {
                return;
            }

            var parsedMessage = null;
            try {
                parsedMessage = JSON.parse(message);
            } catch (e) {
                print('error parsing Hifi-Object-Manipulation message');
                return;
            }

            if (parsedMessage.action === 'loaded') {
                rightController.checkNewlyLoaded(parsedMessage['grabbedEntity']);
                leftController.checkNewlyLoaded(parsedMessage['grabbedEntity']);
            }
        }
    }
}

Messages.messageReceived.connect(handleHandMessages);

function cleanup() {
    rightController.cleanup();
    leftController.cleanup();
    Controller.disableMapping(MAPPING_NAME);
    Reticle.setVisible(true);
}
Script.scriptEnding.connect(cleanup);
Script.update.connect(update);


function scale(value, min1, max1, min2, max2) {
    return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
}