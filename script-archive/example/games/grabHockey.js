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
  z: 0.82
};
var fieldMinOffset = {
  x: -0.460, // yes, smaller than max
  y: 0.315,
  z: -0.830
};
var halfCornerBoxWidth = 0.84;

var tablePosition = {
  x: 0,
  y: 0,
  z: 0
}
var isGrabbing = false;
var isGrabbingPaddle = false;
var grabbedEntity = null;
var prevMouse = {x: 0, y: 0};
var deltaMouse = {
  z: 0
}

var MAX_GRAB_DISTANCE = 100;
var TABLE_SEARCH_RANGE = 10;
var entityProps;
var moveUpDown = false;
var CLOSE_ENOUGH = 0.001;
var FULL_STRENGTH = 1.0;
var SPRING_TIMESCALE = 0.05;
var ANGULAR_SPRING_TIMESCALE = 0.03;
var DAMPING_RATE = 0.80;
var ANGULAR_DAMPING_RATE = 0.40;
var SCREEN_TO_METERS = 0.001;
var currentPosition, currentVelocity, cameraEntityDistance, currentRotation;
var grabHeight;
var initialVerticalGrabPosition;
var MAX_VERTICAL_ANGLE = Math.PI / 3;
var MIN_VERTICAL_ANGLE = - MAX_VERTICAL_ANGLE;
var velocityTowardTarget, desiredVelocity, addedVelocity, newVelocity, dPosition, camYaw, distanceToTarget, targetPosition;
var grabOffset;
var originalGravity = {
  x: 0,
  y: 0,
  z: 0
};
var shouldRotate = false;
var dQ, dT;
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

function xzPickRayIntersetion(pointOnPlane, mouseX, mouseY) {
  var relativePoint = Vec3.subtract(pointOnPlane, Camera.getPosition());
  var pickRay = Camera.computePickRay(mouseX, mouseY);
  if (Math.abs(pickRay.direction.y) > 0.001) {
    var distance = relativePoint.y / pickRay.direction.y;
    if (distance < 0.001) {
        return pointOnPlane;
    }
    var pickInersection = Vec3.multiply(pickRay.direction, distance);
    pickInersection = Vec3.sum(Camera.getPosition(), pickInersection);
    return pickInersection;
  }
  // point and line are more-or-less co-planar: compute closest approach of pickRay and pointOnPlane
  var distance = Vec3.dot(relativePoint, pickRay.direction);
  var pickInersection = Vec3.multiply(pickRay.direction, distance);
  return pickInersection;
}

function forwardPickRayIntersection(pointOnPlane, mouseX, mouseY) {
  var relativePoint = Vec3.subtract(pointOnPlane, Camera.getPosition());
  var pickRay = Camera.computePickRay(mouseX, mouseY);
  var planeNormal = Quat.getFront(Camera.getOrientation());
  var distance = Vec3.dot(planeNormal, relativePoint);
  var rayDistance = Vec3.dot(planeNormal, pickRay.direction);
  var pickIntersection = Vec3.multiply(pickRay.direction, distance / rayDistance);
  pickIntersection = Vec3.sum(pickIntersection, Camera.getPosition())
  return pickIntersection;
}

function mousePressEvent(event) {
  if (!event.isLeftButton) {
    return;
  }
  prevMouse.x = event.x;
  prevMouse.y = event.y;

  var pickRay = Camera.computePickRay(event.x, event.y);
  var pickResults = Entities.findRayIntersection(pickRay, true); // accurate picking
  if (!pickResults.intersects) {
    return;
  }
  var isDynamic = Entites.getEntityProperties(pickResults.entityID, "dynamic").dynamic;
  if (isDynamic) {
    grabbedEntity = pickResults.entityID;
    var props = Entities.getEntityProperties(grabbedEntity)
    originalGravity = props.gravity;
    var objectPosition = props.position;
    currentPosition = props.position;
    if (Vec3.distance(currentPosition, Camera.getPosition()) > MAX_GRAB_DISTANCE) {
        // don't allow grabs of things far away
        return;
    }

    isGrabbing = true;
    isGrabbingPaddle = (props.name == "air-hockey-paddle-23j4h1jh82jsjfw91jf232n2k");

    currentVelocity = props.velocity;
    updateDropLine(objectPosition);

    var pointOnPlane = xzPickRayIntersetion(objectPosition, event.x, event.y);
    grabOffset = Vec3.subtract(pointOnPlane, objectPosition);
    targetPosition = objectPosition;

    // remember the height of the object when first grabbed
    // we'll try to maintain this height during the rest of this grab
    grabHeight = currentPosition.y;
    initialVerticalGrabPosition = currentPosition;

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
        // need to remember the table's position so we can clamp the targetPositon
        // to remain on the playing field
        tablePosition = props.position;
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

function mouseMoveEvent(event) {
  if (isGrabbing) {
    // see if something added/restored gravity
    var props = Entities.getEntityProperties(grabbedEntity);
    if (!vectorIsZero(props.gravity)) {
      originalGravity = props.gravity;
    }

    if (shouldRotate) {
      targetPosition = currentPosition;
      deltaMouse.x = event.x - prevMouse.x;
      deltaMouse.z = event.y - prevMouse.y;
      deltaMouse.y = 0;

      var transformedDeltaMouse = {
        x: deltaMouse.z,
        y: deltaMouse.x,
        z: deltaMouse.y
      };
      dQ = Quat.fromVec3Degrees(transformedDeltaMouse);
      var theta = 2 * Math.acos(dQ.w);
      var axis = Quat.axis(dQ);
      angularVelocity = Vec3.multiply((theta / ANGULAR_SPRING_TIMESCALE), axis);
    } else {
      if (moveUpDown) {
        targetPosition = forwardPickRayIntersection(currentPosition, event.x, event.y);
        grabHeight = targetPosition.y;
      } else {
        var pointOnPlane = xzPickRayIntersetion(currentPosition, event.x, event.y);
        pointOnPlane = Vec3.subtract(pointOnPlane, grabOffset);  

        if (isGrabbingPaddle) {
          // translate pointOnPlane into local-frame
          pointOnPlane = Vec3.subtract(pointOnPlane, tablePosition);

          // clamp local pointOnPlane to table field
          if (pointOnPlane.x > fieldMaxOffset.x) {
            pointOnPlane.x = fieldMaxOffset.x;
          } else if (pointOnPlane.x < fieldMinOffset.x) {
            pointOnPlane.x = fieldMinOffset.x;
          }
          if (pointOnPlane.z > fieldMaxOffset.z) {
            pointOnPlane.z = fieldMaxOffset.z;
          } else if (pointOnPlane.z < fieldMinOffset.z) {
            pointOnPlane.z = fieldMinOffset.z;
          }
  
          // clamp to rotated square (for cut corners)
          var rotation = Quat.angleAxis(45, { x:0, y:1, z:0 });
          pointOnPlane = Vec3.multiplyQbyV(rotation, pointOnPlane);
          if (pointOnPlane.x > halfCornerBoxWidth) {
              pointOnPlane.x = halfCornerBoxWidth;
          } else if (pointOnPlane.x < -halfCornerBoxWidth) {
              pointOnPlane.x = -halfCornerBoxWidth;
          }
          if (pointOnPlane.z > halfCornerBoxWidth) {
              pointOnPlane.z = halfCornerBoxWidth;
          } else if (pointOnPlane.z < -halfCornerBoxWidth) {
              pointOnPlane.z = -halfCornerBoxWidth;
          }
          // rotate back into local frame
          rotation.y = -rotation.y;
          pointOnPlane = Vec3.multiplyQbyV(rotation, pointOnPlane);
  
          // translate into world-frame
          pointOnPlane = Vec3.sum(tablePosition, pointOnPlane);
        } else {
          // we're grabbing a non-paddle object
  
          // clamp distance
          var relativePosition = Vec3.subtract(pointOnPlane, Camera.getPosition());
          var length = Vec3.length(relativePosition);
          if (length > MAX_GRAB_DISTANCE) {
            relativePosition = Vec3.multiply(relativePosition, MAX_GRAB_DISTANCE / length);
            pointOnPlane = Vec3.sum(relativePosition, Camera.getPosition());
          }
        }
        // clamp to grabHeight
        pointOnPlane.y = grabHeight;
        targetPosition = pointOnPlane;
        initialVerticalGrabPosition = targetPosition;
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
    if (shouldRotate) {
      angularVelocity = Vec3.subtract(angularVelocity, Vec3.multiply(angularVelocity, ANGULAR_DAMPING_RATE));
      Entities.editEntity(grabbedEntity, { angularVelocity: angularVelocity, });
    } else {
      var dPosition = Vec3.subtract(targetPosition, currentPosition);
      var delta = Vec3.length(dPosition);
      if (delta > CLOSE_ENOUGH) {
        var MAX_POSITION_DELTA = 0.50;
        if (delta > MAX_POSITION_DELTA) {
            dPosition = Vec3.multiply(dPosition, MAX_POSITION_DELTA / delta);
        }
        // desired speed is proportional to displacement by the inverse of timescale
        // (for critically damped motion)
        newVelocity = Vec3.multiply(dPosition, 1.0 / SPRING_TIMESCALE);
      } else {
        newVelocity = {
          x: 0,
          y: 0,
          z: 0
        };
      }
      Entities.editEntity(grabbedEntity, { velocity: newVelocity, });
    }

    updateDropLine(targetPosition);
  }
}

Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
Script.update.connect(update);
