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

var ROBOT_MODEL = "atp:785c81e117206c36205404beec0cc68529644fe377542dbb2d13fae4d665a5de.fbx";
var ROBOT_POSITION = { x: -0.81, y: 0.88, z: 2.12 };
var ROBOT_DIMENSIONS = { x: 0.95, y: 1.76, z: 0.56 };

var BAT_MODEL = "atp:c47deaae09cca927f6bc9cca0e8bbe77fc618f8c3f2b49899406a63a59f885cb.fbx"
var BAT_COLLISION_MODEL = "atp:9eafceb7510c41d50661130090de7e0632aa4da236ebda84a0059a4be2130e0c.obj"

// add the fresh robot at home plate
// var robot = Entities.addEntity({
//     name: 'Robot',
//     type: 'Model',
//     modelURL: ROBOT_MODEL,
//     position: ROBOT_POSITION,
// //    dimensions: ROBOT_DIMENSIONS,
//     animationIsPlaying: true,
//     animation: {
//         url: ROBOT_MODEL,
//         fps: 30
//     }
// });

// add the bat
var bat = Entities.addEntity({
    name: 'Bat',
    type: 'Model',
    modelURL: BAT_MODEL,
    collisionModelURL: BAT_COLLISION_MODEL,
    userData: "{"grabbableKey":{"spatialKey":{"relativePosition":{"x":0.9,"y":0,"z":0},"relativeRotation":{"x":10,"y":0,"z":0}},"kinematicGrab":true}}"
})

function scriptEnding() {
    Entities.deleteEntity(bat);
}

Script.scriptEnding.connect(scriptEnding);
