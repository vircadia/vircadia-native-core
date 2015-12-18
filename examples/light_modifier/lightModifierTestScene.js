//
//  lightModifierTestScene.js
//
//  Created by James Pollack @imgntn on 12/15/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Given a selected light, instantiate some entities that represent various values you can dynamically adjust.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var PARENT_SCRIPT_URL = Script.resolvePath('lightParent.js?'+Math.random(0-100));
var basePosition, avatarRot;
avatarRot = Quat.fromPitchYawRollDegrees(0, MyAvatar.bodyYaw, 0.0);
basePosition = Vec3.sum(MyAvatar.position, Vec3.multiply(0, Quat.getUp(avatarRot)));

var light, block;

function createLight() {
  var blockProperties = Entities.getEntityProperties(block, ["position", "rotation"]);
    var position = basePosition;
  position.y += 2;
  var lightTransform = evalLightWorldTransform(position,avatarRot);
 // var lightTransform = evalLightWorldTransform(blockProperties.position, blockProperties.rotation);
  var lightProperties = {
    name: 'Hifi-Spotlight',
    type: "Light",
    isSpotlight: true,
    dimensions: {
      x: 2,
      y: 2,
      z: 8
    },
   // parentID: block,
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

//  Messages.sendMessage('Hifi-Light-Mod-Receiver', JSON.stringify(message));

}

function createBlock() {
  var position = basePosition;
  position.y += 3;
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
    position: position,
    script:PARENT_SCRIPT_URL,
    userData: JSON.stringify({
      handControllerKey: {
        disableReleaseVelocity: true
      }
    })
  };

  block = Entities.addEntity(blockProperties);
}

function evalLightWorldTransform(modelPos, modelRot) {
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
  return {
    p: Vec3.sum(modelPos, Vec3.multiplyQbyV(modelRot, MODEL_LIGHT_POSITION)),
    q: Quat.multiply(modelRot, MODEL_LIGHT_ROTATION)
  };
}

function cleanup() {
  //Entities.deleteEntity(block);
  Entities.deleteEntity(light);
}

Script.scriptEnding.connect(cleanup);

//createBlock();
createLight();