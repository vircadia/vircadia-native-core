//
//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var center = Vec3.sum(MyAvatar.position, Vec3.multiply(1.5, Quat.getFront(Camera.getOrientation())));
var SCRIPT_URL = "http://hifi-production.s3.amazonaws.com/tutorials/entity_scripts/pistol.js";
var MODEL_URL = "http://hifi-production.s3.amazonaws.com/tutorials/pistol/gun.fbx";
var COLLISION_SOUND_URL = 'http://hifi-production.s3.amazonaws.com/tutorials/pistol/drop.wav'

var pistolProperties = {
  type: 'Model',
  modelURL: MODEL_URL,
  position: center,
  dimensions: {
    x: 0.05,
    y: 0.23,
    z: 0.36
  },
  script: SCRIPT_URL,
  color: {
    red: 200,
    green: 0,
    blue: 20
  },
  shapeType: 'box',
  dynamic: true,
  gravity: {
    x: 0,
    y: -5.0,
    z: 0
  },
  lifetime: 3600,
  restitution: 0,
  damping: 0.5,
  collisionSoundURL: COLLISION_SOUND_URL,
  userData: JSON.stringify({
    grabbableKey: {
      invertSolidWhileHeld: true
    },
    wearable: {
      joints: {
        RightHand: [{
          x: 0.07079616189002991,
          y: 0.20177987217903137,
          z: 0.06374628841876984
        }, {
          x: -0.5863648653030396,
          y: -0.46007341146469116,
          z: 0.46949487924575806,
          w: -0.4733745753765106
        }],
        LeftHand: [{
          x: 0.1802254319190979,
          y: 0.13442856073379517,
          z: 0.08504903316497803
        }, {
          x: 0.2198076844215393,
          y: -0.7377811074256897,
          z: 0.2780133783817291,
          w: 0.574519157409668
        }]
      }
    }
  })
};

var pistol = Entities.addEntity(pistolProperties);


Script.stop();