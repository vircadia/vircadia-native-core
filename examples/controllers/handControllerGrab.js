//  hydraGrab.js
//  examples
//
//  Created by Eric Levin on  9/2/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Grabs physically moveable entities with hydra-like controllers; it works for either near or far objects.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../libraries/utils.js");


/////////////////////////////////////////////////////////////////
//
// these tune time-averaging and "on" value for analog trigger
//

var TRIGGER_SMOOTH_RATIO = 0.1; // 0.0 disables smoothing of trigger value
var TRIGGER_ON_VALUE = 0.2;

/////////////////////////////////////////////////////////////////
//
// distant manipulation
//

var DISTANCE_HOLDING_RADIUS_FACTOR = 5; // multiplied by distance between hand and object
var DISTANCE_HOLDING_ACTION_TIMEFRAME = 0.1; // how quickly objects move to their new position
var DISTANCE_HOLDING_ROTATION_EXAGGERATION_FACTOR = 2.0; // object rotates this much more than hand did
var NO_INTERSECT_COLOR = {red: 10, green: 10, blue: 255}; // line color when pick misses
var INTERSECT_COLOR = {red: 250, green: 10, blue: 10}; // line color when pick hits
var LINE_ENTITY_DIMENSIONS = {x: 1000, y: 1000, z: 1000};
var LINE_LENGTH = 500;


/////////////////////////////////////////////////////////////////
//
// near grabbing
//

var GRAB_RADIUS = 0.3; // if the ray misses but an object is this close, it will still be selected
var NEAR_GRABBING_ACTION_TIMEFRAME = 0.05; // how quickly objects move to their new position
var NEAR_GRABBING_VELOCITY_SMOOTH_RATIO = 1.0; // adjust time-averaging of held object's velocity.  1.0 to disable.
var NEAR_PICK_MAX_DISTANCE = 0.6; // max length of pick-ray for close grabbing to be selected
var RELEASE_VELOCITY_MULTIPLIER = 1.5; // affects throwing things

/////////////////////////////////////////////////////////////////
//
// other constants
//

var RIGHT_HAND = 1;
var LEFT_HAND = 0;

var ZERO_VEC = {x: 0, y: 0, z: 0};
var NULL_ACTION_ID = "{00000000-0000-0000-000000000000}";
var MSEC_PER_SEC = 1000.0;

// these control how long an abandoned pointer line will hang around
var startTime = Date.now();
var LIFETIME = 10;

// states for the state machine
var STATE_SEARCHING = 0;
var STATE_DISTANCE_HOLDING = 1;
var STATE_CONTINUE_DISTANCE_HOLDING = 2;
var STATE_NEAR_GRABBING = 3;
var STATE_CONTINUE_NEAR_GRABBING = 4;
var STATE_RELEASE = 5;

var GRAB_USER_DATA_KEY = "grabKey";

function controller(hand, triggerAction) {
    this.hand = hand;
    if (this.hand === RIGHT_HAND) {
        this.getHandPosition = MyAvatar.getRightPalmPosition;
        this.getHandRotation = MyAvatar.getRightPalmRotation;
    } else {
        this.getHandPosition = MyAvatar.getLeftPalmPosition;
        this.getHandRotation = MyAvatar.getLeftPalmRotation;
    }
    this.triggerAction = triggerAction;
    this.palm = 2 * hand;
    // this.tip = 2 * hand + 1; // unused, but I'm leaving this here for fear it will be needed

    this.actionID = null; // action this script created...
    this.grabbedEntity = null; // on this entity.
    this.grabbedVelocity = ZERO_VEC; // rolling average of held object's velocity
    this.state = 0;
    this.pointer = null; // entity-id of line object
    this.triggerValue = 0; // rolling average of trigger value

    this.update = function() {
        switch(this.state) {
            case STATE_SEARCHING:
                this.search();
                break;
            case STATE_DISTANCE_HOLDING:
                this.distanceHolding();
                break;
            case STATE_CONTINUE_DISTANCE_HOLDING:
                this.continueDistanceHolding();
                break;
            case STATE_NEAR_GRABBING:
                this.nearGrabbing();
                break;
            case STATE_CONTINUE_NEAR_GRABBING:
                this.continueNearGrabbing();
                break;
            case STATE_RELEASE:
                this.release();
                break;
        }
    }


    this.lineOn = function(closePoint, farPoint, color) {
        // draw a line
        if (this.pointer == null) {
            this.pointer = Entities.addEntity({
                type: "Line",
                name: "pointer",
                dimensions: LINE_ENTITY_DIMENSIONS,
                visible: true,
                position: closePoint,
                linePoints: [ ZERO_VEC, farPoint ],
                color: color,
                lifetime: LIFETIME
            });
        } else {
            Entities.editEntity(this.pointer, {
                position: closePoint,
                linePoints: [ ZERO_VEC, farPoint ],
                color: color,
                lifetime: (Date.now() - startTime) / MSEC_PER_SEC + LIFETIME
            });
        }
    }


    this.lineOff = function() {
        if (this.pointer != null) {
            Entities.deleteEntity(this.pointer);
        }
        this.pointer = null;
    }


    this.triggerSmoothedSqueezed = function() {
        var triggerValue = Controller.getActionValue(this.triggerAction);
        // smooth out trigger value
        this.triggerValue = (this.triggerValue * TRIGGER_SMOOTH_RATIO) +
            (triggerValue * (1.0 - TRIGGER_SMOOTH_RATIO));
        return this.triggerValue > TRIGGER_ON_VALUE;
    }


    this.triggerSqueezed = function() {
        var triggerValue = Controller.getActionValue(this.triggerAction);
        return triggerValue > TRIGGER_ON_VALUE;
    }


    this.search = function() {
        if (!this.triggerSmoothedSqueezed()) {
            this.state = STATE_RELEASE;
            return;
        }

        // the trigger is being pressed, do a ray test
        var handPosition = this.getHandPosition();
        var pickRay = {origin: handPosition, direction: Quat.getUp(this.getHandRotation())};
        var intersection = Entities.findRayIntersection(pickRay, true);
        if (intersection.intersects &&
            intersection.properties.collisionsWillMove === 1 &&
            intersection.properties.locked === 0) {
            // the ray is intersecting something we can move.
            var handControllerPosition = Controller.getSpatialControlPosition(this.palm);
            var intersectionDistance = Vec3.distance(handControllerPosition, intersection.intersection);
            this.grabbedEntity = intersection.entityID;
            if (intersectionDistance < NEAR_PICK_MAX_DISTANCE) {
                // the hand is very close to the intersected object.  go into close-grabbing mode.
                this.state = STATE_NEAR_GRABBING;
            } else {
                // the hand is far from the intersected object.  go into distance-holding mode
                this.state = STATE_DISTANCE_HOLDING;
                this.lineOn(pickRay.origin, Vec3.multiply(pickRay.direction, LINE_LENGTH), NO_INTERSECT_COLOR);
            }
        } else {
            // forward ray test failed, try sphere test.
            var nearbyEntities = Entities.findEntities(handPosition, GRAB_RADIUS);
            var minDistance = GRAB_RADIUS;
            var grabbedEntity = null;
            for (var i = 0; i < nearbyEntities.length; i++) {
                var props = Entities.getEntityProperties(nearbyEntities[i]);
                var distance = Vec3.distance(props.position, handPosition);
                if (distance < minDistance && props.name !== "pointer" &&
                    props.collisionsWillMove === 1 &&
                    props.locked === 0) {
                    this.grabbedEntity = nearbyEntities[i];
                    minDistance = distance;
                }
            }
            if (this.grabbedEntity === null) {
                this.lineOn(pickRay.origin, Vec3.multiply(pickRay.direction, LINE_LENGTH), NO_INTERSECT_COLOR);
            } else {
                this.state = STATE_NEAR_GRABBING;
            }
        }
    }


    this.distanceHolding = function() {
        var handControllerPosition = Controller.getSpatialControlPosition(this.palm);
        var handRotation = Quat.multiply(MyAvatar.orientation, Controller.getSpatialControlRawRotation(this.palm));
        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, ["position","rotation"]);

        // add the action and initialize some variables
        this.currentObjectPosition = grabbedProperties.position;
        this.currentObjectRotation = grabbedProperties.rotation;
        this.currentObjectTime = Date.now();
        this.handPreviousPosition = handControllerPosition;
        this.handPreviousRotation = handRotation;

        this.actionID = Entities.addAction("spring", this.grabbedEntity, {
            targetPosition: this.currentObjectPosition,
            linearTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME,
            targetRotation: this.currentObjectRotation,
            angularTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME
        });
        if (this.actionID == NULL_ACTION_ID) {
            this.actionID = null;
        }

        if (this.actionID != null) {
            this.state = STATE_CONTINUE_DISTANCE_HOLDING;
            this.activateEntity(this.grabbedEntity);
            Entities.callEntityMethod(this.grabbedEntity, "startDistantGrab");

            if (this.hand === RIGHT_HAND) {
                Entities.callEntityMethod(this.grabbedEntity, "setRightHand");
            } else {
                Entities.callEntityMethod(this.grabbedEntity, "setLeftHand");
            }
        }
    }


    this.continueDistanceHolding = function() {
        if (!this.triggerSmoothedSqueezed()) {
            this.state = STATE_RELEASE;
            return;
        }

        var handPosition = this.getHandPosition();
        var handControllerPosition = Controller.getSpatialControlPosition(this.palm);
        var handRotation = Quat.multiply(MyAvatar.orientation, Controller.getSpatialControlRawRotation(this.palm));
        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, ["position", "rotation"]);

        this.lineOn(handPosition, Vec3.subtract(grabbedProperties.position, handPosition), INTERSECT_COLOR);

        // the action was set up on a previous call.  update the targets.
        var radius = Math.max(Vec3.distance(this.currentObjectPosition,
                                            handControllerPosition) * DISTANCE_HOLDING_RADIUS_FACTOR,
                              DISTANCE_HOLDING_RADIUS_FACTOR);

        var handMoved = Vec3.subtract(handControllerPosition, this.handPreviousPosition);
        this.handPreviousPosition = handControllerPosition;
        var superHandMoved = Vec3.multiply(handMoved, radius);

        var newObjectPosition = Vec3.sum(this.currentObjectPosition, superHandMoved);
        var deltaPosition = Vec3.subtract(newObjectPosition, this.currentObjectPosition); // meters
        var now = Date.now();
        var deltaTime = (now - this.currentObjectTime) / MSEC_PER_SEC; // convert to seconds
        this.computeReleaseVelocity(deltaPosition, deltaTime, false);

        this.currentObjectPosition = newObjectPosition;
        this.currentObjectTime = now;

        // this doubles hand rotation
        var handChange = Quat.multiply(Quat.slerp(this.handPreviousRotation, handRotation,
                                                  DISTANCE_HOLDING_ROTATION_EXAGGERATION_FACTOR),
                                       Quat.inverse(this.handPreviousRotation));
        this.handPreviousRotation = handRotation;
        this.currentObjectRotation = Quat.multiply(handChange, this.currentObjectRotation);

        Entities.callEntityMethod(this.grabbedEntity, "continueDistantGrab");

        Entities.updateAction(this.grabbedEntity, this.actionID, {
            targetPosition: this.currentObjectPosition, linearTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME,
            targetRotation: this.currentObjectRotation, angularTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME
        });
    }


    this.nearGrabbing = function() {
        if (!this.triggerSmoothedSqueezed()) {
            this.state = STATE_RELEASE;
            return;
        }

        this.lineOff();

        this.activateEntity(this.grabbedEntity);

        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, ["position", "rotation"]);

        var handRotation = this.getHandRotation();
        var handPosition = this.getHandPosition();

        var objectRotation = grabbedProperties.rotation;
        var offsetRotation = Quat.multiply(Quat.inverse(handRotation), objectRotation);

        currentObjectPosition = grabbedProperties.position;
        var offset = Vec3.subtract(currentObjectPosition, handPosition);
        var offsetPosition = Vec3.multiplyQbyV(Quat.inverse(Quat.multiply(handRotation, offsetRotation)), offset);

        this.actionID = Entities.addAction("hold", this.grabbedEntity, {
            hand: this.hand == RIGHT_HAND ? "right" : "left",
            timeScale: NEAR_GRABBING_ACTION_TIMEFRAME,
            relativePosition: offsetPosition,
            relativeRotation: offsetRotation
        });
        if (this.actionID == NULL_ACTION_ID) {
            this.actionID = null;
        } else {
            this.state = STATE_CONTINUE_NEAR_GRABBING;
            Entities.callEntityMethod(this.grabbedEntity, "startNearGrab");
            if (this.hand === RIGHT_HAND) {
                Entities.callEntityMethod(this.grabbedEntity, "setRightHand");
            } else {
                Entities.callEntityMethod(this.grabbedEntity, "setLeftHand");
            }
        }

        this.currentHandControllerPosition = Controller.getSpatialControlPosition(this.palm);
        this.currentObjectTime = Date.now();
    }


    this.continueNearGrabbing = function() {
        if (!this.triggerSmoothedSqueezed()) {
            this.state = STATE_RELEASE;
            return;
        }

        // keep track of the measured velocity of the held object
        var handControllerPosition = Controller.getSpatialControlPosition(this.palm);
        var now = Date.now();

        var deltaPosition = Vec3.subtract(handControllerPosition, this.currentHandControllerPosition); // meters
        var deltaTime = (now - this.currentObjectTime) / MSEC_PER_SEC; // convert to seconds
        this.computeReleaseVelocity(deltaPosition, deltaTime, true);

        this.currentHandControllerPosition = handControllerPosition;
        this.currentObjectTime = now;
        Entities.callEntityMethod(this.grabbedEntity, "continueNearGrab");
    }


    this.computeReleaseVelocity = function(deltaPosition, deltaTime, useMultiplier) {
        if (deltaTime > 0.0 && !vec3equal(deltaPosition, ZERO_VEC)) {
            var grabbedVelocity = Vec3.multiply(deltaPosition, 1.0 / deltaTime);
            // don't update grabbedVelocity if the trigger is off.  the smoothing of the trigger
            // value would otherwise give the held object time to slow down.
            if (this.triggerSqueezed()) {
                this.grabbedVelocity =
                    Vec3.sum(Vec3.multiply(this.grabbedVelocity,
                                           (1.0 - NEAR_GRABBING_VELOCITY_SMOOTH_RATIO)),
                             Vec3.multiply(grabbedVelocity, NEAR_GRABBING_VELOCITY_SMOOTH_RATIO));
            }

            if (useMultiplier) {
                this.grabbedVelocity = Vec3.multiply(this.grabbedVelocity, RELEASE_VELOCITY_MULTIPLIER);
            }
        }
    }


    this.release = function() {
        this.lineOff();

        if (this.grabbedEntity != null && this.actionID != null) {
            Entities.deleteAction(this.grabbedEntity, this.actionID);
            Entities.callEntityMethod(this.grabbedEntity, "releaseGrab");
        }

        // the action will tend to quickly bring an object's velocity to zero.  now that
        // the action is gone, set the objects velocity to something the holder might expect.
        Entities.editEntity(this.grabbedEntity, {velocity: this.grabbedVelocity});
        this.deactivateEntity(this.grabbedEntity);

        this.grabbedVelocity = ZERO_VEC;
        this.grabbedEntity = null;
        this.actionID = null;
        this.state = STATE_SEARCHING;
    }


    this.cleanup = function() {
        release();
    }

    this.activateEntity = function(entity) {
        var data = {
            activated: true,
            avatarId: MyAvatar.sessionUUID
        };
        setEntityCustomData(GRAB_USER_DATA_KEY, this.grabbedEntity, data);
    }

    this.deactivateEntity = function(entity) {
        var data = {
            activated: false,
            avatarId: null
        };
        setEntityCustomData(GRAB_USER_DATA_KEY, this.grabbedEntity, data);
    }
}


var rightController = new controller(RIGHT_HAND, Controller.findAction("RIGHT_HAND_CLICK"));
var leftController = new controller(LEFT_HAND, Controller.findAction("LEFT_HAND_CLICK"));


function update() {
    rightController.update();
    leftController.update();
}


function cleanup() {
    rightController.cleanup();
    leftController.cleanup();
}


Script.scriptEnding.connect(cleanup);
Script.update.connect(update)
