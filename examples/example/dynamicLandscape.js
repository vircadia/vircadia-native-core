
//  dynamicLandscape.js
//  examples
//
//  Created by Eric Levin on June 8
//  Copyright 2015 High Fidelity, Inc.
//
//  Meditative ocean landscape
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
Script.include(HIFI_PUBLIC_BUCKET + 'scripts/utilities.js')

var NUM_ROWS = 10;
var CUBE_SIZE = 1;
var cubes = [];
var cubesSettings = [];
var time = 0;

var OMEGA = 2.0 * Math.PI/8;
var RANGE = CUBE_SIZE/2;

var center = Vec3.sum(MyAvatar.position, Vec3.multiply(CUBE_SIZE* 10, Quat.getFront(Camera.getOrientation())));


for (var x = 0, rowIndex = 0; x < NUM_ROWS * CUBE_SIZE; x += CUBE_SIZE, rowIndex++) {
  for (var z = 0, columnIndex = 0; z < NUM_ROWS * CUBE_SIZE; z += CUBE_SIZE, columnIndex++) {

    var baseHeight = map( columnIndex + 1, 1, NUM_ROWS, -CUBE_SIZE * 2, -CUBE_SIZE);
    var relativePosition = {
      x: x,
      y: baseHeight,
      z: z
    };
    var position = Vec3.sum(center, relativePosition);
    cubes.push(Entities.addEntity({
      type: 'Box',
      position: position,
      dimensions: {
        x: CUBE_SIZE,
        y: CUBE_SIZE,
        z: CUBE_SIZE
      }
    }));

    var phase = map( (columnIndex + 1) * (rowIndex + 1), 2, NUM_ROWS * NUM_ROWS, Math.PI * 2, Math.PI * 4);
    cubesSettings.push({
      baseHeight: center.y + baseHeight,
      phase: phase
    })
  }
}

function update(deleteTime) {
  time += deleteTime;
  for (var i = 0; i < cubes.length; i++) {
    var phase = cubesSettings[i].phase;
    var props = Entities.getEntityProperties(cubes[i]);
    var newHeight =  Math.sin(time * OMEGA + phase) / 2.0;
    var hue = map(newHeight, -.5, .5, 0.5, 0.7);
    var light = map(newHeight, -.5, .5, 0.4, 0.6)
    newHeight = cubesSettings[i].baseHeight + (newHeight * RANGE);
    var newVelocityY = Math.cos(time * OMEGA + phase) / 2.0 * RANGE * OMEGA;

    var newPosition = props.position;
    var newVelocity = props.velocity;

    newPosition.y = newHeight;
    newVelocity = newVelocityY;
    Entities.editEntity( cubes[i], {
      position: newPosition,
      velocity: props.velocity,
      color: hslToRgb({hue: hue, sat: 0.7, light: light})
    });
  }
}

function cleanup() {
  cubes.forEach(function(cube) {
    Entities.deleteEntity(cube);
  })
}

Script.update.connect(update);
Script.scriptEnding.connect(cleanup)


