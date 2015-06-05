//
//  paint.js
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
//
Script.include('lineRider.js')
var MAX_POINTS_PER_LINE = 30;
var DRAWING_DISTANCE = 5;

var colorPalette = [{
  red: 236,
  green: 208,
  blue: 120
}, {
  red: 217,
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



if (hydraCheck() === true) {
  HydraPaint();
} else {
  MousePaint();
}


function cycleColor() {
  currentColor = colorPalette[++currentColorIndex];
  if (currentColorIndex === colorPalette.length - 1) {
    currentColorIndex = -1;
  }

}

function hydraCheck() {
  var numberOfButtons = Controller.getNumberOfButtons();
  var numberOfTriggers = Controller.getNumberOfTriggers();
  var numberOfSpatialControls = Controller.getNumberOfSpatialControls();
  var controllersPerTrigger = numberOfSpatialControls / numberOfTriggers;
  hydrasConnected = (numberOfButtons == 12 && numberOfTriggers == 2 && controllersPerTrigger == 2);
  return hydrasConnected; //hydrasConnected;
}

//************ Mouse Paint **************************

function MousePaint() {
  var lines = [];
  var deletedLines = [];
  var isDrawing = false;
  var path = [];

  var lineRider = new LineRider();
  lineRider.addStartHandler(function() {
    var points = [];
    //create points array from list of all points in path
    path.forEach(function(point) {
      points.push(point);
    });
    lineRider.setPath(points);
  });



  var LINE_WIDTH = 7;
  var line;
  var points = [];


  var BRUSH_SIZE = 0.08;

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


  function newLine(point) {
    line = Entities.addEntity({
      position: MyAvatar.position,
      type: "Line",
      color: currentColor,
      dimensions: {
        x: 10,
        y: 10,
        z: 10
      },
      lineWidth: LINE_WIDTH
    });
    points = [];
    if (point) {
      points.push(point);
      path.push(point);
    }
    lines.push(line);
  }


  function mouseMoveEvent(event) {
    if (!isDrawing) {
      return;
    }


    var pickRay = Camera.computePickRay(event.x, event.y);
    var addVector = Vec3.multiply(Vec3.normalize(pickRay.direction), DRAWING_DISTANCE);
    var point = Vec3.sum(Camera.getPosition(), addVector);
    points.push(point);
    Entities.editEntity(line, {
      linePoints: points
    });
    Entities.editEntity(brush, {
      position: point
    });
    path.push(point);

    if (points.length === MAX_POINTS_PER_LINE) {
      //We need to start a new line!
      newLine(point);
    }
  }

  function undoStroke() {
    var deletedLine = lines.pop();
    var deletedLineProps = Entities.getEntityProperties(deletedLine);
    deletedLines.push(deletedLineProps);
    Entities.deleteEntity(deletedLine);
  }

  function redoStroke() {
    var restoredLine = Entities.addEntity(deletedLines.pop());
    Entities.addEntity(restoredLine);
    lines.push(restoredLine);
  }

  function mousePressEvent(event) {
    lineRider.mousePressEvent(event);
    path = [];
    newLine();
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
    if (event.text === "z") {
      undoStroke();
    }
    if(event.text === "x") {
      redoStroke();
    }
  }

  function cleanup() {
    lines.forEach(function(line) {
      Entities.deleteEntity(line);
    });
    Entities.deleteEntity(brush);
    lineRider.cleanup();

  }


  Controller.mousePressEvent.connect(mousePressEvent);
  Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
  Controller.mouseMoveEvent.connect(mouseMoveEvent);
  Script.scriptEnding.connect(cleanup);

  Controller.keyPressEvent.connect(keyPressEvent);
}



//*****************HYDRA PAINT *******************************************



function HydraPaint() {



  var lineRider = new LineRider();
  lineRider.addStartHandler(function() {
    var points = [];
    //create points array from list of all points in path
    rightController.path.forEach(function(point) {
      points.push(point);
    });
    lineRider.setPath(points);
  });

  var LEFT = 0;
  var RIGHT = 1;

  var currentTime = 0;


  var DISTANCE_FROM_HAND = 2;
  var minBrushSize = .02;
  var maxBrushSize = .04


  var minLineWidth = 5;
  var maxLineWidth = 10;
  var currentLineWidth = minLineWidth;
  var MIN_PAINT_TRIGGER_THRESHOLD = .01;
  var LINE_LIFETIME = 20;
  var COLOR_CHANGE_TIME_FACTOR = 0.1;

  var RIGHT_BUTTON_1 = 7
  var RIGHT_BUTTON_2 = 8
  var RIGHT_BUTTON_3 = 9;
  var RIGHT_BUTTON_4 = 10

  var LEFT_BUTTON_1 = 1;
  var LEFT_BUTTON_2 = 2;
  var LEFT_BUTTON_3 = 3;
  var LEFT_BUTTON_4 = 4;

  var STROKE_SMOOTH_FACTOR = 1;

  var MIN_DRAW_DISTANCE = 1;
  var MAX_DRAW_DISTANCE = 2;

  function controller(side, undoButton, redoButton, cycleColorButton, startRideButton) {
    this.triggerHeld = false;
    this.triggerThreshold = 0.9;
    this.side = side;
    this.palm = 2 * side;
    this.tip = 2 * side + 1;
    this.trigger = side;
    this.lines = [];
    this.deletedLines = [] //just an array of properties objects
    this.isPainting = false;

    this.undoButton = undoButton;
    this.undoButtonPressed = false;
    this.prevUndoButtonPressed = false;

    this.redoButton = redoButton;
    this.redoButtonPressed = false;
    this.prevRedoButtonPressed = false;

    this.cycleColorButton = cycleColorButton;
    this.cycleColorButtonPressed = false;
    this.prevColorCycleButtonPressed = false;

    this.startRideButton = startRideButton;
    this.startRideButtonPressed = false;
    this.prevStartRideButtonPressed = false;

    this.strokeCount = 0;
    this.currentBrushSize = minBrushSize;
    this.points = [];
    this.path = [];

    this.brush = Entities.addEntity({
      type: 'Sphere',
      position: {
        x: 0,
        y: 0,
        z: 0
      },
      color: currentColor,
      dimensions: {
        x: minBrushSize,
        y: minBrushSize,
        z: minBrushSize
      }
    });


    this.newLine = function(point) {
      this.line = Entities.addEntity({
        position: MyAvatar.position,
        type: "Line",
        color: currentColor,
        dimensions: {
          x: 10,
          y: 10,
          z: 10
        },
        lineWidth: 5,
        // lifetime: LINE_LIFETIME
      });
      this.points = [];
      if (point) {
        this.points.push(point);
        this.path.push(point);
      }
      this.lines.push(this.line);
    }

    this.update = function(deltaTime) {
      this.updateControllerState();
      this.avatarPalmOffset = Vec3.subtract(this.palmPosition, MyAvatar.position);
      this.projectedForwardDistance = Vec3.dot(Quat.getFront(Camera.getOrientation()), this.avatarPalmOffset);
      this.mappedPalmOffset = map(this.projectedForwardDistance, -.5, .5, MIN_DRAW_DISTANCE, MAX_DRAW_DISTANCE);
      this.tipDirection = Vec3.normalize(Vec3.subtract(this.tipPosition, this.palmPosition));
      this.offsetVector = Vec3.multiply(this.mappedPalmOffset, this.tipDirection);
      this.drawPoint = Vec3.sum(this.palmPosition, this.offsetVector);
      this.currentBrushSize = map(this.triggerValue, 0, 1, minBrushSize, maxBrushSize);
      Entities.editEntity(this.brush, {
        position: this.drawPoint,
        dimensions: {
          x: this.currentBrushSize,
          y: this.currentBrushSize,
          z: this.currentBrushSize
        },
        color: currentColor
      });
      if (this.triggerValue > MIN_PAINT_TRIGGER_THRESHOLD) {
        if (!this.isPainting) {
          this.isPainting = true;
          this.newLine();
          this.path = [];
        }
        if (this.strokeCount % STROKE_SMOOTH_FACTOR === 0) {
          this.paint(this.drawPoint);
        }
        this.strokeCount++;
      } else if (this.triggerValue < MIN_PAINT_TRIGGER_THRESHOLD && this.isPainting) {
        this.releaseTrigger();
      }

      this.oldPalmPosition = this.palmPosition;
      this.oldTipPosition = this.tipPosition;
    }

    this.releaseTrigger = function() {
      this.isPainting = false;

    }


    this.updateControllerState = function() {
      this.undoButtonPressed = Controller.isButtonPressed(this.undoButton);
      this.redoButtonPressed = Controller.isButtonPressed(this.redoButton);
      this.cycleColorButtonPressed = Controller.isButtonPressed(this.cycleColorButton);
      this.startRideButtonPressed = Controller.isButtonPressed(this.startRideButton);

      //This logic gives us button release
      if (this.prevUndoButtonPressed === true && this.undoButtonPressed === false) {
        //User released undo button, so undo
        this.undoStroke();
      }
      if (this.prevRedoButtonPressed === true && this.redoButtonPressed === false) {
        this.redoStroke();
      }

      if (this.prevCycleColorButtonPressed === true && this.cycleColorButtonPressed === false) {
        cycleColor();
        Entities.editEntity(this.brush, {
          color: currentColor
        });
      }
      if (this.prevStartRideButtonPressed === true && this.startRideButtonPressed === false) {
        lineRider.toggleRide();
      }
      this.prevRedoButtonPressed = this.redoButtonPressed;
      this.prevUndoButtonPressed = this.undoButtonPressed;
      this.prevCycleColorButtonPressed = this.cycleColorButtonPressed;
      this.prevStartRideButtonPressed = this.startRideButtonPressed;

      this.palmPosition = Controller.getSpatialControlPosition(this.palm);
      this.tipPosition = Controller.getSpatialControlPosition(this.tip);
      this.triggerValue = Controller.getTriggerValue(this.trigger);
    }

    this.undoStroke = function() {
      var deletedLine = this.lines.pop();
      var deletedLineProps = Entities.getEntityProperties(deletedLine);
      this.deletedLines.push(deletedLineProps);
      Entities.deleteEntity(deletedLine);
    }

    this.redoStroke = function() {
      var restoredLine = Entities.addEntity(this.deletedLines.pop());
      Entities.addEntity(restoredLine);
      this.lines.push(restoredLine);
    }

    this.paint = function(point) {

      currentLineWidth = map(this.triggerValue, 0, 1, minLineWidth, maxLineWidth);
      this.points.push(point);
      this.path.push(point);
      Entities.editEntity(this.line, {
        linePoints: this.points,
        lineWidth: currentLineWidth,
      });
      if (this.points.length > MAX_POINTS_PER_LINE) {
        this.newLine(point);
      }
    }

    this.cleanup = function() {
      Entities.deleteEntity(this.brush);
      this.lines.forEach(function(line) {
        Entities.deleteEntity(line);
      });
    }
  }

  function update(deltaTime) {
    rightController.update(deltaTime);
    leftController.update(deltaTime);
    currentTime += deltaTime;
  }

  function cleanup() {
    rightController.cleanup();
    leftController.cleanup();
    lineRider.cleanup();
  }

  function mousePressEvent(event) {
    lineRider.mousePressEvent(event);
  }

  function vectorIsZero(v) {
    return v.x === 0 && v.y === 0 && v.z === 0;
  }


  var rightController = new controller(RIGHT, RIGHT_BUTTON_3, RIGHT_BUTTON_4, RIGHT_BUTTON_1, RIGHT_BUTTON_2);
  var leftController = new controller(LEFT, LEFT_BUTTON_3, LEFT_BUTTON_4, LEFT_BUTTON_1, LEFT_BUTTON_2);

  Script.update.connect(update);
  Script.scriptEnding.connect(cleanup);
  Controller.mousePressEvent.connect(mousePressEvent);

}

function randFloat(low, high) {
  return low + Math.random() * (high - low);
}


function randInt(low, high) {
  return Math.floor(randFloat(low, high));
}

function map(value, min1, max1, min2, max2) {
  return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
}