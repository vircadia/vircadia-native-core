//
//  growingPlantSpawner.js
//  examples/homeContent/plant
//
//  Created by Eric Levin on 2/10/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This entity script handles the logic for growing a plant when it has water poured on it
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


var orientation = Camera.getOrientation();
orientation = Quat.safeEulerAngles(orientation);
orientation.x = 0;
orientation = Quat.fromVec3Degrees(orientation);


var bowlPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(2, Quat.getFront(orientation)));
var BOWL_MODEL_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/Flowers--Bowl.fbx";
var bowlDimensions = {x: 0.518, y: 0.1938, z: 0.5518};
var bowl= Entities.addEntity({
  type: "Model",
  modelURL: BOWL_MODEL_URL,
  dimensions: bowlDimensions,
  name: "plant bowl",
  position: bowlPosition
});


var PLANT_MODEL_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/Flowers--Moss-Rock.fbx";
var PLANT_SCRIPT_URL = Script.resolvePath("growingPlantEntityScript.js");
var plantDimensions =  {x: 0.52, y: 0.2600, z: 0.52};
var plantPosition = Vec3.sum(bowlPosition, {x: 0, y: plantDimensions.y/2, z: 0});
var plant = Entities.addEntity({
  type: "Model",
  modelURL: PLANT_MODEL_URL,
  name: "plant",
  dimensions: plantDimensions,
  position: plantPosition,
  parentID: bowl
});

var WATER_CAN_MODEL_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/waterCan.fbx";
var waterCanPosition = Vec3.sum(plantPosition, Vec3.multiply(0.6, Quat.getRight(orientation)));
var waterCan = Entities.addEntity({
  type: "Model",
  shapeType: 'box',
  modelURL: WATER_CAN_MODEL_URL,
  position: waterCanPosition,
  dynamic: true
});

function cleanup() {
  Entities.deleteEntity(plant);
  Entities.deleteEntity(bowl);
  Entities.deleteEntity(waterCan);
}


Script.scriptEnding.connect(cleanup);