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

var RIGHT_HAND = 1;
var LEFT_HAND = 0;

var GRAB_RADIUS = 0.3;
var RADIUS_FACTOR = 4;
var ZERO_VEC = {x: 0, y: 0, z: 0};
var NULL_ACTION_ID = "{00000000-0000-0000-000000000000}";
var LINE_LENGTH = 500;

var rightController = new controller(RIGHT_HAND, Controller.findAction("RIGHT_HAND_CLICK"));
var leftController = new controller(LEFT_HAND, Controller.findAction("LEFT_HAND_CLICK"));

var startTime = Date.now();
var LIFETIME = 10;

var NO_INTERSECT_COLOR = {
    red: 10,
    green: 10,
    blue: 255
};
var INTERSECT_COLOR = {
    red: 250,
    green: 10,
    blue: 10
};


var STATE_SEARCHING = 0;
var STATE_DISTANCE_HOLDING = 1;
var STATE_CLOSE_GRABBING = 2;
var STATE_CONTINUE_CLOSE_GRABBING = 3;
var STATE_RELEASE = 4;

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
    this.tip = 2 * hand + 1;

    this.actionID = null; // action this script created...
    this.grabbedEntity = null; // on this entity.
    this.grabbedVelocity = ZERO_VEC;
    this.state = 0; // 0 = searching, 1 = distanceHolding, 2 = closeGrabbing
    this.pointer = null; // entity-id of line object
    this.triggerValue = 0; // rolling average of trigger value
}


controller.prototype.update = function() {
    switch(this.state) {
        case STATE_SEARCHING:
            search(this);
            break;
        case STATE_DISTANCE_HOLDING:
            distanceHolding(this);
            break;
        case STATE_CLOSE_GRABBING:
            closeGrabbing(this);
            break;
        case STATE_CONTINUE_CLOSE_GRABBING:
            continueCloseGrabbing(this);
            break;
        case STATE_RELEASE:
            release(this);
            break;
    }
}


function lineOn(self, closePoint, farPoint, color) {
    // draw a line
    if (self.pointer == null) {
        self.pointer = Entities.addEntity({
            type: "Line",
            name: "pointer",
            dimensions: {x: 1000, y: 1000, z: 1000},
            visible: true,
            position: closePoint,
            linePoints: [ ZERO_VEC, farPoint ],
            color: color,
            lifetime: LIFETIME
        });
    } else {
        Entities.editEntity(self.pointer, {
            position: closePoint,
            linePoints: [ ZERO_VEC, farPoint ],
            color: color,
            lifetime: (Date.now() - startTime) / 1000.0 + LIFETIME
        });
    }
}


function lineOff(self) {
    if (self.pointer != null) {
        Entities.deleteEntity(self.pointer);
    }
    self.pointer = null;
}

function triggerSmoothedSqueezed(self) {
    var triggerValue = Controller.getActionValue(self.triggerAction);
    self.triggerValue = (self.triggerValue * 0.7) + (triggerValue * 0.3); // smooth out trigger value
    return self.triggerValue > 0.2;
}

function triggerSqueezed(self) {
    var triggerValue = Controller.getActionValue(self.triggerAction);
    return triggerValue > 0.2;
}


function search(self) {
    if (!triggerSmoothedSqueezed(self)) {
        self.state = STATE_RELEASE;
        return;
    }

    // the trigger is being pressed, do a ray test
    var handPosition = self.getHandPosition();
    var pickRay = {origin: handPosition, direction: Quat.getUp(self.getHandRotation())};
    var intersection = Entities.findRayIntersection(pickRay, true);
    if (intersection.intersects &&
        intersection.properties.collisionsWillMove === 1 &&
        intersection.properties.locked === 0) {
        // the ray is intersecting something we can move.
        var handControllerPosition = Controller.getSpatialControlPosition(self.palm);
        var intersectionDistance = Vec3.distance(handControllerPosition, intersection.intersection);
        self.grabbedEntity = intersection.entityID;
        if (intersectionDistance < 0.6) {
            // the hand is very close to the intersected object.  go into close-grabbing mode.
            self.state = STATE_CLOSE_GRABBING;
        } else {
            // the hand is far from the intersected object.  go into distance-holding mode
            self.state = STATE_DISTANCE_HOLDING;
            lineOn(self, pickRay.origin, Vec3.multiply(pickRay.direction, LINE_LENGTH), NO_INTERSECT_COLOR);
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
                self.grabbedEntity = nearbyEntities[i];
                minDistance = distance;
            }
        }
        if (self.grabbedEntity === null) {
            lineOn(self, pickRay.origin, Vec3.multiply(pickRay.direction, LINE_LENGTH), NO_INTERSECT_COLOR);
        } else {
            self.state = STATE_CLOSE_GRABBING;
        }
    }
}


function distanceHolding(self) {
    if (!triggerSmoothedSqueezed(self)) {
        self.state = STATE_RELEASE;
        return;
    }

    var handPosition = self.getHandPosition();
    var handControllerPosition = Controller.getSpatialControlPosition(self.palm);
    var handRotation = Quat.multiply(MyAvatar.orientation, Controller.getSpatialControlRawRotation(self.palm));
    var grabbedProperties = Entities.getEntityProperties(self.grabbedEntity);

    lineOn(self, handPosition, Vec3.subtract(grabbedProperties.position, handPosition), INTERSECT_COLOR);

    if (self.actionID === null) {
        // first time here since trigger pulled -- add the action and initialize some variables
        self.currentObjectPosition = grabbedProperties.position;
        self.currentObjectRotation = grabbedProperties.rotation;
        self.handPreviousPosition = handControllerPosition;
        self.handPreviousRotation = handRotation;

        self.actionID = Entities.addAction("spring", self.grabbedEntity, {
            targetPosition: self.currentObjectPosition,
            linearTimeScale: .1,
            targetRotation: self.currentObjectRotation,
            angularTimeScale: .1
        });
        if (self.actionID == NULL_ACTION_ID) {
            self.actionID = null;
        }
    } else {
        // the action was set up on a previous call.  update the targets.
        var radius = Math.max(Vec3.distance(self.currentObjectPosition, handControllerPosition) * RADIUS_FACTOR, RADIUS_FACTOR);

        var handMoved = Vec3.subtract(handControllerPosition, self.handPreviousPosition);
        self.handPreviousPosition = handControllerPosition;
        var superHandMoved = Vec3.multiply(handMoved, radius);
        self.currentObjectPosition = Vec3.sum(self.currentObjectPosition, superHandMoved);

        // this doubles hand rotation
        var handChange = Quat.multiply(Quat.slerp(self.handPreviousRotation, handRotation, 2.0),
                                       Quat.inverse(self.handPreviousRotation));
        self.handPreviousRotation = handRotation;
        self.currentObjectRotation = Quat.multiply(handChange, self.currentObjectRotation);

        Entities.updateAction(self.grabbedEntity, self.actionID, {
            targetPosition: self.currentObjectPosition, linearTimeScale: .1,
            targetRotation: self.currentObjectRotation, angularTimeScale: .1
        });
    }
}


function closeGrabbing(self) {
    if (!triggerSmoothedSqueezed(self)) {
        self.state = STATE_RELEASE;
        return;
    }

    lineOff(self);

    var grabbedProperties = Entities.getEntityProperties(self.grabbedEntity);

    var handRotation = self.getHandRotation();
    var handPosition = self.getHandPosition();

    var objectRotation = grabbedProperties.rotation;
    var offsetRotation = Quat.multiply(Quat.inverse(handRotation), objectRotation);

    self.currentObjectPosition = grabbedProperties.position;
    self.currentObjectTime = Date.now();
    var offset = Vec3.subtract(self.currentObjectPosition, handPosition);
    var offsetPosition = Vec3.multiplyQbyV(Quat.inverse(Quat.multiply(handRotation, offsetRotation)), offset);

    self.actionID = Entities.addAction("hold", self.grabbedEntity, {
        hand: self.hand == RIGHT_HAND ? "right" : "left",
        timeScale: 0.05,
        relativePosition: offsetPosition,
        relativeRotation: offsetRotation
    });
    if (self.actionID == NULL_ACTION_ID) {
        self.actionID = null;
    } else {
        self.state = STATE_CONTINUE_CLOSE_GRABBING;
    }
}


function continueCloseGrabbing(self) {
    if (!triggerSmoothedSqueezed(self)) {
        self.state = STATE_RELEASE;
        return;
    }

    // keep track of the measured velocity of the held object
    var grabbedProperties = Entities.getEntityProperties(self.grabbedEntity);
    var now = Date.now();

    var deltaPosition = Vec3.subtract(grabbedProperties.position, self.currentObjectPosition);
    var deltaTime = (now - self.currentObjectTime) / 1000.0; // convert to seconds

    if (deltaTime > 0.0) {
        var grabbedVelocity = Vec3.multiply(deltaPosition, 1.0 / deltaTime);
        // don't update grabbedVelocity if the trigger is off.  the smoothing of the trigger
        // value would otherwise give the held object time to slow down.
        if (triggerSqueezed(self)) {
            self.grabbedVelocity = Vec3.sum(Vec3.multiply(self.grabbedVelocity, 0.1),
                                            Vec3.multiply(grabbedVelocity, 0.9));
        }
    }

    self.currentObjectPosition = grabbedProperties.position;
    self.currentObjectTime = now;
}


function release(self) {
    lineOff(self);

    if (self.grabbedEntity != null && self.actionID != null) {
        Entities.deleteAction(self.grabbedEntity, self.actionID);
    }

    // the action will tend to quickly bring an object's velocity to zero.  now that
    // the action is gone, set the objects velocity to something the holder might expect.
    Entities.editEntity(self.grabbedEntity, {velocity: self.grabbedVelocity});

    self.grabbedVelocity = ZERO_VEC;
    self.grabbedEntity = null;
    self.actionID = null;
    self.state = STATE_SEARCHING;
}


controller.prototype.cleanup = function() {
    release(this);
}


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
