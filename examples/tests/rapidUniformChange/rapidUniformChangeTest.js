//  rapidUniformChangeTest.js
//  examples
//
//  Created by Eric Levin on  3/9/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  This test creates primitives with fragment shaders and rapidly updates its uniforms, as well as a skybox.
//  For the test to pass:
//  - The primitives (cube and sphere) should update at rate of update loop, cycling through red values.
//  - The skymap should do the same, although its periodicity may be different.
//
//  Under the hood, the primitives are driven by a uniform, while the skymap is driven by a timer.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var orientation = Camera.getOrientation();
orientation = Quat.safeEulerAngles(orientation);
orientation.x = 0;
orientation = Quat.fromVec3Degrees(orientation);

var centerUp = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(orientation)));
centerUp.y += 0.5;
var centerDown = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(orientation)));
centerDown.y -= 0.5;

var SHADER_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/shaders/rapidUniformChangeTest.fs";

var userData = {
    ProceduralEntity: {
        shaderUrl: SHADER_URL,
        uniforms: { red: 0.0 }
    }
}
var edit = JSON.stringify(userData);
var testBox = Entities.addEntity({
    type: "Box",
    dimensions: { x: 0.5, y: 0.5, z: 0.5 },
    position: centerUp,
    userData: edit
});
var testSphere = Entities.addEntity({
    type: "Sphere",
    dimensions: { x: 0.5, y: 0.5, z: 0.5 },
    position: centerDown,
    userData: edit
});
var testZone = Entities.addEntity({
    type: "Zone",
    dimensions: { x: 3, y: 3, z: 3 },
    position: MyAvatar.position,
    userData: edit
});


var currentTime = 0;

function update(deltaTime) {
    var red = (Math.sin(currentTime) + 1) / 2;
    userData.ProceduralEntity.uniforms.red = red;
    edit = { userData: JSON.stringify(userData) };
    Entities.editEntity(testBox, edit);
    Entities.editEntity(testSphere, edit);
    Entities.editEntity(testZone, edit);

    currentTime += deltaTime;
}

Script.update.connect(update);

Script.scriptEnding.connect(cleanup);

function cleanup() {
    Entities.deleteEntity(testBox);
    Entities.deleteEntity(testSphere);
    Entities.deleteEntity(testZone);
}

