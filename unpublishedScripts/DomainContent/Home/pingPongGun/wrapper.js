//  createPingPongGun.js
//
//  Script Type: Entity Spawner
//  Created by James B. Pollack on  9/30/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This script creates a gun that shoots ping pong balls when you pull the trigger on a hand controller.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../utils.js");
var scriptURL = 'atp:/pingPongGun/pingPongGun.js';
var MODEL_URL = 'atp:/pingPongGun/Pingpong-Gun-New.fbx';
var COLLISION_HULL_URL = 'atp:/pingPongGun/Pingpong-Gun-New.obj';

HomePingPongGun = function(spawnPosition, spawnRotation) {
  var pingPongGun = Entities.addEntity({
    type: "Model",
    name: "home_model_pingPongGun",
    modelURL: MODEL_URL,
    shapeType: 'compound',
    compoundShapeURL: COLLISION_HULL_URL,
    script: scriptURL,
    position: spawnPosition,
    dimensions: {
      x: 0.125,
      y: 0.3875,
      z: 0.9931
    },
    gravity: {
      x: 0,
      y: -10,
      z: 0
    },
    rotation: Quat.fromPitchYawRollDegrees(spawnRotation.x, spawnRotation.y, spawnRotation.z),
    dynamic: true,
    userData: JSON.stringify({
      grabbableKey: {
        invertSolidWhileHeld: true
      },
      'hifiHomeKey': {
        'reset': true
      },
      wearable: {
        joints: {
          RightHand: [{
            x: 0.1177130937576294,
            y: 0.12922893464565277,
            z: 0.08307232707738876
          }, {
            x: 0.4934672713279724,
            y: 0.3605862259864807,
            z: 0.6394805908203125,
            w: -0.4664038419723511
          }],
          LeftHand: [{
            x: 0.09151676297187805,
            y: 0.13639454543590546,
            z: 0.09354984760284424
          }, {
            x: -0.19628101587295532,
            y: 0.6418180465698242,
            z: 0.2830369472503662,
            w: 0.6851521730422974
          }]
        }
      }
    })
  });

  function cleanup() {
    print('HOME PING PONG CLEANUP!')
    Entities.deleteEntity(pingPongGun);
  }

  this.cleanup = cleanup;

  print('HOME CREATED PING PONG GUN')

}