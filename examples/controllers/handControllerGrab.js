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

var GRAB_RADIUS = 0.03; // if the ray misses but an object is this close, it will still be selected
var NEAR_GRABBING_ACTION_TIMEFRAME = 0.05; // how quickly objects move to their new position
var NEAR_GRABBING_VELOCITY_SMOOTH_RATIO = 1.0; // adjust time-averaging of held object's velocity.  1.0 to disable.
var NEAR_PICK_MAX_DISTANCE = 0.3; // max length of pick-ray for close grabbing to be selected
var RELEASE_VELOCITY_MULTIPLIER = 1.5; // affects throwing things
var PICK_BACKOFF_DISTANCE = 0.2; // helps when hand is intersecting the grabble object
var NEAR_GRABBING_KINEMATIC = true; // force objects to be kinematic when near-grabbed

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
    "collisionsWillMove",
    "locked",
    "name",
    "shapeType",
    "parentID",
    "parentJointIndex"
];

var GRABBABLE_DATA_KEY = "grabbableKey"; // shared with grab.js
var GRAB_USER_DATA_KEY = "grabKey"; // shared with grab.js

var DEFAULT_GRABBABLE_DATA = {
    grabbable: true,
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
var STATE_EQUIP_SPRING = 16;

// "collidesWith" is specified by comma-separated list of group names
// the possible group names are:  static, dynamic, kinematic, myAvatar, otherAvatar
var COLLIDES_WITH_WHILE_GRABBED = "dynamic,otherAvatar";
var COLLIDES_WITH_WHILE_MULTI_GRABBED = "dynamic";

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
        case STATE_EQUIP_SPRING:
            return "state_equip_spring";
    }

    return "unknown";
}

function getTag() {
    return "grab-" + MyAvatar.sessionUUID;
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

function getSpatialOffsetPosition(hand, spatialKey) {
    var position = Vec3.ZERO;

    if (hand !== RIGHT_HAND && spatialKey.leftRelativePosition) {
        position = spatialKey.leftRelativePosition;
    }
    if (hand === RIGHT_HAND && spatialKey.rightRelativePosition) {
        position = spatialKey.rightRelativePosition;
    }
    if (spatialKey.relativePosition) {
        position = spatialKey.relativePosition;
    }

    // add the relative hand center offset
    var handSizeRatio = calculateHandSizeRatio();
    position = Vec3.multiply(position, handSizeRatio);
    return position;
}

var yFlip = Quat.angleAxis(180, Vec3.UNIT_Y);

function getSpatialOffsetRotation(hand, spatialKey) {
    var rotation = Quat.IDENTITY;

    if (hand !== RIGHT_HAND && spatialKey.leftRelativeRotation) {
        rotation = spatialKey.leftRelativeRotation;
    }
    if (hand === RIGHT_HAND && spatialKey.rightRelativeRotation) {
        rotation = spatialKey.rightRelativeRotation;
    }
    if (spatialKey.relativeRotation) {
        rotation = spatialKey.relativeRotation;
    }

    // Flip left hand
    if (hand !== RIGHT_HAND) {
        rotation = Quat.multiply(yFlip, rotation);
    }

    return rotation;
}

function MyController(hand) {
    this.hand = hand;
    if (this.hand === RIGHT_HAND) {
        this.getHandPosition = MyAvatar.getRightPalmPosition;
        this.getHandRotation = MyAvatar.getRightPalmRotation;
    } else {
        this.getHandPosition = MyAvatar.getLeftPalmPosition;
        this.getHandRotation = MyAvatar.getLeftPalmRotation;
    }

    var SPATIAL_CONTROLLERS_PER_PALM = 2;
    var TIP_CONTROLLER_OFFSET = 1;

    this.actionID = null; // action this script created...
    this.grabbedEntity = null; // on this entity.
    this.state = STATE_OFF;
    this.pointer = null; // entity-id of line object
    this.triggerValue = 0; // rolling average of trigger value
    this.rawTriggerValue = 0;
    this.rawBumperValue = 0;
    //for visualizations
    this.overlayLine = null;
    this.particleBeam = null;

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
            case STATE_EQUIP_SPRING:
                this.pullTowardEquipPosition()
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

    this.setState = function(newState) {
        if (WANT_DEBUG || WANT_DEBUG_STATE) {
            print("STATE: " + stateToName(this.state) + " --> " + stateToName(newState) + ", hand: " + this.hand);
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
            collisionsWillMove: false,
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
                collisionsWillMove: false,
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


    this.handleDistantParticleBeam = function(handPosition, objectPosition, color) {

        var handToObject = Vec3.subtract(objectPosition, handPosition);
        var finalRotation = Quat.rotationBetween(Vec3.multiply(-1, Vec3.UP), handToObject);

        var distance = Vec3.distance(handPosition, objectPosition);
        var speed = 5;
        var spread = 0;

        var lifespan = distance / speed;

        if (this.particleBeam === null) {
            this.createParticleBeam(objectPosition, finalRotation, color, speed, spread, lifespan);
        } else {
            this.updateParticleBeam(objectPosition, finalRotation, color, speed, spread, lifespan);
        }
    };

    this.createParticleBeam = function(position, orientation, color, speed, spread, lifespan) {

        var particleBeamProperties = {
            type: "ParticleEffect",
            isEmitting: true,
            position: position,
            visible: false,
            lifetime: 60,
            "name": "Particle Beam",
            "color": color,
            "maxParticles": 2000,
            "lifespan": lifespan,
            "emitRate": 50,
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

        this.particleBeam = Entities.addEntity(particleBeamProperties);
    };

    this.updateParticleBeam = function(position, orientation, color, speed, spread, lifespan) {
        Entities.editEntity(this.particleBeam, {
            rotation: orientation,
            position: position,
            visible: true,
            color: color,
            emitSpeed: speed,
            speedSpread: spread,
            lifespan: lifespan
        })

    };

    this.renewParticleBeamLifetime = function() {
        var props = Entities.getEntityProperties(this.particleBeam, "age");
        Entities.editEntity(this.particleBeam, {
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
        if (this.particleBeam !== null) {
            Entities.deleteEntity(this.particleBeam);
            this.particleBeam = null;
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

        if (this.state == STATE_SEARCHING ? this.triggerSmoothedReleased() : this.bumperReleased()) {
            this.setState(STATE_RELEASE);
            return;
        }

        // the trigger is being pressed, so do a ray test to see what we are hitting
        var handPosition = this.getHandPosition();
        var controllerHandInput = (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        var currentHandRotation = Controller.getPoseValue(controllerHandInput).rotation;
        var handDeltaRotation = Quat.multiply(currentHandRotation, Quat.inverse(this.startingHandRotation));

        var distantPickRay = {
            origin: PICK_WITH_HAND_RAY ? handPosition : Camera.position,
            direction: PICK_WITH_HAND_RAY ? Quat.getUp(this.getHandRotation()) : Vec3.mix(Quat.getUp(this.getHandRotation()), Quat.getFront(Camera.orientation), HAND_HEAD_MIX_RATIO),
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

            Messages.sendMessage('Hifi-Light-Overlay-Ray-Check', JSON.stringify(pickRayBacked));

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
        var forbiddenTyes = ['Unknown', 'Light', 'ParticleEffect', 'PolyLine', 'Zone'];

        var minDistance = PICK_MAX_DISTANCE;
        var i, props, distance, grabbableData;
        this.grabbedEntity = null;
        for (i = 0; i < candidateEntities.length; i++) {
            var grabbableDataForCandidate =
                getEntityCustomData(GRABBABLE_DATA_KEY, candidateEntities[i], DEFAULT_GRABBABLE_DATA);
            var propsForCandidate = Entities.getEntityProperties(candidateEntities[i], GRABBABLE_PROPERTIES);
            var grabbable = (typeof grabbableDataForCandidate.grabbable === 'undefined' || grabbableDataForCandidate.grabbable);
            if (!grabbable && !grabbableDataForCandidate.wantsTrigger) {
                continue;
            }
            if (forbiddenTyes.indexOf(propsForCandidate.type) >= 0) {
                continue;
            }
            if (propsForCandidate.locked && !grabbableDataForCandidate.wantsTrigger) {
                continue;
            }
            if (forbiddenNames.indexOf(propsForCandidate.name) >= 0) {
                continue;
            }

            distance = Vec3.distance(propsForCandidate.position, handPosition);
            if (distance > PICK_MAX_DISTANCE) {
                // too far away, don't grab
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
            var near = (nearPickedCandidateEntities.indexOf(this.grabbedEntity) >= 0);
            var isPhysical = this.propsArePhysical(props);

            // near or far trigger
            if (grabbableData.wantsTrigger) {
                this.setState(near ? STATE_NEAR_TRIGGER : STATE_FAR_TRIGGER);
                return;
            }
            // near grab or equip with action
            if (isPhysical && near) {
                this.setState(this.state == STATE_SEARCHING ? STATE_NEAR_GRABBING : STATE_EQUIP);
                return;
            }
            // far grab or equip with action
            if (isPhysical && !near) {
                if (entityIsGrabbedByOther(intersection.entityID)) {
                    // don't distance grab something that is already grabbed.
                    return;
                }
                this.temporaryPositionOffset = null;
                if (typeof grabbableData.spatialKey === 'undefined') {
                    // We want to give a temporary position offset to this object so it is pulled close to hand
                    var intersectionPointToCenterDistance = Vec3.length(Vec3.subtract(intersection.intersection,
                        intersection.properties.position));
                    this.temporaryPositionOffset = Vec3.normalize(Vec3.subtract(intersection.properties.position, handPosition));
                    this.temporaryPositionOffset = Vec3.multiply(this.temporaryPositionOffset,
                        intersectionPointToCenterDistance *
                        FAR_TO_NEAR_GRAB_PADDING_FACTOR);
                }
                this.setState(this.state == STATE_SEARCHING ? STATE_DISTANCE_HOLDING : STATE_EQUIP);
                this.searchSphereOff();
                return;
            }

            // else this thing isn't physical.  grab it by reparenting it.
            this.setState(STATE_NEAR_GRABBING);
            return;
        }

        //search line visualizations
        if (USE_ENTITY_LINES_FOR_SEARCHING === true) {
            this.lineOn(distantPickRay.origin, Vec3.multiply(distantPickRay.direction, LINE_LENGTH), NO_INTERSECT_COLOR);
        }

        var SEARCH_SPHERE_SIZE = 0.011;
        var SEARCH_SPHERE_FOLLOW_RATE = 0.50;

        if (this.intersectionDistance > 0) {
            //  If we hit something with our pick ray, move the search sphere toward that distance 
            this.searchSphereDistance = this.searchSphereDistance * SEARCH_SPHERE_FOLLOW_RATE + this.intersectionDistance * (1.0 - SEARCH_SPHERE_FOLLOW_RATE);
        }

        var searchSphereLocation = Vec3.sum(distantPickRay.origin, Vec3.multiply(distantPickRay.direction, this.searchSphereDistance));
        this.searchSphereOn(searchSphereLocation, SEARCH_SPHERE_SIZE * this.searchSphereDistance, (this.triggerSmoothedGrab() || this.bumperSqueezed()) ? INTERSECT_COLOR : NO_INTERSECT_COLOR);
        if ((USE_OVERLAY_LINES_FOR_SEARCHING === true) && PICK_WITH_HAND_RAY) {
            this.overlayLineOn(handPosition, searchSphereLocation, (this.triggerSmoothedGrab() || this.bumperSqueezed()) ? INTERSECT_COLOR : NO_INTERSECT_COLOR);
        }
    };

    this.distanceHolding = function() {
        var handControllerPosition = (this.hand === RIGHT_HAND) ? MyAvatar.rightHandPosition : MyAvatar.leftHandPosition;
        var controllerHandInput = (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        var handRotation = Quat.multiply(MyAvatar.orientation, Controller.getPoseValue(controllerHandInput).rotation);
        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
        var now = Date.now();

        // add the action and initialize some variables
        this.currentObjectPosition = grabbedProperties.position;
        this.currentObjectRotation = grabbedProperties.rotation;
        this.currentObjectTime = now;
        this.handRelativePreviousPosition = Vec3.subtract(handControllerPosition, MyAvatar.position);
        this.handPreviousRotation = handRotation;
        this.currentCameraOrientation = Camera.orientation;

        // compute a constant based on the initial conditions which we use below to exagerate hand motion onto the held object
        this.radiusScalar = Math.log(Vec3.distance(this.currentObjectPosition, handControllerPosition) + 1.0);
        if (this.radiusScalar < 1.0) {
            this.radiusScalar = 1.0;
        }

        this.actionID = NULL_UUID;
        this.actionID = Entities.addAction("spring", this.grabbedEntity, {
            targetPosition: this.currentObjectPosition,
            linearTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME,
            targetRotation: this.currentObjectRotation,
            angularTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME,
            tag: getTag(),
            ttl: ACTION_TTL
        });
        if (this.actionID === NULL_UUID) {
            this.actionID = null;
        }
        this.actionTimeout = now + (ACTION_TTL * MSEC_PER_SEC);

        if (this.actionID !== null) {
            this.setState(STATE_CONTINUE_DISTANCE_HOLDING);
            this.activateEntity(this.grabbedEntity, grabbedProperties);
            if (this.hand === RIGHT_HAND) {
                Entities.callEntityMethod(this.grabbedEntity, "setRightHand");
            } else {
                Entities.callEntityMethod(this.grabbedEntity, "setLeftHand");
            }
            Entities.callEntityMethod(this.grabbedEntity, "setHand", [this.hand]);
            Entities.callEntityMethod(this.grabbedEntity, "startDistantGrab");
        }

        this.currentAvatarPosition = MyAvatar.position;
        this.currentAvatarOrientation = MyAvatar.orientation;

        this.turnOffVisualizations();
    };

    this.continueDistanceHolding = function() {
        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            Entities.callEntityMethod(this.grabbedEntity, "releaseGrab");
            return;
        }

        var handPosition = this.getHandPosition();
        var handControllerPosition = (this.hand === RIGHT_HAND) ? MyAvatar.rightHandPosition : MyAvatar.leftHandPosition;
        var controllerHandInput = (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        var handRotation = Quat.multiply(MyAvatar.orientation, Controller.getPoseValue(controllerHandInput).rotation);
        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
        var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, this.grabbedEntity, DEFAULT_GRABBABLE_DATA);

        if (this.state == STATE_CONTINUE_DISTANCE_HOLDING && this.bumperSqueezed() &&
            typeof grabbableData.spatialKey !== 'undefined') {
            var saveGrabbedID = this.grabbedEntity;
            this.release();
            this.setState(STATE_EQUIP);
            this.grabbedEntity = saveGrabbedID;
            return;
        }


        // the action was set up on a previous call.  update the targets.
        var radius = Vec3.distance(this.currentObjectPosition, handControllerPosition) *
            this.radiusScalar * DISTANCE_HOLDING_RADIUS_FACTOR;
        if (radius < 1.0) {
            radius = 1.0;
        }

        // how far did avatar move this timestep?
        var currentPosition = MyAvatar.position;
        var avatarDeltaPosition = Vec3.subtract(currentPosition, this.currentAvatarPosition);
        this.currentAvatarPosition = currentPosition;

        // How far did the avatar turn this timestep?
        // Note:  The following code is too long because we need a Quat.quatBetween() function
        // that returns the minimum quaternion between two quaternions.
        var currentOrientation = MyAvatar.orientation;
        if (Quat.dot(currentOrientation, this.currentAvatarOrientation) < 0.0) {
            var negativeCurrentOrientation = {
                x: -currentOrientation.x,
                y: -currentOrientation.y,
                z: -currentOrientation.z,
                w: -currentOrientation.w
            };
            var avatarDeltaOrientation = Quat.multiply(negativeCurrentOrientation, Quat.inverse(this.currentAvatarOrientation));
        } else {
            var avatarDeltaOrientation = Quat.multiply(currentOrientation, Quat.inverse(this.currentAvatarOrientation));
        }
        var handToAvatar = Vec3.subtract(handControllerPosition, this.currentAvatarPosition);
        var objectToAvatar = Vec3.subtract(this.currentObjectPosition, this.currentAvatarPosition);
        var handMovementFromTurning = Vec3.subtract(Quat.multiply(avatarDeltaOrientation, handToAvatar), handToAvatar);
        var objectMovementFromTurning = Vec3.subtract(Quat.multiply(avatarDeltaOrientation, objectToAvatar), objectToAvatar);
        this.currentAvatarOrientation = currentOrientation;

        // how far did hand move this timestep?
        var handMoved = Vec3.subtract(handToAvatar, this.handRelativePreviousPosition);
        this.handRelativePreviousPosition = handToAvatar;

        // magnify the hand movement but not the change from avatar movement & rotation
        handMoved = Vec3.subtract(handMoved, handMovementFromTurning);
        var superHandMoved = Vec3.multiply(handMoved, radius);

        // Move the object by the magnified amount and then by amount from avatar movement & rotation
        var newObjectPosition = Vec3.sum(this.currentObjectPosition, superHandMoved);
        newObjectPosition = Vec3.sum(newObjectPosition, avatarDeltaPosition);
        newObjectPosition = Vec3.sum(newObjectPosition, objectMovementFromTurning);

        var deltaPosition = Vec3.subtract(newObjectPosition, this.currentObjectPosition); // meters
        var now = Date.now();
        var deltaTime = (now - this.currentObjectTime) / MSEC_PER_SEC; // convert to seconds

        this.currentObjectPosition = newObjectPosition;
        this.currentObjectTime = now;

        // this doubles hand rotation
        var handChange = Quat.multiply(Quat.slerp(this.handPreviousRotation,
                handRotation,
                DISTANCE_HOLDING_ROTATION_EXAGGERATION_FACTOR),
            Quat.inverse(this.handPreviousRotation));
        this.handPreviousRotation = handRotation;
        this.currentObjectRotation = Quat.multiply(handChange, this.currentObjectRotation);

        Entities.callEntityMethod(this.grabbedEntity, "continueDistantGrab");

        var defaultMoveWithHeadData = {
            disableMoveWithHead: false
        };

        var handControllerData = getEntityCustomData('handControllerKey', this.grabbedEntity, defaultMoveWithHeadData);

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

        var success = Entities.updateAction(this.grabbedEntity, this.actionID, {
            targetPosition: targetPosition,
            linearTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME,
            targetRotation: this.currentObjectRotation,
            angularTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME,
            ttl: ACTION_TTL
        });
        if (success) {
            this.actionTimeout = now + (ACTION_TTL * MSEC_PER_SEC);
        } else {
            print("continueDistanceHolding -- updateAction failed");
        }
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

    this.nearGrabbing = function() {
        var now = Date.now();
        var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, this.grabbedEntity, DEFAULT_GRABBABLE_DATA);

        if (this.state == STATE_NEAR_GRABBING && this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            Entities.callEntityMethod(this.grabbedEntity, "releaseGrab");
            return;
        }

        this.lineOff();
        this.overlayLineOff();

        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
        this.activateEntity(this.grabbedEntity, grabbedProperties);
        if (grabbedProperties.collisionsWillMove && NEAR_GRABBING_KINEMATIC) {
            Entities.editEntity(this.grabbedEntity, {
                collisionsWillMove: false
            });
        }

        var handRotation = this.getHandRotation();
        var handPosition = this.getHandPosition();

        var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, this.grabbedEntity, DEFAULT_GRABBABLE_DATA);

        if (this.state != STATE_NEAR_GRABBING && grabbableData.spatialKey) {
            // if an object is "equipped" and has a spatialKey, use it.
            this.ignoreIK = grabbableData.spatialKey.ignoreIK ? grabbableData.spatialKey.ignoreIK : false;
            this.offsetPosition = getSpatialOffsetPosition(this.hand, grabbableData.spatialKey);
            this.offsetRotation = getSpatialOffsetRotation(this.hand, grabbableData.spatialKey);
        } else {
            this.ignoreIK = false;

            var objectRotation = grabbedProperties.rotation;
            this.offsetRotation = Quat.multiply(Quat.inverse(handRotation), objectRotation);

            var currentObjectPosition = grabbedProperties.position;
            var offset = Vec3.subtract(currentObjectPosition, handPosition);
            this.offsetPosition = Vec3.multiplyQbyV(Quat.inverse(Quat.multiply(handRotation, this.offsetRotation)), offset);
            if (this.temporaryPositionOffset && this.state != STATE_NEAR_GRABBING) {
                this.offsetPosition = this.temporaryPositionOffset;
            }
        }

        var isPhysical = this.propsArePhysical(grabbedProperties);
        if (isPhysical) {
            // grab entity via action
            if (!this.setupHoldAction()) {
                return;
            }
        } else {
            // grab entity via parenting
            var handJointIndex = MyAvatar.getJointIndex(this.hand === RIGHT_HAND ? "RightHand" : "LeftHand");
            Entities.editEntity(this.grabbedEntity, {
                parentID: MyAvatar.sessionUUID,
                parentJointIndex: handJointIndex
            });
        }

        if (this.state == STATE_NEAR_GRABBING) {
            this.setState(STATE_CONTINUE_NEAR_GRABBING);
        } else {
            // equipping
            Entities.callEntityMethod(this.grabbedEntity, "startEquip", [JSON.stringify(this.hand)]);
            this.setState(STATE_CONTINUE_EQUIP_BD);
        }

        if (this.hand === RIGHT_HAND) {
            Entities.callEntityMethod(this.grabbedEntity, "setRightHand");
        } else {
            Entities.callEntityMethod(this.grabbedEntity, "setLeftHand");
        }

        Entities.callEntityMethod(this.grabbedEntity, "setHand", [this.hand]);
        Entities.callEntityMethod(this.grabbedEntity, "startNearGrab");

        this.currentHandControllerTipPosition =
            (this.hand === RIGHT_HAND) ? MyAvatar.rightHandTipPosition : MyAvatar.leftHandTipPosition;

        this.currentObjectTime = Date.now();
    };

    this.continueNearGrabbing = function() {
        if (this.state == STATE_CONTINUE_NEAR_GRABBING && this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            Entities.callEntityMethod(this.grabbedEntity, "releaseGrab");
            return;
        }
        if (this.state == STATE_CONTINUE_EQUIP_BD && this.bumperReleased()) {
            this.setState(STATE_CONTINUE_EQUIP);
            return;
        }
        if (this.state == STATE_CONTINUE_EQUIP && this.bumperSqueezed()) {
            this.setState(STATE_WAITING_FOR_BUMPER_RELEASE);
            return;
        }
        if (this.state == STATE_CONTINUE_NEAR_GRABBING && this.bumperSqueezed()) {
            this.setState(STATE_CONTINUE_EQUIP_BD);
            Entities.callEntityMethod(this.grabbedEntity, "startEquip", [JSON.stringify(this.hand)]);
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
        Entities.callEntityMethod(this.grabbedEntity, "continueNearGrab");

        if (this.state === STATE_CONTINUE_EQUIP_BD) {
            Entities.callEntityMethod(this.grabbedEntity, "continueEquip");
        }

        //// jbp::: SEND UPDATE MESSAGE TO WEARABLES MANAGER
        Messages.sendMessage('Hifi-Wearables-Manager', JSON.stringify({
            action: 'update',
            grabbedEntity: this.grabbedEntity
        }))

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
            Entities.callEntityMethod(this.grabbedEntity, "releaseGrab");
            Entities.callEntityMethod(this.grabbedEntity, "unequip");
        }
    };

    this.pullTowardEquipPosition = function() {

        this.turnOffVisualizations();

        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
        var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, this.grabbedEntity, DEFAULT_GRABBABLE_DATA);

        // use a spring to pull the object to where it will be when equipped
        var relativeRotation = getSpatialOffsetRotation(this.hand, grabbableData.spatialKey);
        var relativePosition = getSpatialOffsetPosition(this.hand, grabbableData.spatialKey);
        var ignoreIK = grabbableData.spatialKey.ignoreIK ? grabbableData.spatialKey.ignoreIK : false;
        var handRotation = this.getHandRotation();
        var handPosition = this.getHandPosition();
        var targetRotation = Quat.multiply(handRotation, relativeRotation);
        var offset = Vec3.multiplyQbyV(targetRotation, relativePosition);
        var targetPosition = Vec3.sum(handPosition, offset);

        if (typeof this.equipSpringID === 'undefined' ||
            this.equipSpringID === null ||
            this.equipSpringID === NULL_UUID) {
            this.equipSpringID = Entities.addAction("spring", this.grabbedEntity, {
                targetPosition: targetPosition,
                linearTimeScale: EQUIP_SPRING_TIMEFRAME,
                targetRotation: targetRotation,
                angularTimeScale: EQUIP_SPRING_TIMEFRAME,
                ttl: ACTION_TTL,
                ignoreIK: ignoreIK
            });
            if (this.equipSpringID === NULL_UUID) {
                this.equipSpringID = null;
                this.setState(STATE_OFF);
                return;
            }
        } else {
            var success = Entities.updateAction(this.grabbedEntity, this.equipSpringID, {
                targetPosition: targetPosition,
                linearTimeScale: EQUIP_SPRING_TIMEFRAME,
                targetRotation: targetRotation,
                angularTimeScale: EQUIP_SPRING_TIMEFRAME,
                ttl: ACTION_TTL,
                ignoreIK: ignoreIK
            });
            if (!success) {
                print("pullTowardEquipPosition -- updateActionfailed");
            }
        }

        if (Vec3.distance(grabbedProperties.position, targetPosition) < EQUIP_SPRING_SHUTOFF_DISTANCE) {
            Entities.deleteAction(this.grabbedEntity, this.equipSpringID);
            this.equipSpringID = null;
            this.setState(STATE_EQUIP);
        }
    };

    this.nearTrigger = function() {
        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            Entities.callEntityMethod(this.grabbedEntity, "stopNearTrigger");
            return;
        }
        if (this.hand === RIGHT_HAND) {
            Entities.callEntityMethod(this.grabbedEntity, "setRightHand");
        } else {
            Entities.callEntityMethod(this.grabbedEntity, "setLeftHand");
        }

        Entities.callEntityMethod(this.grabbedEntity, "setHand", [this.hand]);

        Entities.callEntityMethod(this.grabbedEntity, "startNearTrigger");
        this.setState(STATE_CONTINUE_NEAR_TRIGGER);
    };

    this.farTrigger = function() {
        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            Entities.callEntityMethod(this.grabbedEntity, "stopFarTrigger");
            return;
        }

        if (this.hand === RIGHT_HAND) {
            Entities.callEntityMethod(this.grabbedEntity, "setRightHand");
        } else {
            Entities.callEntityMethod(this.grabbedEntity, "setLeftHand");
        }
        Entities.callEntityMethod(this.grabbedEntity, "setHand", [this.hand]);
        Entities.callEntityMethod(this.grabbedEntity, "startFarTrigger");
        this.setState(STATE_CONTINUE_FAR_TRIGGER);
    };

    this.continueNearTrigger = function() {
        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            Entities.callEntityMethod(this.grabbedEntity, "stopNearTrigger");
            return;
        }

        Entities.callEntityMethod(this.grabbedEntity, "continueNearTrigger");
    };

    this.continueFarTrigger = function() {
        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            Entities.callEntityMethod(this.grabbedEntity, "stopFarTrigger");
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
            this.lastPickTime = now;
            if (intersection.entityID != this.grabbedEntity) {
                this.setState(STATE_RELEASE);
                Entities.callEntityMethod(this.grabbedEntity, "stopFarTrigger");
                return;
            }
        }

        if (USE_ENTITY_LINES_FOR_MOVING === true) {
            this.lineOn(pickRay.origin, Vec3.multiply(pickRay.direction, LINE_LENGTH), NO_INTERSECT_COLOR);
        }

        Entities.callEntityMethod(this.grabbedEntity, "continueFarTrigger");
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
            if (props.name === 'pointer') {
                return;
            } else {
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

                } else {
                    //we are in another state
                    return;
                }
            }

        });

    };

    this.startTouch = function(entityID) {
        Entities.callEntityMethod(entityID, "startTouch");
    };

    this.continueTouch = function(entityID) {
        Entities.callEntityMethod(entityID, "continueTouch");
    };

    this.stopTouch = function(entityID) {
        Entities.callEntityMethod(entityID, "stopTouch");
    };

    this.release = function() {

        this.turnLightsOff();
        this.turnOffVisualizations();

        if (this.grabbedEntity !== null) {
            if (this.actionID !== null) {
                //sometimes we want things to stay right where they are when we let go.
                var releaseVelocityData = getEntityCustomData(GRABBABLE_DATA_KEY,
                    this.grabbedEntity,
                    DEFAULT_GRABBABLE_DATA);
                if (releaseVelocityData.disableReleaseVelocity === true) {
                    Entities.deleteAction(this.grabbedEntity, this.actionID);

                    Entities.editEntity(this.grabbedEntity, {
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
                    Entities.deleteAction(this.grabbedEntity, this.actionID);

                } else {
                    //don't make adjustments
                    Entities.deleteAction(this.grabbedEntity, this.actionID);
                }
            }
        }

        this.deactivateEntity(this.grabbedEntity);

        this.actionID = null;
        this.setState(STATE_OFF);

        //// jbp::: SEND RELEASE MESSAGE TO WEARABLES MANAGER

        Messages.sendMessage('Hifi-Wearables-Manager', JSON.stringify({
            action: 'checkIfWearable',
            grabbedEntity: this.grabbedEntity
        }))

        this.grabbedEntity = null;
    };

    this.cleanup = function() {
        this.release();
        Entities.deleteEntity(this.particleBeam);
        Entities.deleteEntity(this.spotLight);
        Entities.deleteEntity(this.pointLight);
    };

    this.activateEntity = function(entityID, grabbedProperties) {
        var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, entityID, DEFAULT_GRABBABLE_DATA);
        var data = getEntityCustomData(GRAB_USER_DATA_KEY, entityID, {});
        data["activated"] = true;
        data["avatarId"] = MyAvatar.sessionUUID;
        data["refCount"] = data["refCount"] ? data["refCount"] + 1 : 1;
        // zero gravity and set ignoreForCollisions in a way that lets us put them back, after all grabs are done
        if (data["refCount"] == 1) {
            data["gravity"] = grabbedProperties.gravity;
            data["collidesWith"] = grabbedProperties.collidesWith;
            data["collisionsWillMove"] = grabbedProperties.collisionsWillMove;
            data["parentID"] = grabbedProperties.parentID;
            data["parentJointIndex"] = grabbedProperties.parentJointIndex;
            var whileHeldProperties = {
                gravity: {
                    x: 0,
                    y: 0,
                    z: 0
                },
                // bummer, it isn't easy to do bitwise collisionMask operations like this:
                //"collisionMask": COLLISION_MASK_WHILE_GRABBED | grabbedProperties.collisionMask
                // when using string values
                "collidesWith": COLLIDES_WITH_WHILE_GRABBED
            };
            Entities.editEntity(entityID, whileHeldProperties);
        } else if (data["refCount"] > 1) {
            // if an object is being grabbed by more than one person (or the same person twice, but nevermind), switch
            // the collision groups so that it wont collide with "other" avatars.  This avoids a situation where two
            // people are holding something and one of them will be able (if the other releases at the right time) to
            // bootstrap themselves with the held object.  This happens because the meaning of "otherAvatar" in
            // the collision mask hinges on who the physics simulation owner is.
            Entities.editEntity(entityID, {"collidesWith": COLLIDES_WITH_WHILE_MULTI_GRABBED});
        }

        setEntityCustomData(GRAB_USER_DATA_KEY, entityID, data);
        return data;
    };

    this.deactivateEntity = function(entityID) {
        var data = getEntityCustomData(GRAB_USER_DATA_KEY, entityID, {});
        if (data && data["refCount"]) {
            data["refCount"] = data["refCount"] - 1;
            if (data["refCount"] < 1) {
                Entities.editEntity(entityID, {
                    gravity: data["gravity"],
                    collidesWith: data["collidesWith"],
                    collisionsWillMove: data["collisionsWillMove"],
                    ignoreForCollisions: data["ignoreForCollisions"],
                    parentID: data["parentID"],
                    parentJointIndex: data["parentJointIndex"]
                });
                data = null;
            }
        } else {
            data = null;
        }
        setEntityCustomData(GRAB_USER_DATA_KEY, entityID, data);
    };
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
        }
    }
}

Messages.messageReceived.connect(handleHandMessages);

function cleanup() {
    rightController.cleanup();
    leftController.cleanup();
    Controller.disableMapping(MAPPING_NAME);

}
Script.scriptEnding.connect(cleanup);
Script.update.connect(update);
