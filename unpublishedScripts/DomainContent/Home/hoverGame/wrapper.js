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

//var position = {x:1098.4813,y:461.6781,z:-71.3820}
// var dimensions = {x:1.0233,y:3.1541,z:0.8684}
HoverGame = function(spawnPosition, spawnRotation) {

  var scriptURL = "atp:/hoverGame/hoverInner.js";

  var hoverContainerProps = {
    type: 'Model',
    modelURL: 'atp:/hoverGame/hover.fbx',
    name: 'home_model_hoverGame_container',
    dimensions: {x:1.0233,y:3.1541,z:0.8684},
    compoundShapeURL: 'atp:/hoverGame/hoverHull.obj',
    rotation: spawnRotation,
    script: scriptURL,
    userData: JSON.stringify({
      "grabbableKey": {
        "grabbable":false
      },
      'hifiHomeKey': {
        'reset': true
      }
    }),
    dynamic: false,
    position: spawnPosition
  };

  var hoverBall = {
    type: 'Sphere',
    name: 'home_model_hoverGame_sphere',
    dimensions: {
      x: 0.25,
      y: 0.25,
      z: 0.25,
    },
    compoundShapeURL: 'atp:/hoverGame/hoverHull.obj',
    rotation: spawnRotation,
    script: scriptURL,
    userData: JSON.stringify({
      'hifiHomeKey': {
        'reset': true
      }
    }),
    dynamic: true,
    gravity:{
      x:0,
      y:-9.8,
      z:0
    },
    position: spawnPosition,
    userData:JSON.stringify({
      grabKey:{
        shouldCollideWith:'static'
      }
    })
  };

  var hoverContainer = Entities.addEntity(hoverContainerProps);
  var hoverBall = Entities.addEntity(hoverContainerProps);

  function cleanup() {
    print('HOVER GAME CLEANUP!')
    Entities.deleteEntity(hoverInner);
  }

  this.cleanup = cleanup;

  print('HOME CREATED HOVER GAME')

}