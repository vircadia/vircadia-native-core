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

// these are hand-measured bounds of the AirHockey table
var fieldMaxOffset = {
  x: 0.475,
  y: 0.315,
  z: 0.830
};
var fieldMinOffset = {
  x: -0.475,
  y: 0.315,
  z: -0.830
};

// parameters for storing the table playing field
var fieldMax = {
  x: 0,
  y: 0,
  z: 0
};
var fieldMin = {
  x: 0,
  y: 0,
  z: 0
};

var isGrabbing = false;
var grabbedEntity = null;
var prevMouse = {};
var deltaMouse = {
  z: 0
}

var TABLE_SEARCH_RANGE = 10;
var entityProps;
var moveUpDown = false;
var CLOSE_ENOUGH = 0.001;
var FULL_STRENGTH = 1.0;
var SPRING_RATE = 1.5;
var DAMPING_RATE = 0.80;
var ANGULAR_DAMPING_RATE = 0.40;
var SCREEN_TO_METERS = 0.001;
var currentPosition, currentVelocity, cameraEntityDistance, currentRotation;
var grabHeight;
var velocityTowardTarget, desiredVelocity, addedVelocity, newVelocity, dPosition, camYaw, distanceToTarget, targetPosition;
var originalGravity = {
  x: 0,
  y: 0,
  z: 0
};
var shouldRotate = false;
var dQ, theta, axisAngle, dT;
var angularVelocity = {
  x: 0,
  y: 0,
  z: 0
};

var grabSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/eric/sounds/CloseClamp.wav");
var releaseSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/eric/sounds/ReleaseClamp.wav");
var VOLUME = 0.0;

var DROP_DISTANCE = 0.10;
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
  prevMouse.x = event.x;
  prevMouse.y = event.y;

  var pickRay = Camera.computePickRay(event.x, event.y);
  var intersection = Entities.findRayIntersection(pickRay, true); // accurate picking
  if (!intersection.intersects) {
    return;
  }
  if (intersection.properties.collisionsWillMove) {
    grabbedEntity = intersection.entityID;
    var props = Entities.getEntityProperties(grabbedEntity)
    isGrabbing = true;
    originalGravity = props.gravity;
    targetPosition = props.position;
    currentPosition = props.position;
    currentVelocity = props.velocity;
    updateDropLine(targetPosition);

    // remember the height of the object when first grabbed
    // we'll try to maintain this height during the rest of this grab
    grabHeight = currentPosition.y;

    Entities.editEntity(grabbedEntity, {
      gravity: {
        x: 0,
        y: 0,
        z: 0
      }
    });

    Audio.playSound(grabSound, {
      position: props.position,
      volume: VOLUME
    });

    //We want to detect table once user grabs something that may be on a table...
    var potentialTables = Entities.findEntities(MyAvatar.position, TABLE_SEARCH_RANGE);
    potentialTables.forEach(function(table) {
      var props = Entities.getEntityProperties(table);
      // keep this name synchronized with what's in airHockey.js
      if (props.name === "air-hockey-table-23j4h1jh82jsjfw91jf232n2k") {
        var tablePosition = props.position;
        // when we know the table's position we can compute the X-Z bounds of its field
        fieldMax = Vec3.sum(tablePosition, fieldMaxOffset);
        fieldMin = Vec3.sum(tablePosition, fieldMinOffset);
      }
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

    Audio.playSound(releaseSound, {
      position: entityProps.position,
      volume: VOLUME
    });

  }
}

// new mouseMoveEvent
function mouseMoveEvent(event) {
  if (isGrabbing) {
    // see if something added/restored gravity
    var props = Entities.getEntityProperties(grabbedEntity);
    if (!vectorIsZero(props.gravity)) {
      originalGravity = props.gravity;
    }

    if (shouldRotate) {
      deltaMouse.x = event.x - prevMouse.x;
      if (!moveUpDown) {
        deltaMouse.z = event.y - prevMouse.y;
        deltaMouse.y = 0;
      } else {
        deltaMouse.y = (event.y - prevMouse.y) * -1;
        deltaMouse.z = 0;
      }
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
    } else {
      var relativePosition = Vec3.subtract(currentPosition, Camera.getPosition());
      if (relativePosition.y < 0) {
        // grabee is below camera, so movement is valid
        // compute intersectionPoint where mouse ray hits grabee's current x-z plane
        var pickRay = Camera.computePickRay(event.x, event.y);
        var mousePosition = pickRay.direction;
        var length = relativePosition.y / mousePosition.y;
        mousePosition = Vec3.multiply(mousePosition, length);
        mousePosition = Vec3.sum(Camera.getPosition(), mousePosition);

        // clamp mousePosition to table field
        if (mousePosition.x < fieldMin.x) {
          mousePosition.x = fieldMin.x;
        } else if (mousePosition.x > fieldMax.x) {
          mousePosition.x = fieldMax.x;
        }
        if (mousePosition.z < fieldMin.z) {
          mousePosition.z = fieldMin.z;
        } else if (mousePosition.z > fieldMax.z) {
          mousePosition.z = fieldMax.z;
        }
        mousePosition.y = grabHeight;
        targetPosition = mousePosition;
      }
    }
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
      newVelocity = {
        x: 0,
        y: 0,
        z: 0
      };
    }
    if (shouldRotate) {
      angularVelocity = Vec3.subtract(angularVelocity, Vec3.multiply(angularVelocity, ANGULAR_DAMPING_RATE));
    } else {
      angularVelocity = entityProps.angularVelocity;
    }

    // enforce that grabee's rotation is only about y-axis while being grabbed
    currentRotation.x = 0;
    currentRotation.z = 0;
    currentRotation.y = Math.sqrt(1.0 - currentRotation.w * currentRotation.w);
    // Hrm... slamming the currentRotation doesn't seem to work

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
