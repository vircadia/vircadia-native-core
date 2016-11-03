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


BlockyGame = function(spawnPosition, spawnRotation) {

  var scriptURL = "atp:/blocky/blocky.js";

  var blockyProps = {
    type: 'Model',
    modelURL:'atp:/blocky/swiper.fbx',
    name: 'home_box_blocky_resetter',
    dimensions: {
      x: 0.2543,
      y: 0.3269,
      z: 0.4154
    },
    rotation:Quat.fromPitchYawRollDegrees(-9.5165,-147.3687,16.6577),
    script: scriptURL,
    userData: JSON.stringify({
      "grabbableKey": {
        "wantsTrigger": true
      },
      'hifiHomeKey': {
        'reset': true
      }
    }),
    dynamic: false,
    position: spawnPosition
  };

  var blocky = Entities.addEntity(blockyProps);

  function cleanup() {
    print('BLOCKY CLEANUP!')
    Entities.deleteEntity(blocky);
  }

  this.cleanup = cleanup;

  print('HOME CREATED BLOCKY GAME BLOCK 1')

}