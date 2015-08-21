//
//  inspect.js
//  examples
//
//  Created by Clément Brisset on March 20, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Allows you to inspect non moving objects (Voxels or Avatars) using Atl, Control (Command on Mac) and Shift
//
//  radial mode = hold ALT
//  orbit mode  = hold ALT + CONTROL
//  pan mode    = hold ALT + CONTROL + SHIFT
//  Once you are in a mode left click on the object to inspect and hold the click
//  Dragging the mouse will move your camera according to the mode you are in.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var PI = Math.PI;
var RAD_TO_DEG = 180.0 / PI;

var AZIMUTH_RATE = 90.0;
var ALTITUDE_RATE = 200.0;
var RADIUS_RATE = 1.0 / 100.0;
var PAN_RATE = 250.0;

var Y_AXIS = {
  x: 0,
  y: 1,
  z: 0
};
var X_AXIS = {
  x: 1,
  y: 0,
  z: 0
};

var LOOK_AT_TIME = 500;

var alt = false;
var shift = false;
var control = false;

var isActive = false;

var oldMode = Camera.mode;
var noMode = 0;
var orbitMode = 1;
var radialMode = 2;
var panningMode = 3;
var detachedMode = 4;

var mode = noMode;

var mouseLastX = 0;
var mouseLastY = 0;


var center = {
  x: 0,
  y: 0,
  z: 0
};
var position = {
  x: 0,
  y: 0,
  z: 0
};
var vector = {
  x: 0,
  y: 0,
  z: 0
};
var radius = 0.0;
var azimuth = 0.0;
var altitude = 0.0;

var avatarPosition;
var avatarOrientation;

var rotatingTowardsTarget = false;
var targetCamOrientation;
var oldPosition, oldOrientation;


function orientationOf(vector) {
  var direction,
    yaw,
    pitch;

  direction = Vec3.normalize(vector);
  yaw = Quat.angleAxis(Math.atan2(direction.x, direction.z) * RAD_TO_DEG, Y_AXIS);
  pitch = Quat.angleAxis(Math.asin(-direction.y) * RAD_TO_DEG, X_AXIS);
  return Quat.multiply(yaw, pitch);
}


function handleRadialMode(dx, dy) {
  azimuth += dx / AZIMUTH_RATE;
  radius += radius * dy * RADIUS_RATE;
  if (radius < 1) {
    radius = 1;
  }

  vector = {
    x: (Math.cos(altitude) * Math.cos(azimuth)) * radius,
    y: Math.sin(altitude) * radius,
    z: (Math.cos(altitude) * Math.sin(azimuth)) * radius
  };
  position = Vec3.sum(center, vector);
  Camera.setPosition(position);
  Camera.setOrientation(orientationOf(vector));
}

function handleOrbitMode(dx, dy) {
  azimuth += dx / AZIMUTH_RATE;
  altitude += dy / ALTITUDE_RATE;
  if (altitude > PI / 2.0) {
    altitude = PI / 2.0;
  }
  if (altitude < -PI / 2.0) {
    altitude = -PI / 2.0;
  }

  vector = {
    x: (Math.cos(altitude) * Math.cos(azimuth)) * radius,
    y: Math.sin(altitude) * radius,
    z: (Math.cos(altitude) * Math.sin(azimuth)) * radius
  };
  position = Vec3.sum(center, vector);
  Camera.setPosition(position);
  Camera.setOrientation(orientationOf(vector));
}


function handlePanMode(dx, dy) {
  var up = Quat.getUp(Camera.getOrientation());
  var right = Quat.getRight(Camera.getOrientation());
  var distance = Vec3.length(vector);

  var dv = Vec3.sum(Vec3.multiply(up, distance * dy / PAN_RATE), Vec3.multiply(right, -distance * dx / PAN_RATE));

  center = Vec3.sum(center, dv);
  position = Vec3.sum(position, dv);

  Camera.setPosition(position);
  Camera.setOrientation(orientationOf(vector));
}

function saveCameraState() {
  oldMode = Camera.mode;
  oldPosition = Camera.getPosition();
  oldOrientation = Camera.getOrientation();

  Camera.mode = "independent";
  Camera.setPosition(oldPosition);
}

function restoreCameraState() {
  Camera.mode = oldMode;
  Camera.setPosition(oldPosition);
  Camera.setOrientation(oldOrientation);
}

function handleModes() {
  var newMode = (mode == noMode) ? noMode : detachedMode;
  if (alt) {
    if (control) {
      if (shift) {
        newMode = panningMode;
      } else {
        newMode = orbitMode;
      }
    } else {
      newMode = radialMode;
    }
  }

  // if entering detachMode
  if (newMode == detachedMode && mode != detachedMode) {
    avatarPosition = MyAvatar.position;
    avatarOrientation = MyAvatar.orientation;
  }
  // if leaving detachMode
  if (mode == detachedMode && newMode == detachedMode &&
    (avatarPosition.x != MyAvatar.position.x ||
      avatarPosition.y != MyAvatar.position.y ||
      avatarPosition.z != MyAvatar.position.z ||
      avatarOrientation.x != MyAvatar.orientation.x ||
      avatarOrientation.y != MyAvatar.orientation.y ||
      avatarOrientation.z != MyAvatar.orientation.z ||
      avatarOrientation.w != MyAvatar.orientation.w)) {
    newMode = noMode;
  }

  if (mode == noMode && newMode != noMode && Camera.mode == "independent") {
    newMode = noMode;
  }

  // if leaving noMode
  if (mode == noMode && newMode != noMode) {
    saveCameraState();
  }
  // if entering noMode
  if (newMode == noMode && mode != noMode) {
    restoreCameraState();
  }

  mode = newMode;
}

function keyPressEvent(event) {
  var changed = false;

  if (event.text == "ALT") {
    alt = true;
    changed = true;
  }
  if (event.text == "CONTROL") {
    control = true;
    changed = true;
  }
  if (event.text == "SHIFT") {
    shift = true;
    changed = true;
  }

  if (changed) {
    handleModes();
  }
}

function keyReleaseEvent(event) {
  var changed = false;

  if (event.text == "ALT") {
    alt = false;
    changed = true;
    mode = noMode;
    restoreCameraState();
  }
  if (event.text == "CONTROL") {
    control = false;
    changed = true;
  }
  if (event.text == "SHIFT") {
    shift = false;
    changed = true;
  }

  if (changed) {
    handleModes();
  }
}



function mousePressEvent(event) {
  if (alt && !isActive) {
    mouseLastX = event.x;
    mouseLastY = event.y;

    // Compute trajectories related values
    var pickRay = Camera.computePickRay(mouseLastX, mouseLastY);
    var modelIntersection = Entities.findRayIntersection(pickRay, true);

    position = Camera.getPosition();

    var avatarTarget = MyAvatar.getTargetAvatarPosition();


    var distance = -1;
    var string;

    if (modelIntersection.intersects && modelIntersection.accurate) {
      distance = modelIntersection.distance;
      center = modelIntersection.intersection;
      string = "Inspecting model";
      //We've selected our target, now orbit towards it automatically
      rotatingTowardsTarget = true;
      //calculate our target cam rotation
      Script.setTimeout(function() {
        rotatingTowardsTarget = false;
      }, LOOK_AT_TIME);

      vector = Vec3.subtract(position, center);
      targetCamOrientation = orientationOf(vector);
      radius = Vec3.length(vector);
      azimuth = Math.atan2(vector.z, vector.x);
      altitude = Math.asin(vector.y / Vec3.length(vector));

      isActive = true;
    }

  }
}

function mouseReleaseEvent(event) {
  if (isActive) {
    isActive = false;
  }
}

function mouseMoveEvent(event) {
  if (isActive && mode != noMode && !rotatingTowardsTarget) {
    if (mode == radialMode) {
      handleRadialMode(event.x - mouseLastX, event.y - mouseLastY);
    }
    if (mode == orbitMode) {
      handleOrbitMode(event.x - mouseLastX, event.y - mouseLastY);
    }
    if (mode == panningMode) {
      handlePanMode(event.x - mouseLastX, event.y - mouseLastY);
    }

  }
  mouseLastX = event.x;
  mouseLastY = event.y;
}

function update() {
  handleModes();
  if (rotatingTowardsTarget) {
    rotateTowardsTarget();
  }
}

function rotateTowardsTarget() {
  var newOrientation = Quat.mix(Camera.getOrientation(), targetCamOrientation, .1);
  Camera.setOrientation(newOrientation);
}

function scriptEnding() {
  if (mode != noMode) {
    restoreCameraState();
  }
}

Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);

Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);

Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);