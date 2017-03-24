//
//  sphereLodTest.js
//  examples/tests
//
//  Created by Eric Levin on 1/21/16.
//  Copyright 2016 High Fidelity, Inc.

//  A test script for testing LODing of sphere entities and sphere overlays
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

MyAvatar.orientation = Quat.fromPitchYawRollDegrees(0, 0, 0);
orientation = Quat.safeEulerAngles(MyAvatar.orientation);
orientation.x = 0;
orientation = Quat.fromVec3Degrees(orientation);
var tablePosition = Vec3.sum(MyAvatar.position, Quat.getForward(orientation));
tablePosition.y += 0.5;


var tableDimensions = {
    x: 1,
    y: 0.2,
    z: 1
};
var table = Entities.addEntity({
    type: "Box",
    position: tablePosition,
    dimensions: tableDimensions,
    color: {
        red: 70,
        green: 21,
        blue: 21
    }
});


var sphereDimensions = {x: 0.01, y: 0.01, z: 0.01};
var entitySpherePosition = Vec3.sum(tablePosition, {x: 0, y: tableDimensions.y/2 + sphereDimensions.y/2, z: 0});
var entitySphere = Entities.addEntity({
    type: "Sphere",
    position: entitySpherePosition,
    color: {red: 200, green: 20, blue: 200},
    dimensions: sphereDimensions
});

var overlaySpherePosition = Vec3.sum(tablePosition, {x: sphereDimensions.x, y: tableDimensions.y/2 + sphereDimensions.y/2, z: 0});
var overlaySphere = Overlays.addOverlay("sphere", {
    position: overlaySpherePosition,
    size: 0.01,
    color: { red: 20, green: 200, blue: 0},
    alpha: 1.0,
    solid: true,
});



function cleanup() {
    Entities.deleteEntity(table);
    Entities.deleteEntity(entitySphere);
    Overlays.deleteOverlay(overlaySphere);


}
Script.scriptEnding.connect(cleanup);