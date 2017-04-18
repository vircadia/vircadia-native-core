//
//  Created by Eric Levin on 3/25/16
//  Copyright 2016 High Fidelity, Inc.
//
// This spawns a cow which will untip itself
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// references to our assets.  entity scripts need to be served from somewhere that is publically accessible -- so http(s) or atp
var SCRIPT_URL ="http://hifi-production.s3.amazonaws.com/tutorials/entity_scripts/cow.js";
var MODEL_URL = "http://hifi-production.s3.amazonaws.com/tutorials/cow/cow.fbx";
var ANIMATION_URL = 'http://hifi-production.s3.amazonaws.com/tutorials/cow/cow.fbx';

// this part of the code describes how to center the entity in front of your avatar when it is created.
var orientation = MyAvatar.orientation;
orientation = Quat.safeEulerAngles(orientation);
orientation.x = 0;
orientation = Quat.fromVec3Degrees(orientation);
var center = Vec3.sum(MyAvatar.getHeadPosition(), Vec3.multiply(2, Quat.getForward(orientation)));

// An entity is described and created by specifying a map of properties
var cow = Entities.addEntity({
  type: "Model",
  modelURL: MODEL_URL,
  name: "example_cow",
  position: center,
  animation: {
    currentFrame: 278,
    running: true,
    url: ANIMATION_URL
  },
  dimensions: {
    x: 0.739,
    y: 1.613,
    z: 2.529
  },
  dynamic: true,
  gravity: {
    x: 0,
    y: -5,
    z: 0
  },
  lifetime: 3600,
  shapeType: "box",
  script: SCRIPT_URL,
  userData: JSON.stringify({
    grabbableKey: {
      grabbable: true
    }
  })
});

Script.stop();