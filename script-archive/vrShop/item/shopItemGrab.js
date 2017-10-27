//  shopItemGrab.js
//  
//  Simplified and coarse version of handControllerGrab.js with the addition of the ownerID concept.
//  This grab is the only one which should run in the vrShop. It allows only near grab and add the feature of checking the ownerID. (See shopGrapSwapperEntityScript.js)
//  

Script.include("../../libraries/utils.js");

var SHOP_GRAB_CHANNEL = "Hifi-vrShop-Grab";
Messages.sendMessage('Hifi-Hand-Disabler', "both");     //disable both the hands from handControlledGrab

//
// add lines where the hand ray picking is happening
//
var WANT_DEBUG = false;

//
// these tune time-averaging and "on" value for analog trigger
//

var TRIGGER_SMOOTH_RATIO = 0.1; // 0.0 disables smoothing of trigger value
var TRIGGER_ON_VALUE = 0.4;
var TRIGGER_OFF_VALUE = 0.15;
//
// distant manipulation
//

var DISTANCE_HOLDING_RADIUS_FACTOR = 5; // multiplied by distance between hand and object
var DISTANCE_HOLDING_ACTION_TIMEFRAME = 0.1; // how quickly objects move to their new position
var DISTANCE_HOLDING_ROTATION_EXAGGERATION_FACTOR = 2.0; // object rotates this much more than hand did

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

var EQUIP_TRACTOR_SHUTOFF_DISTANCE = 0.05;
var EQUIP_TRACTOR_TIMEFRAME = 0.4; // how quickly objects move to their new position

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

var NULL_ACTION_ID = "{00000000-0000-0000-000000000000}";
var MSEC_PER_SEC = 1000.0;

// these control how long an abandoned pointer line or action will hang around
var LIFETIME = 10;
var ACTION_TTL = 15; // seconds
var ACTION_TTL_REFRESH = 5;
var PICKS_PER_SECOND_PER_HAND = 5;
var MSECS_PER_SEC = 1000.0;
var GRABBABLE_PROPERTIES = [
    "position",
    "rotation",
    "gravity",
    "ignoreForCollisions",
    "collisionsWillMove",
    "locked",
    "name"
];

var GRABBABLE_DATA_KEY = "grabbableKey"; // shared with grab.js
var GRAB_USER_DATA_KEY = "grabKey"; // shared with grab.js

var DEFAULT_GRABBABLE_DATA = {
    grabbable: true,
    invertSolidWhileHeld: false
};


// states for the state machine
var STATE_OFF = 0;
var STATE_SEARCHING = 1;
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
var STATE_EQUIP_TRACTOR = 16;


function stateToName(state) {
    switch (state) {
        case STATE_OFF:
            return "off";
        case STATE_SEARCHING:
            return "searching";
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
        case STATE_EQUIP_TRACTOR:
            return "state_equip_tractor";
    }

    return "unknown";
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
    this.palm = SPATIAL_CONTROLLERS_PER_PALM * hand;
    this.tip = SPATIAL_CONTROLLERS_PER_PALM * hand + TIP_CONTROLLER_OFFSET;

    this.actionID = null; // action this script created...
    this.grabbedEntity = null; // on this entity.
    this.state = STATE_OFF;
    this.pointer = null; // entity-id of line object
    this.triggerValue = 0; // rolling average of trigger value
    this.rawTriggerValue = 0;

    this.offsetPosition = {
        x: 0.0,
        y: 0.0,
        z: 0.0
    };
    this.offsetRotation = {
        x: 0.0,
        y: 0.0,
        z: 0.0,
        w: 1.0
    };

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
            case STATE_NEAR_GRABBING:
            case STATE_EQUIP:
                this.nearGrabbing();
                break;
            case STATE_EQUIP_TRACTOR:
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
        if (WANT_DEBUG) {
            print("STATE: " + stateToName(this.state) + " --> " + stateToName(newState) + ", hand: " + this.hand);
        }
        this.state = newState;
    }

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
    }

    this.lineOn = function(closePoint, farPoint, color) {
        // draw a line
        if (this.pointer === null) {
            this.pointer = Entities.addEntity({
                type: "Line",
                name: "grab pointer",
                dimensions: LINE_ENTITY_DIMENSIONS,
                visible: false,
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

    this.lineOff = function() {
        if (this.pointer !== null) {
            Entities.deleteEntity(this.pointer);
        }
        this.pointer = null;
    };

    this.triggerPress = function(value) {
        _this.rawTriggerValue = value;
    };


    this.updateSmoothedTrigger = function() {
        var triggerValue = this.rawTriggerValue;
        // smooth out trigger value
        this.triggerValue = (this.triggerValue * TRIGGER_SMOOTH_RATIO) +
            (triggerValue * (1.0 - TRIGGER_SMOOTH_RATIO));
    };

    this.triggerSmoothedSqueezed = function() {
        return this.triggerValue > TRIGGER_ON_VALUE;
    };

    this.triggerSmoothedReleased = function() {
        return this.triggerValue < TRIGGER_OFF_VALUE;
    };

    this.triggerSqueezed = function() {
        var triggerValue = this.rawTriggerValue;
        return triggerValue > TRIGGER_ON_VALUE;
    };

    this.off = function() {
        if (this.triggerSmoothedSqueezed()) {
            this.lastPickTime = 0;
            this.setState(STATE_SEARCHING);
            return;
        }
    }

    this.search = function() {
        this.grabbedEntity = null;

        if (this.state == STATE_SEARCHING && this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            return;
        }

        // the trigger is being pressed, do a ray test
        var handPosition = this.getHandPosition();
        var distantPickRay = {
            origin: handPosition,
            direction: Quat.getUp(this.getHandRotation()),
            length: PICK_MAX_DISTANCE
        };

        // don't pick 60x per second.
        var pickRays = [];
        var now = Date.now();
        if (now - this.lastPickTime > MSECS_PER_SEC / PICKS_PER_SECOND_PER_HAND) {
            pickRays = [distantPickRay];
            this.lastPickTime = now;
        }

        for (var index = 0; index < pickRays.length; ++index) {
            var pickRay = pickRays[index];
            var directionNormalized = Vec3.normalize(pickRay.direction);
            var directionBacked = Vec3.multiply(directionNormalized, PICK_BACKOFF_DISTANCE);
            var pickRayBacked = {
                origin: Vec3.subtract(pickRay.origin, directionBacked),
                direction: pickRay.direction
            };


            var intersection = Entities.findRayIntersection(pickRayBacked, true);

            if (intersection.intersects) {
                // the ray is intersecting something we can move.
                var intersectionDistance = Vec3.distance(pickRay.origin, intersection.intersection);

                var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, intersection.entityID, DEFAULT_GRABBABLE_DATA);

                var properties = Entites.getEntityProperties(intersection.entityID, ["locked", "name"]);


                if (properties.name == "Grab Debug Entity") {
                    continue;
                }

                if (typeof grabbableData.grabbable !== 'undefined' && !grabbableData.grabbable) {
                    continue;
                }
                if (intersectionDistance > pickRay.length) {
                    // too far away for this ray.
                    continue;
                }
                if (intersectionDistance <= NEAR_PICK_MAX_DISTANCE) {
                    // the hand is very close to the intersected object.  go into close-grabbing mode.
                    if (grabbableData.wantsTrigger) {
                        this.grabbedEntity = intersection.entityID;
                        this.setState(STATE_NEAR_TRIGGER);
                        return;
                    } else if (!properties.locked) {
                        var ownerObj = getEntityCustomData('ownerKey', intersection.entityID, null);

                        if (ownerObj == null || ownerObj.ownerID === MyAvatar.sessionUUID) {    //I can only grab new or already mine items
                            this.grabbedEntity = intersection.entityID;
                            if (this.state == STATE_SEARCHING) {
                                this.setState(STATE_NEAR_GRABBING);
                            } else { // equipping
                                if (typeof grabbableData.spatialKey !== 'undefined') {
                                    this.setState(STATE_EQUIP_TRACTOR);
                                } else {
                                    this.setState(STATE_EQUIP);
                                }
                            }
                            return;
                        }
                    }
                }
            }
        }

        this.lineOn(distantPickRay.origin, Vec3.multiply(distantPickRay.direction, LINE_LENGTH), NO_INTERSECT_COLOR);
    };

    this.nearGrabbing = function() {
        var now = Date.now();
        var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, this.grabbedEntity, DEFAULT_GRABBABLE_DATA);

        if (this.state == STATE_NEAR_GRABBING && this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            //Entities.callEntityMethod(this.grabbedEntity, "releaseGrab");
            return;
        }

        this.lineOff();

        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
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
            if (grabbableData.spatialKey.relativePosition) {
                this.offsetPosition = grabbableData.spatialKey.relativePosition;
            }
            if (grabbableData.spatialKey.relativeRotation) {
                this.offsetRotation = grabbableData.spatialKey.relativeRotation;
            }
        } else {
            var objectRotation = grabbedProperties.rotation;
            this.offsetRotation = Quat.multiply(Quat.inverse(handRotation), objectRotation);

            var currentObjectPosition = grabbedProperties.position;
            var offset = Vec3.subtract(currentObjectPosition, handPosition);
            this.offsetPosition = Vec3.multiplyQbyV(Quat.inverse(Quat.multiply(handRotation, this.offsetRotation)), offset);
        }

        this.actionID = NULL_ACTION_ID;
        this.actionID = Entities.addAction("hold", this.grabbedEntity, {
            hand: this.hand === RIGHT_HAND ? "right" : "left",
            timeScale: NEAR_GRABBING_ACTION_TIMEFRAME,
            relativePosition: this.offsetPosition,
            relativeRotation: this.offsetRotation,
            ttl: ACTION_TTL,
            kinematic: NEAR_GRABBING_KINEMATIC,
            kinematicSetVelocity: true
        });
        if (this.actionID === NULL_ACTION_ID) {
            this.actionID = null;
        } else {
            this.actionTimeout = now + (ACTION_TTL * MSEC_PER_SEC);
            if (this.state == STATE_NEAR_GRABBING) {
                this.setState(STATE_CONTINUE_NEAR_GRABBING);
            } else {
                // equipping
                Entities.callEntityMethod(this.grabbedEntity, "startEquip", [JSON.stringify(this.hand)]);
                this.startHandGrasp();
                this.setState(STATE_CONTINUE_EQUIP_BD);
            }

            if (this.hand === RIGHT_HAND) {
                Entities.callEntityMethod(this.grabbedEntity, "setRightHand");
            } else {
                Entities.callEntityMethod(this.grabbedEntity, "setLeftHand");
            }

            Entities.callEntityMethod(this.grabbedEntity, "setHand", [this.hand]);

            Entities.callEntityMethod(this.grabbedEntity, "startNearGrab");

        }

        this.currentHandControllerTipPosition =
            (this.hand === RIGHT_HAND) ? MyAvatar.rightHandTipPosition : MyAvatar.leftHandTipPosition;

        this.currentObjectTime = Date.now();
    };

    this.continueNearGrabbing = function() {
        if (this.state == STATE_CONTINUE_NEAR_GRABBING && this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            //Entities.callEntityMethod(this.grabbedEntity, "releaseGrab");
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

        if (this.actionTimeout - now < ACTION_TTL_REFRESH * MSEC_PER_SEC) {
            // if less than a 5 seconds left, refresh the actions ttl
            Entities.updateAction(this.grabbedEntity, this.actionID, {
                hand: this.hand === RIGHT_HAND ? "right" : "left",
                timeScale: NEAR_GRABBING_ACTION_TIMEFRAME,
                relativePosition: this.offsetPosition,
                relativeRotation: this.offsetRotation,
                ttl: ACTION_TTL,
                kinematic: NEAR_GRABBING_KINEMATIC,
                kinematicSetVelocity: true
            });
            this.actionTimeout = now + (ACTION_TTL * MSEC_PER_SEC);
        }
    };


    this.pullTowardEquipPosition = function() {
        this.lineOff();

        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
        var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, this.grabbedEntity, DEFAULT_GRABBABLE_DATA);

        // use a tractor to pull the object to where it will be when equipped
        var relativeRotation = {
            x: 0.0,
            y: 0.0,
            z: 0.0,
            w: 1.0
        };
        var relativePosition = {
            x: 0.0,
            y: 0.0,
            z: 0.0
        };
        if (grabbableData.spatialKey.relativePosition) {
            relativePosition = grabbableData.spatialKey.relativePosition;
        }
        if (grabbableData.spatialKey.relativeRotation) {
            relativeRotation = grabbableData.spatialKey.relativeRotation;
        }
        var handRotation = this.getHandRotation();
        var handPosition = this.getHandPosition();
        var targetRotation = Quat.multiply(handRotation, relativeRotation);
        var offset = Vec3.multiplyQbyV(targetRotation, relativePosition);
        var targetPosition = Vec3.sum(handPosition, offset);

        if (typeof this.equipTractorID === 'undefined' ||
            this.equipTractorID === null ||
            this.equipTractorID === NULL_ACTION_ID) {
            this.equipTractorID = Entities.addAction("tractor", this.grabbedEntity, {
                targetPosition: targetPosition,
                linearTimeScale: EQUIP_TRACTOR_TIMEFRAME,
                targetRotation: targetRotation,
                angularTimeScale: EQUIP_TRACTOR_TIMEFRAME,
                ttl: ACTION_TTL
            });
            if (this.equipTractorID === NULL_ACTION_ID) {
                this.equipTractorID = null;
                this.setState(STATE_OFF);
                return;
            }
        } else {
            Entities.updateAction(this.grabbedEntity, this.equipTractorID, {
                targetPosition: targetPosition,
                linearTimeScale: EQUIP_TRACTOR_TIMEFRAME,
                targetRotation: targetRotation,
                angularTimeScale: EQUIP_TRACTOR_TIMEFRAME,
                ttl: ACTION_TTL
            });
        }

        if (Vec3.distance(grabbedProperties.position, targetPosition) < EQUIP_TRACTOR_SHUTOFF_DISTANCE) {
            Entities.deleteAction(this.grabbedEntity, this.equipTractorID);
            this.equipTractorID = null;
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
            Entities.callEntityMethod(this.grabbedEntity, "stopNearTrigger");
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

        this.lineOn(pickRay.origin, Vec3.multiply(pickRay.direction, LINE_LENGTH), NO_INTERSECT_COLOR);
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

        this.lineOff();

        if (this.grabbedEntity !== null) {
            if (this.actionID !== null) {
                Entities.deleteAction(this.grabbedEntity, this.actionID);
                Entities.callEntityMethod(this.grabbedEntity, "releaseGrab");
                print("CALLING releaseGrab");
            }
        }


        this.grabbedEntity = null;
        this.actionID = null;
        this.setState(STATE_OFF);
        
    };

    this.cleanup = function() {
        this.release();
        this.endHandGrasp();
    };




    //this is our handler, where we do the actual work of changing animation settings
    this.graspHand = function(animationProperties) {
        var result = {};
        //full alpha on overlay for this hand
        //set grab to true
        //set idle to false
        //full alpha on the blend btw open and grab
        if (_this.hand === RIGHT_HAND) {
            result['rightHandOverlayAlpha'] = 1.0;
            result['isRightHandGrab'] = true;
            result['isRightHandIdle'] = false;
            result['rightHandGrabBlend'] = 1.0;
        } else if (_this.hand === LEFT_HAND) {
            result['leftHandOverlayAlpha'] = 1.0;
            result['isLeftHandGrab'] = true;
            result['isLeftHandIdle'] = false;
            result['leftHandGrabBlend'] = 1.0;
        }
        //return an object with our updated settings
        return result;
    }

    this.graspHandler = null
    this.startHandGrasp = function() {
        if (this.hand === RIGHT_HAND) {
            this.graspHandler = MyAvatar.addAnimationStateHandler(this.graspHand, ['isRightHandGrab']);
        } else if (this.hand === LEFT_HAND) {
            this.graspHandler = MyAvatar.addAnimationStateHandler(this.graspHand, ['isLeftHandGrab']);
        }
    }

    this.endHandGrasp = function() {
        // Tell the animation system we don't need any more callbacks.
        MyAvatar.removeAnimationStateHandler(this.graspHandler);
    }

}

var rightController = new MyController(RIGHT_HAND);
var leftController = new MyController(LEFT_HAND);

var MAPPING_NAME = "com.highfidelity.handControllerGrab";

var mapping = Controller.newMapping(MAPPING_NAME);
mapping.from([Controller.Standard.RT]).peek().to(rightController.triggerPress);
mapping.from([Controller.Standard.LT]).peek().to(leftController.triggerPress);

Controller.enableMapping(MAPPING_NAME);

var handToDisable = 'none';

function update() {
    if (handToDisable !== LEFT_HAND) {
        leftController.update();
    }
    if (handToDisable !== RIGHT_HAND) {
        rightController.update();
    }
}

Messages.subscribe(SHOP_GRAB_CHANNEL);

stopScriptMessage = function(channel, message, sender) {
    if (channel == SHOP_GRAB_CHANNEL && sender === MyAvatar.sessionUUID) {
        //stop this script and enable the handControllerGrab
        Messages.sendMessage('Hifi-Hand-Disabler', "none");
        Messages.unsubscribe(SHOP_GRAB_CHANNEL);
        Messages.messageReceived.disconnect(stopScriptMessage);
        Script.stop();
    }
}

Messages.messageReceived.connect(stopScriptMessage);

function cleanup() {
    rightController.cleanup();
    leftController.cleanup();
    Controller.disableMapping(MAPPING_NAME);
}

Script.scriptEnding.connect(cleanup);
Script.update.connect(update);
