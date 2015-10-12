//
//  whiteBoard.js
//  examples/painting
//
//  Created by Eric Levina on 10/12/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Run this script to spawn a whiteboard that one can paint on
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


Script.include("../../libraries/utils.js");

var rotation = Quat.safeEulerAngles(Camera.getOrientation());
rotation = Quat.fromPitchYawRollDegrees(0, rotation.y, 0)
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(rotation)));
var whiteboard = Entities.addEntity({
    type: "Box",
    position: center,
    rotation: rotation,
    dimensions: {x: 2, y: 1.5, z: 0.01},
    color: {red: 255, green: 255, blue: 255}
});

function cleanup() {
    Entities.deleteEntity(whiteboard);
}


Script.scriptEnding.connect(cleanup);