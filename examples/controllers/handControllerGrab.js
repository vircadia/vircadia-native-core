//  hydraGrab.js
//  examples
//
//  Created by Eric Levin on  9/2/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Grab's physically moveable entities with the hydra- works for either near or far objects. User can also grab a far away object and drag it towards them by pressing the "4" button on either the left or ride controller.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


Script.include("../libraries/utils.js");

var RIGHT_HAND_CLICK = Controller.findAction("RIGHT_HAND_CLICK");
var rightTriggerAction = RIGHT_HAND_CLICK;

var GRAB_USER_DATA_KEY = "grabKey";

var LEFT_HAND_CLICK = Controller.findAction("LEFT_HAND_CLICK");
var leftTriggerAction = LEFT_HAND_CLICK;

var LIFETIME = 10;
var EXTRA_TIME = 5;
var POINTER_CHECK_TIME = 5000;

var ZERO_VEC = {
    x: 0,
    y: 0,
    z: 0
}
var LINE_LENGTH = 500;
var THICK_LINE_WIDTH = 7;
var THIN_LINE_WIDTH = 2;

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

var GRAB_RADIUS = 1.5;

var GRAB_COLOR = {
    red: 250,
    green: 10,
    blue: 250
};
var SHOW_LINE_THRESHOLD = 0.2;
var DISTANCE_HOLD_THRESHOLD = 0.8;

var right4Action = 18;
var left4Action = 17;

var TRACTOR_BEAM_VELOCITY_THRESHOLD = 0.5;

var RIGHT = 1;
var LEFT = 0;
var rightController = new controller(RIGHT, rightTriggerAction, right4Action, "right");
var leftController = new controller(LEFT, leftTriggerAction, left4Action, "left");


//Need to wait before calling these methods for some reason...
Script.setTimeout(function() {
    rightController.checkPointer();
    leftController.checkPointer();
}, 100)

function controller(side, triggerAction, pullAction, hand) {
    this.hand = hand;
    if (hand === "right") {
        this.getHandPosition = MyAvatar.getRightPalmPosition;
        this.getHandRotation = MyAvatar.getRightPalmRotation;
    } else {

        this.getHandPosition = MyAvatar.getLeftPalmPosition;
        this.getHandRotation = MyAvatar.getLeftPalmRotation;
    }
    this.triggerAction = triggerAction;
    this.pullAction = pullAction;
    this.actionID = null;
    this.tractorBeamActive = false;
    this.distanceHolding = false;
    this.closeGrabbing = false;
    this.triggerValue = 0;
    this.prevTriggerValue = 0;
    this.palm = 2 * side;
    this.tip = 2 * side + 1;
    this.pointer = Entities.addEntity({
        type: "Line",
        name: "pointer",
        color: NO_INTERSECT_COLOR,
        dimensions: {
            x: 1000,
            y: 1000,
            z: 1000
        },
        visible: false,
        lifetime: LIFETIME
    });

}


controller.prototype.updateLine = function() {
    var handPosition = Controller.getSpatialControlPosition(this.palm);
    var direction = Controller.getSpatialControlNormal(this.tip);

    Entities.editEntity(this.pointer, {
        position: handPosition,
        linePoints: [
            ZERO_VEC,
            Vec3.multiply(direction, LINE_LENGTH)
        ]
    });

    //only check if we havent already grabbed an object
    if (this.distanceHolding) {
        return;
    }

    //move origin a bit away from hand so nothing gets in way
    var origin = Vec3.sum(handPosition, direction);
    if (this.checkForIntersections(origin, direction)) {
        Entities.editEntity(this.pointer, {
            color: INTERSECT_COLOR,
        });
    } else {
        Entities.editEntity(this.pointer, {
            color: NO_INTERSECT_COLOR,
        });
    }
}


controller.prototype.checkPointer = function() {
    var self = this;
    Script.setTimeout(function() {
        var props = Entities.getEntityProperties(self.pointer);
        Entities.editEntity(self.pointer, {
            lifetime: props.age + EXTRA_TIME
        });
        self.checkPointer();
    }, POINTER_CHECK_TIME);
}

controller.prototype.checkForIntersections = function(origin, direction) {
    var pickRay = {
        origin: origin,
        direction: direction
    };

    var intersection = Entities.findRayIntersection(pickRay, true);
    if (intersection.intersects && intersection.properties.collisionsWillMove === 1) {
        var handPosition = Controller.getSpatialControlPosition(this.palm);
        this.distanceToEntity = Vec3.distance(handPosition, intersection.properties.position);
        Entities.editEntity(this.pointer, {
            linePoints: [
                ZERO_VEC,
                Vec3.multiply(direction, this.distanceToEntity)
            ]
        });
        this.grabbedEntity = intersection.entityID;
        return true;
    }
    return false;
}

controller.prototype.attemptMove = function() {
    if (this.tractorBeamActive) {
        return;
    }
    if (this.grabbedEntity || this.distanceHolding) {
        var handPosition = Controller.getSpatialControlPosition(this.palm);
        var direction = Controller.getSpatialControlNormal(this.tip);

        var newPosition = Vec3.sum(handPosition, Vec3.multiply(direction, this.distanceToEntity))
        this.distanceHolding = true;
        if (this.actionID === null) {
            this.actionID = Entities.addAction("spring", this.grabbedEntity, {
                targetPosition: newPosition,
                linearTimeScale: .1
            });
        } else {
            Entities.updateAction(this.grabbedEntity, this.actionID, {
                targetPosition: newPosition
            });
        }
    }

}

controller.prototype.showPointer = function() {
    Entities.editEntity(this.pointer, {
        visible: true
    });

}

controller.prototype.hidePointer = function() {
    Entities.editEntity(this.pointer, {
        visible: false
    });
}


controller.prototype.letGo = function() {
    if (this.grabbedEntity && this.actionID) {
        this.deactivateEntity(this.grabbedEntity);
        Entities.deleteAction(this.grabbedEntity, this.actionID);
    }
    this.grabbedEntity = null;
    this.actionID = null;
    this.distanceHolding = false;
    this.tractorBeamActive = false;
    this.checkForEntityArrival = false;
    this.closeGrabbing = false;
}

controller.prototype.update = function() {
    if (this.tractorBeamActive && this.checkForEntityArrival) {
        var entityVelocity = Entities.getEntityProperties(this.grabbedEntity).velocity
        if (Vec3.length(entityVelocity) < TRACTOR_BEAM_VELOCITY_THRESHOLD) {
            this.letGo();
        }
        return;
    }
    this.triggerValue = Controller.getActionValue(this.triggerAction);
    if (this.triggerValue > SHOW_LINE_THRESHOLD && this.prevTriggerValue < SHOW_LINE_THRESHOLD) {
        //First check if an object is within close range and then run the close grabbing logic
        if (this.checkForInRangeObject()) {
            this.grabEntity();
        } else {
            this.showPointer();
            this.shouldDisplayLine = true;
        }
    } else if (this.triggerValue < SHOW_LINE_THRESHOLD && this.prevTriggerValue > SHOW_LINE_THRESHOLD) {
        this.hidePointer();
        this.letGo();
        this.shouldDisplayLine = false;
    }

    if (this.shouldDisplayLine) {
        this.updateLine();
    }
    if (this.triggerValue > DISTANCE_HOLD_THRESHOLD && !this.closeGrabbing) {
        this.attemptMove();
    }


    this.prevTriggerValue = this.triggerValue;
}

controller.prototype.grabEntity = function() {
    var handRotation = this.getHandRotation();
    var handPosition = this.getHandPosition();
    this.closeGrabbing = true;
    //check if our entity has instructions on how to be grabbed, otherwise, just use default relative position and rotation
    var userData = getEntityUserData(this.grabbedEntity);

    var objectRotation = Entities.getEntityProperties(this.grabbedEntity).rotation;
    var offsetRotation = Quat.multiply(Quat.inverse(handRotation), objectRotation);

    var objectPosition = Entities.getEntityProperties(this.grabbedEntity).position;
    var offset = Vec3.subtract(objectPosition, handPosition);
    var offsetPosition = Vec3.multiplyQbyV(Quat.inverse(Quat.multiply(handRotation, offsetRotation)), offset);

    var relativePosition = offsetPosition;
    var relativeRotation = offsetRotation;
    if (userData.grabFrame) {
        if (userData.grabFrame.relativePosition) {
            relativePosition = userData.grabFrame.relativePosition;
        }
        if (userData.grabFrame.relativeRotation) {
            relativeRotation = userData.grabFrame.relativeRotation;
        }
    }
    this.actionID = Entities.addAction("hold", this.grabbedEntity, {
        hand: this.hand,
        timeScale: 0.05,
        relativePosition: relativePosition,
        relativeRotation: relativeRotation
    });
}


controller.prototype.checkForInRangeObject = function() {
    var handPosition = Controller.getSpatialControlPosition(this.palm);
    var entities = Entities.findEntities(handPosition, GRAB_RADIUS);
    var minDistance = GRAB_RADIUS;
    var grabbedEntity = null;
    //Get nearby entities and assign nearest
    for (var i = 0; i < entities.length; i++) {
        var props = Entities.getEntityProperties(entities[i]);
        var distance = Vec3.distance(props.position, handPosition);
        if (distance < minDistance && props.name !== "pointer" && props.collisionsWillMove === 1) {
            grabbedEntity = entities[i];
            minDistance = distance;
        }
    }
    if (grabbedEntity === null) {
        return false;
    } else {
        //We are grabbing an entity, so let it know we've grabbed it
        this.grabbedEntity = grabbedEntity;
        this.activateEntity(this.grabbedEntity);

        return true;
    }
}

controller.prototype.activateEntity = function(entity) {
    var data = {
        activated: true,
        avatarId: MyAvatar.sessionUUID
    };
    setEntityCustomData(GRAB_USER_DATA_KEY, this.grabbedEntity, data);
}

controller.prototype.deactivateEntity = function(entity) {
    var data = {
        activated: false,
        avatarId: null
    };
    setEntityCustomData(GRAB_USER_DATA_KEY, this.grabbedEntity, data);
}

controller.prototype.onActionEvent = function(action, state) {
    if (this.pullAction === action && state === 1) {
        if (this.actionID !== null) {
            var self = this;
            this.tractorBeamActive = true;
            //We need to wait a bit before checking for entity arrival at target destination (meaning checking for velocity being close to some 
            //low threshold) because otherwise we'll think the entity has arrived before its even really gotten moving! 
            Script.setTimeout(function() {
                self.checkForEntityArrival = true;
            }, 500);
            var handPosition = Controller.getSpatialControlPosition(this.palm);
            var direction = Vec3.normalize(Controller.getSpatialControlNormal(this.tip));
            //move final destination along line a bit, so it doesnt hit avatar hand
            Entities.updateAction(this.grabbedEntity, this.actionID, {
                targetPosition: Vec3.sum(handPosition, Vec3.multiply(3, direction))
            });
        }
    }

}

controller.prototype.cleanup = function() {
    Entities.deleteEntity(this.pointer);
    if (this.grabbedEntity) {
        Entities.deleteAction(this.grabbedEntity, this.actionID);
    }
}

function update() {
    rightController.update();
    leftController.update();
}

function onActionEvent(action, state) {
    rightController.onActionEvent(action, state);
    leftController.onActionEvent(action, state);

}


function cleanup() {
    rightController.cleanup();
    leftController.cleanup();
}

Script.scriptEnding.connect(cleanup);
Script.update.connect(update)
Controller.actionEvent.connect(onActionEvent);