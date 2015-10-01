//
//  closePaint.js
//  examples
//
//  Created by Eric Levina on 9/30/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Run this script to be able to paint on entities you are close to, with hydras.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var RIGHT_HAND = 1;
var LEFT_HAND = 0;

var MIN_POINT_DISTANCE = 0.01;
var MAX_POINT_DISTANCE = 0.5;

var SPATIAL_CONTROLLERS_PER_PALM = 2;
var TIP_CONTROLLER_OFFSET = 1;

var TRIGGER_ON_VALUE = 0.3;

var MAX_DISTANCE = 10;

var STROKE_WIDTH = 0.02
var MAX_POINTS_PER_LINE = 40;

var RIGHT_4_ACTION = 18;
var RIGHT_2_ACTION = 16;

var LEFT_4_ACTION = 17;
var LEFT_2_ACTION = 16;

var HUE_INCREMENT = 0.01;


var center = Vec3.sum(MyAvatar.position, Vec3.multiply(2, Quat.getFront(Camera.getOrientation())));



function MyController(hand, triggerAction) {
  this.hand = hand;
  this.strokes = [];
  this.painting = false;

  if (this.hand === RIGHT_HAND) {
    this.getHandPosition = MyAvatar.getRightPalmPosition;
    this.getHandRotation = MyAvatar.getRightPalmRotation;
  } else {
    this.getHandPosition = MyAvatar.getLeftPalmPosition;
    this.getHandRotation = MyAvatar.getLeftPalmRotation;
  }

  this.triggerAction = triggerAction;
  this.palm = SPATIAL_CONTROLLERS_PER_PALM * hand;
  this.tip = SPATIAL_CONTROLLERS_PER_PALM * hand + TIP_CONTROLLER_OFFSET;


  this.strokeColor = {
    h: 0.8,
    s: 0.9,
    l: 0.4
  };

  this.laserPointer = Overlays.addOverlay("circle3d", {
    size: {
      x: STROKE_WIDTH / 2,
      y: STROKE_WIDTH / 2
    },
    color: hslToRgb(this.strokeColor),
    solid: true,
    position: center
  })
  this.triggerValue = 0;
  this.prevTriggerValue = 0;
  var _this = this;


  this.update = function() {
    this.updateControllerState()
    this.search();
    if (this.canPaint === true) {
      this.paint(this.intersection.intersection, this.intersection.surfaceNormal);
    }
  };

  this.paint = function(position, normal) {
    if (this.painting === false) {
      if (this.oldPosition) {
        this.newStroke(this.oldPosition);
      } else {
        this.newStroke(position);
      }
      this.painting = true;
    }



    var localPoint = Vec3.subtract(position, this.strokeBasePosition);
    //Move stroke a bit forward along normal so it doesnt zfight with mesh its drawing on 
    localPoint = Vec3.sum(localPoint, Vec3.multiply(normal, 0.001 + Math.random() * .001)); //rand avoid z fighting

    var distance = Vec3.distance(localPoint, this.strokePoints[this.strokePoints.length - 1]);
    if (this.strokePoints.length > 0 && distance < MIN_POINT_DISTANCE) {
      //need a minimum distance to avoid binormal NANs
      return;
    }
    if (this.strokePoints.length > 0 && distance > MAX_POINT_DISTANCE) {
      //Prevents drawing lines accross models
      this.painting = false;
      return;
    }
    if (this.strokePoints.length === 0) {
      localPoint = {
        x: 0,
        y: 0,
        z: 0
      };
    }

    this.strokePoints.push(localPoint);
    this.strokeNormals.push(normal);
    this.strokeWidths.push(STROKE_WIDTH);
    Entities.editEntity(this.currentStroke, {
      linePoints: this.strokePoints,
      normals: this.strokeNormals,
      strokeWidths: this.strokeWidths
    });
    if (this.strokePoints.length === MAX_POINTS_PER_LINE) {
      this.painting = false;
      return;
    }
    this.oldPosition = position
  }

  this.newStroke = function(position) {
    this.strokeBasePosition = position;
    this.currentStroke = Entities.addEntity({
      position: position,
      type: "PolyLine",
      color: hslToRgb(this.strokeColor),
      dimensions: {
        x: 50,
        y: 50,
        z: 50
      },
      lifetime: 100
    });
    this.strokePoints = [];
    this.strokeNormals = [];
    this.strokeWidths = [];

    this.strokes.push(this.currentStroke);

  }

  this.updateControllerState = function() {
    var triggerValue = Controller.getActionValue(this.triggerAction);
    if (triggerValue > TRIGGER_ON_VALUE && this.prevTriggerValue <= TRIGGER_ON_VALUE) {
      this.squeeze();
    } else if (triggerValue < TRIGGER_ON_VALUE && this.prevTriggerValue >= TRIGGER_ON_VALUE) {
      this.release()
    }

    this.prevTriggerValue = triggerValue;
  }

  this.squeeze = function() {
    this.tryPainting = true;

  }
  this.release = function() {
    this.painting = false;
    this.tryPainting = false;
    this.canPaint = false;
    this.oldPosition = null;
  }
  this.search = function() {

    // the trigger is being pressed, do a ray test
    var handPosition = this.getHandPosition();
    var pickRay = {
      origin: handPosition,
      direction: Quat.getUp(this.getHandRotation())
    };


    this.intersection = Entities.findRayIntersection(pickRay, true);
    if (this.intersection.intersects) {
      var distance = Vec3.distance(handPosition, this.intersection.intersection);
      if (distance < MAX_DISTANCE) {
        var displayPoint = this.intersection.intersection;
        displayPoint = Vec3.sum(displayPoint, Vec3.multiply(this.intersection.surfaceNormal, .001));
        if (this.tryPainting) {
          this.canPaint = true;
        }
        Overlays.editOverlay(this.laserPointer, {
          visible: true,
          position: displayPoint,
          rotation: orientationOf(this.intersection.surfaceNormal)
        });

      } else {
        this.hitFail();
      }
    } else {
      this.hitFail();
    }
  };

  this.hitFail = function() {
    this.canPaint = false;

    Overlays.editOverlay(this.laserPointer, {
      visible: false
    });

  }

  this.cleanup = function() {
    Overlays.deleteOverlay(this.laserPointer);
    this.strokes.forEach(function(stroke) {
      Entities.deleteEntity(stroke);
    });
  }

  this.cycleColorDown = function() {
    this.strokeColor.h -= HUE_INCREMENT;
    if (this.strokeColor.h < 0) {
      this.strokeColor = 1;
    }
  }

  this.cycleColorUp = function() {
    this.strokeColor.h += HUE_INCREMENT;
    if (this.strokeColor.h > 1) {
      this.strokeColor.h = 0;
    }
  }
}

var rightController = new MyController(RIGHT_HAND, Controller.findAction("RIGHT_HAND_CLICK"));
var leftController = new MyController(LEFT_HAND, Controller.findAction("LEFT_HAND_CLICK"));

Controller.actionEvent.connect(function(action, state) {
  if (state === 0) {
    return;
  }
  if (action === RIGHT_4_ACTION) {
    rightController.cycleColorUp();
  } else if (action === RIGHT_2_ACTION) {
    rightController.cycleColorDown();
  }
  if (action === LEFT_4_ACTION) {
    leftController.cycleColorUp();
  } else if (action === LEFT_2_ACTION) {
    leftController.cycleColorDown();
  }
});

function update() {
  rightController.update();
  leftController.update();
}

function cleanup() {
  rightController.cleanup();
  leftController.cleanup();
}

Script.scriptEnding.connect(cleanup);
Script.update.connect(update);


function orientationOf(vector) {
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

  var theta = 0.0;

  var RAD_TO_DEG = 180.0 / Math.PI;
  var direction, yaw, pitch;
  direction = Vec3.normalize(vector);
  yaw = Quat.angleAxis(Math.atan2(direction.x, direction.z) * RAD_TO_DEG, Y_AXIS);
  pitch = Quat.angleAxis(Math.asin(-direction.y) * RAD_TO_DEG, X_AXIS);
  return Quat.multiply(yaw, pitch);
}

/**
 * Converts an HSL color value to RGB. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
 * Assumes h, s, and l are contained in the set [0, 1] and
 * returns r, g, and b in the set [0, 255].
 *
 * @param   Number  h       The hue
 * @param   Number  s       The saturation
 * @param   Number  l       The lightness
 * @return  Array           The RGB representation
 */
function hslToRgb(hsl) {
  var r, g, b;

  if (hsl.s == 0) {
    r = g = b = hsl.l; // achromatic
  } else {
    var hue2rgb = function hue2rgb(p, q, t) {
      if (t < 0) t += 1;
      if (t > 1) t -= 1;
      if (t < 1 / 6) return p + (q - p) * 6 * t;
      if (t < 1 / 2) return q;
      if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
      return p;
    }

    var q = hsl.l < 0.5 ? hsl.l * (1 + hsl.s) : hsl.l + hsl.s - hsl.l * hsl.s;
    var p = 2 * hsl.l - q;
    r = hue2rgb(p, q, hsl.h + 1 / 3);
    g = hue2rgb(p, q, hsl.h);
    b = hue2rgb(p, q, hsl.h - 1 / 3);
  }

  return {
    red: Math.round(r * 255),
    green: Math.round(g * 255),
    blue: Math.round(b * 255)
  };
}