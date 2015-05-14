
//  grab.js
//  examples
//
//  Created by Eric Levin on May 1, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Grab's physically moveable entities with the mouse, by applying a spring force. 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var isGrabbing = false;
var grabbedEntity = null;
var lineEntityID = null;
var prevMouse = {};
var deltaMouse = {
  z: 0
}
var entityProps;
var moveUpDown = false;
var CLOSE_ENOUGH = 0.001;
var FULL_STRENGTH = 1.0;
var SPRING_RATE = 1.5;
var DAMPING_RATE = 0.80;
var ANGULAR_DAMPING_RATE = 0.40;
var SCREEN_TO_METERS = 0.001;
var currentPosition, currentVelocity, cameraEntityDistance, currentRotation;
var velocityTowardTarget, desiredVelocity, addedVelocity, newVelocity, dPosition, camYaw, distanceToTarget, targetPosition;
var originalGravity = {x: 0, y: 0, z: 0};
var shouldRotate = false;
var dQ, theta, axisAngle, dT;
var angularVelocity = {
  x: 0,
  y: 0,
  z: 0
};

var grabSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/eric/sounds/CloseClamp.wav");
var releaseSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/eric/sounds/ReleaseClamp.wav");
var VOLUME = 0.10;

var DROP_DISTANCE = 5.0;
var DROP_COLOR = {
  red: 200,
  green: 200,
  blue: 200
};
var DROP_WIDTH = 2;


var dropLine = Overlays.addOverlay("line3d", {
  color: DROP_COLOR,
  alpha: 1,
  visible: false,
  lineWidth: DROP_WIDTH
});


function vectorIsZero(v) {
    return v.x == 0 && v.y == 0 && v.z == 0;
}

function nearLinePoint(targetPosition) {
 // var handPosition = Vec3.sum(MyAvatar.position, {x:0, y:0.2, z:0});
  var handPosition = MyAvatar.getRightPalmPosition();
  var along = Vec3.subtract(targetPosition, handPosition);
  along = Vec3.normalize(along);
  along = Vec3.multiply(along, 0.4);
  return Vec3.sum(handPosition, along);
}


function mousePressEvent(event) {
  if (!event.isLeftButton) {
    return;
  }
  var pickRay = Camera.computePickRay(event.x, event.y);
  var intersection = Entities.findRayIntersection(pickRay, true); // accurate picking
  if (intersection.intersects && intersection.properties.collisionsWillMove) {
    grabbedEntity = intersection.entityID;
    var props = Entities.getEntityProperties(grabbedEntity)
    isGrabbing = true;
    originalGravity = props.gravity;
    targetPosition = props.position;
    currentPosition = props.position;
    currentVelocity = props.velocity;
    updateDropLine(targetPosition);

    Entities.editEntity(grabbedEntity, {
      gravity: {x: 0, y: 0, z: 0}
    });

    lineEntityID = Entities.addEntity({
      type: "Line",
      position: nearLinePoint(targetPosition),
      dimensions: Vec3.subtract(targetPosition, nearLinePoint(targetPosition)),
      color: { red: 255, green: 255, blue: 255 },
      lifetime: 300 // if someone crashes while moving something, don't leave the line there forever.
    });

    Audio.playSound(grabSound, {
      position: props.position,
      volume: VOLUME
    });
  }
}

function updateDropLine(position) {
  Overlays.editOverlay(dropLine, {
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
  })
}


function mouseReleaseEvent() {
  if (isGrabbing) {
    isGrabbing = false;

    // only restore the original gravity if it's not zero.  This is to avoid...
    // 1. interface A grabs an entity and locally saves off its gravity
    // 2. interface A sets the entity's gravity to zero
    // 3. interface B grabs the entity and saves off its gravity (which is zero)
    // 4. interface A releases the entity and puts the original gravity back
    // 5. interface B releases the entity and puts the original gravity back (to zero)
    if (!vectorIsZero(originalGravity)) {
      Entities.editEntity(grabbedEntity, {
          gravity: originalGravity
      });
    }

    Overlays.editOverlay(dropLine, {
      visible: false
    });
    targetPosition = null;

    Entities.deleteEntity(lineEntityID);

    Audio.playSound(releaseSound, {
      position: entityProps.position,
      volume: VOLUME
    });

  }
}

function mouseMoveEvent(event) {
  if (isGrabbing) {
    // see if something added/restored gravity
    var props = Entities.getEntityProperties(grabbedEntity);
    if (!vectorIsZero(props.gravity)) {
      originalGravity = props.gravity;
    }

    deltaMouse.x = event.x - prevMouse.x;
    if (!moveUpDown) {
      deltaMouse.z = event.y - prevMouse.y;
      deltaMouse.y = 0;
    } else {
      deltaMouse.y = (event.y - prevMouse.y) * -1;
      deltaMouse.z = 0;
    }
    //  Update the target position by the amount the mouse moved
    camYaw = Quat.safeEulerAngles(Camera.getOrientation()).y;
    dPosition = Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(0, camYaw, 0), deltaMouse);
    if (!shouldRotate) {
      //  Adjust target position for the object by the mouse move 
      cameraEntityDistance = Vec3.distance(Camera.getPosition(), currentPosition);
      //  Scale distance we want to move by the distance from the camera to the grabbed object 
      //  TODO:  Correct SCREEN_TO_METERS to be correct for the actual FOV, resolution
      targetPosition = Vec3.sum(targetPosition, Vec3.multiply(dPosition, cameraEntityDistance * SCREEN_TO_METERS));
    } else if (shouldRotate) {
      var transformedDeltaMouse = {
        x: deltaMouse.z,
        y: deltaMouse.x,
        z: deltaMouse.y
      };
      transformedDeltaMouse = Vec3.multiplyQbyV(Quat.fromPitchYawRollDegrees(0, camYaw, 0), transformedDeltaMouse);
      dQ = Quat.fromVec3Degrees(transformedDeltaMouse);
      theta = 2 * Math.acos(dQ.w);
      axisAngle = Quat.axis(dQ);
      angularVelocity = Vec3.multiply((theta / dT), axisAngle);
    }

    Entities.editEntity(lineEntityID, {
      position: nearLinePoint(targetPosition),
      dimensions: Vec3.subtract(targetPosition, nearLinePoint(targetPosition))
    });
  }
  prevMouse.x = event.x;
  prevMouse.y = event.y;

}


function keyReleaseEvent(event) {
  if (event.text === "SHIFT") {
    moveUpDown = false;
  }
  if (event.text === "SPACE") {
    shouldRotate = false;
  }
}

function keyPressEvent(event) {
  if (event.text === "SHIFT") {
    moveUpDown = true;
  }
  if (event.text === "SPACE") {
    shouldRotate = true;
  }
}

function update(deltaTime) {
  dT = deltaTime;
  if (isGrabbing) {

    entityProps = Entities.getEntityProperties(grabbedEntity);
    currentPosition = entityProps.position;
    currentVelocity = entityProps.velocity;
    currentRotation = entityProps.rotation;

    var dPosition = Vec3.subtract(targetPosition, currentPosition);

    distanceToTarget = Vec3.length(dPosition);
    if (distanceToTarget > CLOSE_ENOUGH) {
      //  compute current velocity in the direction we want to move 
      velocityTowardTarget = Vec3.dot(currentVelocity, Vec3.normalize(dPosition));
      velocityTowardTarget = Vec3.multiply(Vec3.normalize(dPosition), velocityTowardTarget);
      //  compute the speed we would like to be going toward the target position 

      desiredVelocity = Vec3.multiply(dPosition, (1.0 / deltaTime) * SPRING_RATE);
      //  compute how much we want to add to the existing velocity
      addedVelocity = Vec3.subtract(desiredVelocity, velocityTowardTarget);
      //  If target is too far, roll off the force as inverse square of distance
      if (distanceToTarget / cameraEntityDistance > FULL_STRENGTH) {
        addedVelocity = Vec3.multiply(addedVelocity, Math.pow(FULL_STRENGTH / distanceToTarget, 2.0));
      }
      newVelocity = Vec3.sum(currentVelocity, addedVelocity);
      //  Add Damping 
      newVelocity = Vec3.subtract(newVelocity, Vec3.multiply(newVelocity, DAMPING_RATE));
      //  Update entity
    } else {
      newVelocity = {x: 0, y: 0, z: 0};
    }
    if (shouldRotate) {
      angularVelocity = Vec3.subtract(angularVelocity, Vec3.multiply(angularVelocity, ANGULAR_DAMPING_RATE));
    } else {
      angularVelocity = entityProps.angularVelocity; 
    }

    Entities.editEntity(grabbedEntity, {
      position: currentPosition,
      rotation: currentRotation,
      velocity: newVelocity,
      angularVelocity: angularVelocity
    });
    updateDropLine(targetPosition);
  }
}

Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
Script.update.connect(update);
