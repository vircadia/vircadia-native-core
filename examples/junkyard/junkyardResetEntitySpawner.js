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


  var SCRIPT_URL = Script.resolvePath("junkyardResetEntityScript.js");
  var MODEL_URL = "http://hifi-content.s3.amazonaws.com/caitlyn/dev/Blueprint%20Objects/Asylum/Asylum_Table/Asylum_Table.fbx";
  var resetEntity = Entities.addEntity({
      type: "Model",
      modelURL: MODEL_URL,
      position: center,
      script: SCRIPT_URL,
      dimensions: {
          x: 2.8,
          y: 1.76,
          z: 1.32
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