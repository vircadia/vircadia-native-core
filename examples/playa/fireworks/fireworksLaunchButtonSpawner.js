  //
  //  fireworksLaunchButtonSpawner.js
  //  examples/playa/fireworks
  //
  //  Created by Eric Levina on 2/24/16.
  //  Copyright 2015 High Fidelity, Inc.
  //
  //  Run this script to spawn a big fireworks launch button that a user can press
  //
  //  Distributed under the Apache License, Version 2.0.
  //  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

  var orientation = Camera.getOrientation();
  orientation = Quat.safeEulerAngles(orientation);
  orientation.x = 0;
  orientation = Quat.fromVec3Degrees(orientation);
  var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(orientation)));

  // Math.random ensures no caching of script
  var SCRIPT_URL = Script.resolvePath("fireworksLaunchButtonEntityScript.js");
  var MODEL_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/Launch-Button.fbx";
  var launchButton = Entities.addEntity({
    type: "Model",
    name: "launch pad",
    modelURL: MODEL_URL,
    position: center,
    dimensions: {
      x: 0.98,
      y: 1.16,
      z: 0.98
    },
    script: SCRIPT_URL,
    userData: JSON.stringify({
      launchPosition: {x: 1, y: 1.8, z: -20.9},
      grabbableKey: {
        wantsTrigger: true
      }
    })
  })


  function cleanup() {
    Entities.deleteEntity(launchButton);
  }

  Script.scriptEnding.connect(cleanup);