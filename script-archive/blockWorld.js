//  blockWorld.js
//  examples
//
//  Created by Eric Levin on May 26, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Creates a floor of tiles and then drops planky blocks at random points above the tile floor
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var TILE_SIZE = 7
var GENERATE_INTERVAL = 50;
var NUM_ROWS = 10;
var angVelRange = 4;

var floorTiles = [];
var blocks = [];
var blockSpawner;

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";


var floorPos = Vec3.sum(MyAvatar.position, {
  x: 0,
  y: -2,
  z: 0
});
var x = floorPos.x;

var currentRowIndex = 0;
var currentColumnIndex = 0;

var DROP_HEIGHT = floorPos.y + 5;
var BLOCK_GRAVITY = {
  x: 0,
  y: -9,
  z: 0
};
var BLOCK_SIZE = {
  x: 0.2,
  y: 0.1,
  z: 0.8
};

var bounds = {
  xMin: floorPos.x,
  xMax: floorPos.x + (TILE_SIZE * NUM_ROWS) - TILE_SIZE,
  zMin: floorPos.z,
  zMax: floorPos.z + (TILE_SIZE * NUM_ROWS) - TILE_SIZE
};

var screenSize = Controller.getViewportDimensions();

var BUTTON_SIZE = 32;
var PADDING = 3;

var offButton = Overlays.addOverlay("image", {
  x: screenSize.x / 2 - BUTTON_SIZE * 2 + PADDING,
  y: screenSize.y - (BUTTON_SIZE + PADDING),
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

var deleteButton = Overlays.addOverlay("image", {
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


function generateFloor() {
  for (var z = floorPos.z; currentColumnIndex < NUM_ROWS; z += TILE_SIZE, currentColumnIndex++) {
    floorTiles.push(Entities.addEntity({
      type: 'Box',
      position: {
        x: x,
        y: floorPos.y,
        z: z
      },
      dimensions: {
        x: TILE_SIZE,
        y: 2,
        z: TILE_SIZE
      },
      color: {
        red: randFloat(70, 120),
        green: randFloat(70, 71),
        blue: randFloat(70, 80)
      },
      // dynamic: true
    }));
  }

  currentRowIndex++;
  if (currentRowIndex < NUM_ROWS) {
    currentColumnIndex = 0;
    x += TILE_SIZE;
    Script.setTimeout(generateFloor, GENERATE_INTERVAL);
  } else {
    //Once we're done generating floor, drop planky blocks at random points on floor
    blockSpawner = Script.setInterval(function() {
      dropBlock();
    }, GENERATE_INTERVAL)
  }
}

function dropBlock() {
  var dropPos = floorPos;
  dropPos.y = DROP_HEIGHT;
  dropPos.x = randFloat(bounds.xMin, bounds.xMax);
  dropPos.z = randFloat(bounds.zMin, bounds.zMax);
  blocks.push(Entities.addEntity({
    type: "Model",
    modelURL: 'http://s3.amazonaws.com/hifi-public/marketplace/hificontent/Games/blocks/block.fbx',
    shapeType: 'box',
    position: dropPos,
    dimensions: BLOCK_SIZE,
    dynamic: true,
    gravity: {
      x: 0,
      y: -9,
      z: 0
    },
    velocity: {
      x: 0,
      y: .1,
      z: 0
    },
    angularVelocity: {
      x: randFloat(-angVelRange, angVelRange),
      y: randFloat(-angVelRange, angVelRange),
      z: randFloat(-angVelRange, angVelRange),
    }
  }));
}

function mousePressEvent(event) {
  var clickedOverlay = Overlays.getOverlayAtPoint({
    x: event.x,
    y: event.y
  });
  if (clickedOverlay == offButton) {
    Script.clearInterval(blockSpawner);
  } 
  if(clickedOverlay == deleteButton){
    destroyStuff();
  }
}

generateFloor();

function cleanup() {
  // for (var i = 0; i < floorTiles.length; i++) {
  //   Entities.deleteEntity(floorTiles[i]);
  // }
  // for (var i = 0; i < blocks.length; i++) {
  //   Entities.deleteEntity(blocks[i]);
  // }
  Overlays.deleteOverlay(offButton);
  Overlays.deleteOverlay(deleteButton)
  Script.clearInterval(blockSpawner);
}

function destroyStuff() {
  for (var i = 0; i < floorTiles.length; i++) {
    Entities.deleteEntity(floorTiles[i]);
  }
  for (var i = 0; i < blocks.length; i++) {
    Entities.deleteEntity(blocks[i]);
  }
  Script.clearInterval(blockSpawner);

}

function randFloat(low, high) {
  return Math.floor(low + Math.random() * (high - low));
}

function map(value, min1, max1, min2, max2) {
  return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
}

Script.scriptEnding.connect(cleanup);
Controller.mousePressEvent.connect(mousePressEvent);
