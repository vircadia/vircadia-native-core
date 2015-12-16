//
//  light_modifier_test_scene.js
//
//  Created byJames Pollack @imgntn on 10/19/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Given a selected light, instantiate some entities that represent various values you can dynamically adjust.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
var MODEL_LIGHT_POSITION = {
  x: 0,
  y: -0.3,
  z: 0
};
var MODEL_LIGHT_ROTATION = Quat.angleAxis(-90, {
  x: 1,
  y: 0,
  z: 0
});

var basePosition, avatarRot;
avatarRot = Quat.fromPitchYawRollDegrees(0, MyAvatar.bodyYaw, 0.0);
basePosition = Vec3.sum(MyAvatar.position, Vec3.multiply(-3, Quat.getUp(avatarRot)));

var ground = Entities.addEntity({
  name: 'Hifi-Light-Mod-Floor',
  //type: "Model",
  type: 'Box',
  color: {
    red: 100,
    green: 100,
    blue: 100
  },
  //modelURL: "https://hifi-public.s3.amazonaws.com/eric/models/woodFloor.fbx",
  dimensions: {
    x: 100,
    y: 2,
    z: 100
  },
  position: basePosition,
  shapeType: 'box'
});

var light, block;

function createLight() {
  var blockProperties = Entities.getEntityProperties(block, ["position", "rotation"]);
  var lightTransform = evalLightWorldTransform(blockProperties.position, blockProperties.rotation);
  var lightProperties = {
    name: 'Hifi-Spotlight',
    type: "Light",
    isSpotlight: true,
    dimensions: {
      x: 2,
      y: 2,
      z: 20
    },
    parentID: block,
    color: {
      red: 255,
      green: 0,
      blue: 255
    },
    intensity: 0.035,
    exponent: 1,
    cutoff: 30,
    lifetime: -1,
    position: lightTransform.p,
    rotation: lightTransform.q
  };

  light = Entities.addEntity(lightProperties);

  var message = {
    light: {
      id: light,
      type: 'spotlight',
      initialProperties: lightProperties
    }
  };

  Messages.sendMessage('Hifi-Light-Mod-Receiver', JSON.stringify(message));

}

function createBlock() {
  var position = basePosition;
  position.y += 5;
  var blockProperties = {
    name: 'Hifi-Spotlight-Block',
    type: 'Box',
    dimensions: {
      x: 1,
      y: 1,
      z: 1
    },
    collisionsWillMove: true,
    color: {
      red: 0,
      green: 0,
      blue: 255
    },
    position: position
  }

  block = Entities.addEntity(blockProperties);
}

function evalLightWorldTransform(modelPos, modelRot) {
  return {
    p: Vec3.sum(modelPos, Vec3.multiplyQbyV(modelRot, MODEL_LIGHT_POSITION)),
    q: Quat.multiply(modelRot, MODEL_LIGHT_ROTATION)
  };
}

function cleanup() {
  print('CLEANUP TEST SCENE SCRIPT')
  Entities.deleteEntity(block);
  Entities.deleteEntity(ground);
  Entities.deleteEntity(light);
}

Script.scriptEnding.connect(cleanup);

createBlock();
createLight();