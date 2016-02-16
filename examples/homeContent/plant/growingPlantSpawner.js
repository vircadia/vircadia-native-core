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
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(2, Quat.getFront(orientation)));

var pot, hose;
initializePlant();

function initializePlant() {
  var POT_MODEL_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/pot.fbx";
  var PLANT_SCRIPT_URL = Script.resolvePath("growingPlantEntityScript.js?v1" + Math.random());

  pot = Entities.addEntity({
    type: "Model",
    name: "plant pot",
    modelURL: POT_MODEL_URL,
    position: center
  });

  var HOSE_MODEL_URL = "file:///C:/Users/Eric/Desktop/hose.fbx?v1" + Math.random();
  var HOSE_SCRIPT_URL =  Script.resolvePath("waterHoseEntityScript.js?v1" + Math.random());
  hose = Entities.addEntity({
    type: "Model",
    modelURL: HOSE_MODEL_URL,
    position: Vec3.sum(center, {x: 0.0, y: 1, z: 0}),
    color: {red: 200, green: 10, blue: 200},
  });

  Script.setTimeout(function() {
    var potNaturalDimensions = Entities.getEntityProperties(pot, "naturalDimensions").naturalDimensions;
    Entities.editEntity(pot, {dimensions: potNaturalDimensions, script: PLANT_SCRIPT_URL});

    var hoseNaturalDimensions = Entities.getEntityProperties(hose, "naturalDimensions").naturalDimensions;
    Entities.editEntity(hose, {dimensions: hoseNaturalDimensions, script: HOSE_SCRIPT_URL});
  }, 2000);

}

function cleanup() {
  Entities.deleteEntity(pot);
  Entities.deleteEntity(hose);
}


Script.scriptEnding.connect(cleanup);