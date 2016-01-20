//
//  dice.js
//  examples
//
//  Created by Philip Rosedale on February 2, 2015
//  Persist toolbar by HRS 6/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Press the dice button to throw some dice from the center of the screen. 
//  Change NUMBER_OF_DICE to change the number thrown (Yahtzee, anyone?) 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var isDice = false;
var NUMBER_OF_DICE = 4;
var LIFETIME = 10000; //  Dice will live for about 3 hours
var dice = [];
var DIE_SIZE = 0.20;

var madeSound = true; //  Set false at start of throw to look for collision

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
SoundCache.getSound("http://s3.amazonaws.com/hifi-public/sounds/dice/diceCollide.wav");


var INSUFFICIENT_PERMISSIONS_ERROR_MSG = "You do not have the necessary permissions to create new objects."

var screenSize = Controller.getViewportDimensions();

var BUTTON_SIZE = 32;
var PADDING = 3;

Script.include(["libraries/toolBars.js"]);
var toolBar = new ToolBar(0, 0, ToolBar.HORIZONTAL, "highfidelity.dice.toolbar", function(screenSize) {
  return {
    x: (screenSize.x / 2 - BUTTON_SIZE * 2 + PADDING),
    y: (screenSize.y - (BUTTON_SIZE + PADDING))
  };
});
var offButton = toolBar.addOverlay("image", {
  width: BUTTON_SIZE,
  height: BUTTON_SIZE,
  imageURL: HIFI_PUBLIC_BUCKET + "images/close.png",
  color: {
    red: 255,
    green: 255,
    blue: 255
  },
  alpha: 1
});

var deleteButton = toolBar.addOverlay("image", {
  x: screenSize.x / 2 - BUTTON_SIZE,
  y: screenSize.y - (BUTTON_SIZE + PADDING),
  width: BUTTON_SIZE,
  height: BUTTON_SIZE,
  imageURL: HIFI_PUBLIC_BUCKET + "images/delete.png",
  color: {
    red: 255,
    green: 255,
    blue: 255
  },
  alpha: 1
});

var diceIconURL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/images/dice.png"
var diceButton = toolBar.addOverlay("image", {
  x: screenSize.x / 2 + PADDING,
  y: screenSize.y - (BUTTON_SIZE + PADDING),
  width: BUTTON_SIZE,
  height: BUTTON_SIZE,
  imageURL: diceIconURL,
  color: {
    red: 255,
    green: 255,
    blue: 255
  },
  alpha: 1
});


var GRAVITY = -3.5;

// NOTE: angularVelocity is in radians/sec
var MAX_ANGULAR_SPEED = Math.PI;


function shootDice(position, velocity) {
  if (!Entities.canRez()) {
    Window.alert(INSUFFICIENT_PERMISSIONS_ERROR_MSG);
  } else {
    for (var i = 0; i < NUMBER_OF_DICE; i++) {
      dice.push(Entities.addEntity({
        type: "Model",
        modelURL: HIFI_PUBLIC_BUCKET + "models/props/Dice/goldDie.fbx",
        position: position,
        velocity: velocity,
        rotation: Quat.fromPitchYawRollDegrees(Math.random() * 360, Math.random() * 360, Math.random() * 360),
        angularVelocity: {
          x: Math.random() * MAX_ANGULAR_SPEED,
          y: Math.random() * MAX_ANGULAR_SPEED,
          z: Math.random() * MAX_ANGULAR_SPEED
        },
        gravity: {
          x: 0,
          y: GRAVITY,
          z: 0
        },
        lifetime: LIFETIME,
        shapeType: "box",
        dynamic: true,
        collisionSoundURL: "http://s3.amazonaws.com/hifi-public/sounds/dice/diceCollide.wav"
      }));
      position = Vec3.sum(position, Vec3.multiply(DIE_SIZE, Vec3.normalize(Quat.getRight(Camera.getOrientation()))));
    }
  }
}

function deleteDice() {
  while (dice.length > 0) {
    Entities.deleteEntity(dice.pop());
  }
}



function mousePressEvent(event) {
  var clickedText = false;
  var clickedOverlay = Overlays.getOverlayAtPoint({
    x: event.x,
    y: event.y
  });
  if (clickedOverlay == offButton) {
    deleteDice();
    Script.stop();
  } else if (clickedOverlay == deleteButton) {
    deleteDice();
  } else if (clickedOverlay == diceButton) {
    var HOW_HARD = 2.0;
    var position = Vec3.sum(Camera.getPosition(), Quat.getFront(Camera.getOrientation()));
    var velocity = Vec3.multiply(HOW_HARD, Quat.getFront(Camera.getOrientation()));
    shootDice(position, velocity);
    madeSound = false;
  }
}

function scriptEnding() {
  toolBar.cleanup();
}

Controller.mousePressEvent.connect(mousePressEvent);
Script.scriptEnding.connect(scriptEnding);
