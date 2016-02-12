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
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(orientation)));

var pot;
initializePlant();

function initializePlant() {
  var MODEL_URL = "file:///C:/Users/Eric/Desktop/pot.fbx";
  var SCRIPT_URL = Script.resolvePath("growingPlantEntityScript.js?v1" + Math.random());

  pot = Entities.addEntity({
    type: "Model",
    modelURL: MODEL_URL,
    position: center
  });

  Script.setTimeout(function() {
    var naturalDimensions = Entities.getEntityProperties(pot, "naturalDimensions").naturalDimensions;
    Entities.editEntity(pot, {dimensions: naturalDimensions, script: SCRIPT_URL});
  }, 100);

}

function cleanup() {
  Entities.deleteEntity(pot);
}


Script.scriptEnding.connect(cleanup);