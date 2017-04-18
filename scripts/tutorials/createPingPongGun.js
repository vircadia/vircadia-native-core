//
//  Created by James B. Pollack @imgntn on April 18, 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var SCRIPT_URL =  "http://hifi-production.s3.amazonaws.com/tutorials/entity_scripts/pingPongGun.js";
var MODEL_URL = 'http://hifi-production.s3.amazonaws.com/tutorials/pingPongGun/Pingpong-Gun-New.fbx';
var COLLISION_HULL_URL = 'http://hifi-production.s3.amazonaws.com/tutorials/pingPongGun/Pingpong-Gun-New.obj';
var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
  x: 0,
  y: 0.5,
  z: 0
}), Vec3.multiply(0.5, Quat.getForward(Camera.getOrientation())));


var pingPongGunProperties = {
  type: "Model",
  name: "Tutorial Ping Pong Gun",
  modelURL: MODEL_URL,
  shapeType: 'compound',
  compoundShapeURL: COLLISION_HULL_URL,
  script: SCRIPT_URL,
  position: center,
  dimensions: {
    x: 0.125,
    y: 0.3875,
    z: 0.9931
  },
  gravity: {
    x: 0,
    y: -5.0,
    z: 0
  },
  lifetime: 3600,
  dynamic: true,
  userData: JSON.stringify({
    grabbableKey: {
      invertSolidWhileHeld: true
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
}

var pingPongGun = Entities.addEntity(pingPongGunProperties);

Script.stop();
