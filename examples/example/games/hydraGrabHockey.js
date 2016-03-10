//
//  hydraGrabHockey.js
//  examples
//
//  Created by Eric Levin on 5/14/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This script allows you to grab and move physical objects with the hydra
//  Same as hydraGrab.js, but you object movement is constrained to xz plane and cannot rotate object
//  
//
//  Using the hydras :
//  grab physical entities with the right hydra trigger
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var addedVelocity, newVelocity, angularVelocity, dT, cameraEntityDistance;
var LEFT = 0;
var RIGHT = 1;
var LASER_WIDTH = 3;
var LASER_COLOR = {
  red: 50,
  green: 150,
  blue: 200
};
var LASER_HOVER_COLOR = {
  red: 200,
  green: 50,
  blue: 50
};

var DROP_DISTANCE = 5.0;
var DROP_COLOR = {
  red: 200,
  green: 200,
  blue: 200
};

var FULL_STRENGTH = 0.2;
var LASER_LENGTH_FACTOR = 500;
var CLOSE_ENOUGH = 0.001;
var SPRING_RATE = 1.5;
var DAMPING_RATE = 0.8;
var SCREEN_TO_METERS = 0.001;
var DISTANCE_SCALE_FACTOR = 1000

var grabSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/eric/sounds/CloseClamp.wav");
var releaseSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/eric/sounds/ReleaseClamp.wav");

function getRayIntersection(pickRay) {
  var intersection = Entities.findRayIntersection(pickRay, true);
  return intersection;
}


function controller(side) {
  this.triggerHeld = false;
  this.triggerThreshold = 0.9;
  this.side = side;
  this.trigger = side == LEFT ? Controller.Standard.LT : Controller.Standard.RT;
  this.originalGravity = {
    x: 0,
    y: 0,
    z: 0
  };

  this.laser = Overlays.addOverlay("line3d", {
    start: {
      x: 0,
      y: 0,
      z: 0
    },
    end: {
      x: 0,
      y: 0,
      z: 0
    },
    color: LASER_COLOR,
    alpha: 1,
    lineWidth: LASER_WIDTH,
    anchor: "MyAvatar"
  });

  this.dropLine = Overlays.addOverlay("line3d", {
    color: DROP_COLOR,
    alpha: 1,
    visible: false,
    lineWidth: 2
  });


  this.update = function(deltaTime) {
    this.updateControllerState();
    this.moveLaser();
    this.checkTrigger();
    this.checkEntityIntersection();
    if (this.grabbing) {
      this.updateEntity(deltaTime);
    }

    this.oldPalmPosition = this.palmPosition;
    this.oldTipPosition = this.tipPosition;
  }

  this.updateEntity = function(deltaTime) {
    this.dControllerPosition = Vec3.subtract(this.palmPosition, this.oldPalmPosition);
    this.dControllerPosition.y = 0;
    this.cameraEntityDistance = Vec3.distance(Camera.getPosition(), this.currentPosition);
    this.targetPosition = Vec3.sum(this.targetPosition, Vec3.multiply(this.dControllerPosition, this.cameraEntityDistance * SCREEN_TO_METERS * DISTANCE_SCALE_FACTOR));

    this.entityProps = Entities.getEntityProperties(this.grabbedEntity);
    this.currentPosition = this.entityProps.position;
    this.currentVelocity = this.entityProps.velocity;

    var dPosition = Vec3.subtract(this.targetPosition, this.currentPosition);
    this.distanceToTarget = Vec3.length(dPosition);
    if (this.distanceToTarget > CLOSE_ENOUGH) {
      //  compute current velocity in the direction we want to move 
      this.velocityTowardTarget = Vec3.dot(this.currentVelocity, Vec3.normalize(dPosition));
      this.velocityTowardTarget = Vec3.multiply(Vec3.normalize(dPosition), this.velocityTowardTarget);
      //  compute the speed we would like to be going toward the target position 

      this.desiredVelocity = Vec3.multiply(dPosition, (1.0 / deltaTime) * SPRING_RATE);
      //  compute how much we want to add to the existing velocity
      this.addedVelocity = Vec3.subtract(this.desiredVelocity, this.velocityTowardTarget);
       //If target is to far, roll off force as inverse square of distance
      if(this.distanceToTarget/ this.cameraEntityDistance > FULL_STRENGTH) {
         this.addedVelocity = Vec3.multiply(this.addedVelocity, Math.pow(FULL_STRENGTH/ this.distanceToTarget, 2.0));
      }
      this.newVelocity = Vec3.sum(this.currentVelocity, this.addedVelocity);
      this.newVelocity = Vec3.subtract(this.newVelocity, Vec3.multiply(this.newVelocity, DAMPING_RATE));
    } else {
      this.newVelocity = {
        x: 0,
        y: 0,
        z: 0
      };
    }
    Entities.editEntity(this.grabbedEntity, {
      velocity: this.newVelocity,
    });

    this.updateDropLine(this.targetPosition);

  }


  this.updateControllerState = function() {
    this.palmPosition = this.side == RIGHT ? MyAvatar.rightHandPose.translation : MyAvatar.leftHandPose.translation;
    this.tipPosition = this.side == RIGHT ? MyAvatar.rightHandTipPose.translation : MyAvatar.leftHandTipPose.translation;
    this.triggerValue = Controller.getTriggerValue(this.trigger);
  }

  this.checkTrigger = function() {
    if (this.triggerValue > this.triggerThreshold && !this.triggerHeld) {
      this.triggerHeld = true;
    } else if (this.triggerValue < this.triggerThreshold && this.triggerHeld) {
      this.triggerHeld = false;
      if (this.grabbing) {
        this.release();
      }
    }
  }


  this.updateDropLine = function(position) {

    Overlays.editOverlay(this.dropLine, {
      visible: true,
      start: {
        x: position.x,
        y: position.y + DROP_DISTANCE,
        z: position.z
      },
      end: {
        x: position.x,
        y: position.y - DROP_DISTANCE,
        z: position.z
      }
    });

  }

  this.checkEntityIntersection = function() {

    var pickRay = {
      origin: this.palmPosition,
      direction: Vec3.normalize(Vec3.subtract(this.tipPosition, this.palmPosition))
    };
    var intersection = getRayIntersection(pickRay, true);
    if (intersection.intersects && intersection.properties.dynamic) {
      this.laserWasHovered = true;
      if (this.triggerHeld && !this.grabbing) {
        this.grab(intersection.entityID);
      }
      Overlays.editOverlay(this.laser, {
        color: LASER_HOVER_COLOR
      });
    } else if (this.laserWasHovered) {
      this.laserWasHovered = false;
      Overlays.editOverlay(this.laser, {
        color: LASER_COLOR
      });
    }
  }

  this.grab = function(entityId) {
    this.grabbing = true;
    this.grabbedEntity = entityId;
    this.entityProps = Entities.getEntityProperties(this.grabbedEntity);
    this.targetPosition = this.entityProps.position;
    this.currentPosition = this.targetPosition;
    this.oldPalmPosition = this.palmPosition;
    this.originalGravity = this.entityProps.gravity;
    Entities.editEntity(this.grabbedEntity, {
      gravity: {
        x: 0,
        y: 0,
        z: 0
      }
    });
    Overlays.editOverlay(this.laser, {
      visible: false
    });
    Audio.playSound(grabSound, {
      position: this.entityProps.position,
      volume: 0.25
    });
  }

  this.release = function() {
    this.grabbing = false;
    this.grabbedEntity = null;
    Overlays.editOverlay(this.laser, {
      visible: true
    });
    Overlays.editOverlay(this.dropLine, {
      visible: false
    });

    Audio.playSound(releaseSound, {
      position: this.entityProps.position,
      volume: 0.25
    });

    // only restore the original gravity if it's not zero.  This is to avoid...
    // 1. interface A grabs an entity and locally saves off its gravity
    // 2. interface A sets the entity's gravity to zero
    // 3. interface B grabs the entity and saves off its gravity (which is zero)
    // 4. interface A releases the entity and puts the original gravity back
    // 5. interface B releases the entity and puts the original gravity back (to zero)
    if(vectorIsZero(this.originalGravity)) {
      Entities.editEntity(this.grabbedEntity, {
        gravity: this.originalGravity
      });
    }
  }

  this.moveLaser = function() {
    var inverseRotation = Quat.inverse(MyAvatar.orientation);
    var startPosition = Vec3.multiplyQbyV(inverseRotation, Vec3.subtract(this.palmPosition, MyAvatar.position));
    // startPosition = Vec3.multiply(startPosition, 1 / MyAvatar.scale);
    var direction = Vec3.multiplyQbyV(inverseRotation, Vec3.subtract(this.tipPosition, this.palmPosition));
    direction = Vec3.multiply(direction, LASER_LENGTH_FACTOR / (Vec3.length(direction) * MyAvatar.scale));
    var endPosition = Vec3.sum(startPosition, direction);

    Overlays.editOverlay(this.laser, {
      start: startPosition,
      end: endPosition
    });

  }

  this.cleanup = function() {
    Overlays.deleteOverlay(this.laser);
    Overlays.deleteOverlay(this.dropLine);
  }
}

function update(deltaTime) {
  rightController.update(deltaTime);
  leftController.update(deltaTime);
}

function scriptEnding() {
  rightController.cleanup();
  leftController.cleanup();
}

function vectorIsZero(v) {
  return v.x === 0 && v.y === 0 && v.z === 0;
}

var rightController = new controller(RIGHT);
var leftController = new controller(LEFT);


Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);
