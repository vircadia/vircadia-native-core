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
  var SCRIPT_URL = Script.resolvePath("fireworksLaunchButtonEntityScript.js?v1" + Math.random())

  var myEntity = Entities.addEntity({
    type: "Box",
    color: {
      red: 200,
      green: 10,
      blue: 10
    },
    position: center,
    dimensions: {
      x: 0.1,
      y: 0.1,
      z: 0.1
    },
    script: SCRIPT_URL,
    userData: JSON.stringify({
      grabbableKey: {
        wantsTrigger: true
      }
    })
  })


  function cleanup() {
    Entities.deleteEntity(myEntity);
  }

  Script.scriptEnding.connect(cleanup);