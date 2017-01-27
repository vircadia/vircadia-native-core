//
//  simpleBot.js
//  examples
//
//  Created by Brad Hefta-Gaub on 12/23/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

function getRandomFloat(min, max) {
    return Math.random() * (max - min) + min;
}

function getRandomInt (min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

function printVector(string, vector) {
    print(string + " " + vector.x + ", " + vector.y + ", " + vector.z);
}

var timePassed = 0.0;
var updateSpeed = 3.0;

var X_MIN = 5.0;
var X_MAX = 15.0;
var Z_MIN = 5.0;
var Z_MAX = 15.0;
var Y_PELVIS = 1.0;

Agent.isAvatar = true;

// change the avatar's position to the random one
Avatar.position = {x:0,y:1.1,z:0}; // { x: getRandomFloat(X_MIN, X_MAX), y: Y_PELVIS, z: getRandomFloat(Z_MIN, Z_MAX) };;  
printVector("New bot, position = ", Avatar.position);

var animationData = {url: "file:///D:/Development/HiFi/hifi/interface/resources/avatar/animations/walk_fwd.fbx", lastFrame: 35};
//Avatar.startAnimation(animationData.url, animationData.fps || 30, 1, true, false, animationData.firstFrame || 0, animationData.lastFrame);
//Avatar.skeletonModelURL = "file:///D:/Development/HiFi/hifi/interface/resources/meshes/being_of_light/being_of_light.fbx";

var millisecondsToWaitBeforeStarting = 4 * 1000;
Script.setTimeout(function () {
    print("Starting at", JSON.stringify(Avatar.position));
    Avatar.startAnimation(animationData.url, animationData.fps || 30, 1, true, false, animationData.firstFrame || 0, animationData.lastFrame);
}, millisecondsToWaitBeforeStarting);



function update(deltaTime) {
    timePassed += deltaTime;
    if (timePassed > updateSpeed) {
        timePassed = 0;
        var newPosition = Vec3.sum(Avatar.position, { x: getRandomFloat(-0.1, 0.1), y: 0, z: getRandomFloat(-0.1, 0.1) });
        Avatar.position = newPosition;
        Vec3.print("new:", newPosition);
    }
}

Script.update.connect(update);