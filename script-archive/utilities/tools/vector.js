//
//  vector.js
//  examples
//
//  Created by Bridget Went on 7/1/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  A template for creating vector arrows using line entities. A VectorArrow object creates a 
//  draggable vector arrow where the user clicked at a specified distance from the viewer.
//  The relative magnitude and direction of the vector may be displayed. 
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//

var LINE_DIMENSIONS = 100;
var LIFETIME = 6000;
var RAD_TO_DEG = 180.0 / Math.PI;

var LINE_WIDTH = 4;
var ARROW_WIDTH = 6;
var line, linePosition;
var arrow1, arrow2;

var SCALE = 0.15;
var ANGLE = 150.0;


VectorArrow = function(distance, showStats, statsTitle, statsPosition) {
  this.magnitude = 0;
  this.direction = {x: 0, y: 0, z: 0};

  this.showStats = showStats;
  this.isDragging = false;

  this.newLine = function(position) {
    linePosition = position;
    var points = []; 

    line = Entities.addEntity({
      position: linePosition,
      type: "Line",
      color: {red: 255, green: 255, blue: 255},
      dimensions: {
        x: LINE_DIMENSIONS,
        y: LINE_DIMENSIONS,
        z: LINE_DIMENSIONS
      },
      lineWidth: LINE_WIDTH,
      lifetime: LIFETIME,
      linePoints: []
    });

    arrow1 = Entities.addEntity({
      position: {x: 0, y: 0, z: 0},
      type: "Line",
      dimensions: {
        x: LINE_DIMENSIONS,
        y: LINE_DIMENSIONS,
        z: LINE_DIMENSIONS
      },
      color: {red: 255, green: 255, blue: 255},
      lineWidth: ARROW_WIDTH,
      linePoints: [],
    });

    arrow2 = Entities.addEntity({
      position: {x: 0, y: 0, z: 0},
      type: "Line",
      dimensions: {
        x: LINE_DIMENSIONS,
        y: LINE_DIMENSIONS,
        z: LINE_DIMENSIONS
      },
      color: {red: 255, green: 255, blue: 255},
      lineWidth: ARROW_WIDTH,
      linePoints: [],
    });

  }


  var STATS_DIMENSIONS = {
    x: 4.0,
    y: 1.5,
    z: 0.1
  };
  var TEXT_HEIGHT = 0.3;

  this.onMousePressEvent = function(event) {
    
    this.newLine(computeWorldPoint(event));

    if (this.showStats) {
      this.label = Entities.addEntity({
        type: "Text",
        position: statsPosition,
        dimensions: STATS_DIMENSIONS,
        lineHeight: TEXT_HEIGHT,
        faceCamera: true
      });
    }

    this.isDragging = true;

  }


  this.onMouseMoveEvent = function(event) {
    
    if (!this.isDragging) {
      return;
    }

    var worldPoint = computeWorldPoint(event);
    var localPoint = computeLocalPoint(event, linePosition);
    points = [{x: 0, y: 0, z: 0}, localPoint];
    Entities.editEntity(line, { linePoints: points });

    var nextOffset = Vec3.multiply(SCALE, localPoint);
    var normOffset = Vec3.normalize(localPoint);
    var axis = Vec3.cross(normOffset, Quat.getFront(Camera.getOrientation()) );
    axis = Vec3.cross(axis, normOffset);
    var rotate1 = Quat.angleAxis(ANGLE, axis);
    var rotate2 = Quat.angleAxis(-ANGLE, axis);

    // Rotate arrow head to follow direction of the line
    Entities.editEntity(arrow1, { 
      visible: true,
      position: worldPoint,
      linePoints: [{x: 0, y: 0, z: 0}, nextOffset],
      rotation: rotate1
    });
    Entities.editEntity(arrow2, { 
      visible: true,
      position: worldPoint,
      linePoints: [{x: 0, y: 0, z: 0}, nextOffset],
      rotation: rotate2
    });

    this.magnitude = Vec3.length(localPoint) * 0.1;
    this.direction = Vec3.normalize(Vec3.subtract(worldPoint, linePosition)); 

    if (this.showStats) {
      this.editLabel(statsTitle + "                          Magnitude  " + this.magnitude.toFixed(2) + ",     Direction: " + 
                this.direction.x.toFixed(2) + ", " + this.direction.y.toFixed(2) + ", " + this.direction.z.toFixed(2));
    } 
  }

  this.onMouseReleaseEvent = function() {
    this.isDragging = false;
  }

  this.cleanup = function() {
      Entities.deleteEntity(line);
      Entities.deleteEntity(arrow1);
      Entities.deleteEntity(arrow2);
  }

  this.deleteLabel = function() {
    Entities.deleteEntity(this.label);
  }
  
  this.editLabel = function(str) {
    if(!this.showStats) {
      return;
    }
    Entities.editEntity(this.label, {
      text: str
    });
  }

  function computeWorldPoint(event) {
    var pickRay = Camera.computePickRay(event.x, event.y);
    var addVector = Vec3.multiply(pickRay.direction, distance);
    return Vec3.sum(Camera.getPosition(), addVector);
  }

  function computeLocalPoint(event, linePosition) {
      var localPoint = Vec3.subtract(computeWorldPoint(event), linePosition);
      return localPoint;
  }

}
