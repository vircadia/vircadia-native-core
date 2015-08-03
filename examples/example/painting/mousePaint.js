//
//  mousePaint.js
//  examples
//
//  Created by Eric Levin on 6/4/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  This script allows you to paint with the hydra or mouse!
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var LINE_DIMENSIONS = 10;
var LIFETIME = 6000;
var EVENT_CHANGE_THRESHOLD = 200;
var LINE_WIDTH = .07;
var MAX_POINTS_PER_LINE = 40;
var points = [];
var normals = [];
var deletedLines = [];
var strokeWidths = [];
var count = 0;
var prevEvent = {x: 0, y: 0};
var eventChange;

var MIN_POINT_DISTANCE = .01;

var colorPalette = [{
    red: 250,
    green: 0,
    blue: 0
}, {
    red: 214,
    green: 91,
    blue: 67
}, {
    red: 192,
    green: 41,
    blue: 66
}, {
    red: 84,
    green: 36,
    blue: 55
}, {
    red: 83,
    green: 119,
    blue: 122
}];

var currentColorIndex = 0;
var currentColor = colorPalette[currentColorIndex];

function cycleColor() {
  currentColor = colorPalette[++currentColorIndex];
  if (currentColorIndex === colorPalette.length - 1) {
    currentColorIndex = -1;
  }

}

MousePaint();

function MousePaint() {
  var DRAWING_DISTANCE = 5;
  var lines = [];
  var isDrawing = false;

  var line, linePosition;

  var BRUSH_SIZE = .05;

  var brush = Entities.addEntity({
    type: 'Sphere',
    position: {
      x: 0,
      y: 0,
      z: 0
    },
    color: currentColor,
    dimensions: {
      x: BRUSH_SIZE,
      y: BRUSH_SIZE,
      z: BRUSH_SIZE
    }
  });


  function newLine(position) {
    linePosition = position;
    line = Entities.addEntity({
      position: position,
      type: "PolyLine",
      color: currentColor,
      dimensions: {
        x: LINE_DIMENSIONS,
        y: LINE_DIMENSIONS,
        z: LINE_DIMENSIONS
      },
      linePoints: [],
      lifetime: LIFETIME
    });
    points = [];
    normals = []
    strokeWidths = [];
    lines.push(line);
  }



  function mouseMoveEvent(event) {

    var pickRay = Camera.computePickRay(event.x, event.y);
    count++;
    var worldPoint = computeWorldPoint(pickRay);
    Entities.editEntity(brush, {
      position: worldPoint
    });

    eventChange = Math.sqrt(Math.pow(event.x - prevEvent.x, 2) + Math.pow(event.y - prevEvent.y, 2));
    localPoint = computeLocalPoint(worldPoint);
    if (!isDrawing || points.length > MAX_POINTS_PER_LINE || eventChange > EVENT_CHANGE_THRESHOLD || 
      Vec3.distance(points[points.length - 1], localPoint) < MIN_POINT_DISTANCE) {
      return;
    }   

    points.push(localPoint)
    normals.push(computeNormal(worldPoint, pickRay.origin));
    strokeWidths.push(LINE_WIDTH);
    Entities.editEntity(line, {
      strokeWidths: strokeWidths,
      linePoints: points,
      normals: normals,
    });
    prevEvent = event;
  }

  function computeNormal(p1, p2) {
    return Vec3.normalize(Vec3.subtract(p2, p1));
  }

  function computeWorldPoint(pickRay) {
    var addVector = Vec3.multiply(Vec3.normalize(pickRay.direction), DRAWING_DISTANCE);
    return Vec3.sum(pickRay.origin, addVector);
  }

  function computeLocalPoint(worldPoint) {
    var localPoint = Vec3.subtract(worldPoint, linePosition);
    return localPoint;
  }

  function mousePressEvent(event) {
    if (!event.isLeftButton) {
      isDrawing = false;
      return;
    }
    var pickRay = Camera.computePickRay(event.x, event.y);
    prevEvent = {x: event.x, y:event.y};
    var worldPoint = computeWorldPoint(pickRay);
    newLine(worldPoint);
    var localPoint = computeLocalPoint(worldPoint);
    points.push(localPoint);
    normals.push(computeNormal(worldPoint, pickRay.origin));
    strokeWidths.push(0.07);
    isDrawing = true;
  }

  function mouseReleaseEvent() {
    isDrawing = false;
  }

  function keyPressEvent(event) {
    if (event.text === "SPACE") {
      cycleColor();
      Entities.editEntity(brush, {
        color: currentColor
      });
    }
  }

  function cleanup() {
    lines.forEach(function(line) {
      // Entities.deleteEntity(line);
    });
    Entities.deleteEntity(brush);
  }

  Controller.mousePressEvent.connect(mousePressEvent);
  Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
  Controller.mouseMoveEvent.connect(mouseMoveEvent);
  Script.scriptEnding.connect(cleanup);

  Controller.keyPressEvent.connect(keyPressEvent);
}
