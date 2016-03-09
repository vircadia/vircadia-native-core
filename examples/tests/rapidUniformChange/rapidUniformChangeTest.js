//  rapidUniformChangeTest.js
//  examples
//
//  Created by Eric Levin on  3/9/2016.
//  Copyright 2016 High Fidelity, Inc.
//  This test creates a primitive with a fragment shader and rapidly updates its uniform values
//  For the test to pass, the uniform should update at rate of update loop without the client crashing
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var orientation = Camera.getOrientation();
orientation = Quat.safeEulerAngles(orientation);
orientation.x = 0;
orientation = Quat.fromVec3Degrees(orientation);
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(orientation)));

var SHADER_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/shaders/rapidUniformChangeTest.fs";


var userData = {
    ProceduralEntity: {
        shaderUrl: SHADER_URL,
        uniforms: {
            red: 0.0
        }
    }
}
var testEntity = Entities.addEntity({
    type: "Box",
    dimensions: {
        x: 0.5,
        y: 0.5,
        z: 0.5
    },
    position: center,
    userData: JSON.stringify(userData)
});


var currentTime = 0;

function update(deltaTime) {
    var red = (Math.sin(currentTime) + 1) / 2;
    userData.ProceduralEntity.uniforms.red = red;
    Entities.editEntity(testEntity, {userData: JSON.stringify(userData)});
    currentTime += deltaTime;
}

Script.update.connect(update);

Script.scriptEnding.connect(cleanup);

function cleanup() {
    Entities.deleteEntity(testEntity);
}