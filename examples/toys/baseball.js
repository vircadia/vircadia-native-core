//
//  baseball.js
//  examples/toys
//
//  Created by Stephen Birarda on 10/20/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var BAT_MODEL = "atp:c47deaae09cca927f6bc9cca0e8bbe77fc618f8c3f2b49899406a63a59f885cb.fbx";
var BAT_COLLISION_SHAPE = "atp:9eafceb7510c41d50661130090de7e0632aa4da236ebda84a0059a4be2130e0c.obj";

function createNewBat() {
  // move entity three units in front of the avatar
  var batPosition = Vec3.sum(MyAvatar.position,
                             Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: -0.3, z: -2 }));

  var wand = Entities.addEntity({
      name: 'Bat',
      type: "Model",
      modelURL: BAT_MODEL,
      position: batPosition,
      collisionsWillMove: true,
      compoundShapeURL: BAT_COLLISION_SHAPE
  });
}

createNewBat();
