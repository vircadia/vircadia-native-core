  //
  //  junkyardResetEntitySpawner.js
  //  examples/junkyard
  //
  //  Created by Eric Levin on 1/20/16.
  //  Copyright 2016 High Fidelity, Inc.
  //
  //  This script spawns an entity which, when triggered, will reset the junkyard
  //
  //
  //  Distributed under the Apache License, Version 2.0.
  //  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
  //

  var orientation = Camera.getOrientation();
  orientation = Quat.safeEulerAngles(orientation);
  orientation.x = 0;
  orientation = Quat.fromVec3Degrees(orientation);
  var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(orientation)));


  var scriptURL = Script.resolvePath("junkyardResetEntityScript.js");
  var resetEntity = Entities.addEntity({
      type: "Box",
      position: center,
      script: scriptURL,
      dimensions: {
          x: 1,
          y: 1,
          z: 1
      },
      color: {
          red: 200,
          green: 10,
          blue: 200
      }
  });

  function cleanup() {
      Entities.deleteEntity(resetEntity);
  }

  Script.scriptEnding.connect(cleanup);