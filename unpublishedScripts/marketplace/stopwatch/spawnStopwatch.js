//
//  spawnStopwatch.js
//
//  Created by Ryan Huffman on 1/20/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var forward = Quat.getFront(MyAvatar.orientation);
Vec3.print("Forward: ", forward);
var positionToSpawn = Vec3.sum(MyAvatar.position, Vec3.multiply(3, forward));
var scale = 0.5;
positionToSpawn.y += 0.5;

var stopwatchID = Entities.addEntity({
    type: "Model",
    name: "stopwatch/base",
    position: positionToSpawn,
    modelURL: Script.resolvePath("models/Stopwatch.fbx"),
    dimensions: Vec3.multiply(scale, {"x":4.129462242126465,"y":1.058512806892395,"z":5.773681640625}),
    rotation: Quat.multiply(MyAvatar.orientation, Quat.fromPitchYawRollDegrees(90, 0, 0))
});

var secondHandID = Entities.addEntity({
    type: "Model",
    name: "stopwatch/seconds",
    parentID: stopwatchID,
    localPosition: Vec3.multiply(scale, {"x":-0.004985813982784748,"y":0.39391064643859863,"z":0.8312804698944092}),
    dimensions: Vec3.multiply(scale, {"x":0.14095762372016907,"y":0.02546107769012451,"z":1.6077008247375488}),
    registrationPoint: {"x":0.5,"y":0.5,"z":1},
    modelURL: Script.resolvePath("models/Stopwatch-sec-hand.fbx"),
});

var minuteHandID = Entities.addEntity({
    type: "Model",
    name: "stopwatch/minutes",
    parentID: stopwatchID,
    localPosition: Vec3.multiply(scale, {"x":-0.0023056098725646734,"y":0.3308190703392029,"z":0.21810021996498108}),
    dimensions: Vec3.multiply(scale, {"x":0.045471154153347015,"y":0.015412690117955208,"z":0.22930574417114258}),
    registrationPoint: {"x":0.5,"y":0.5,"z":1},
    modelURL: Script.resolvePath("models/Stopwatch-min-hand.fbx"),
});

var startStopButtonID = Entities.addEntity({
    type: "Model",
    name: "stopwatch/startStop",
    parentID: stopwatchID,
    dimensions: Vec3.multiply(scale, { x: 0.8, y: 0.8, z: 1.0 }),
    localPosition: Vec3.multiply(scale, { x: 0, y: -0.1, z: -2.06 }),
    modelURL: Script.resolvePath("models/transparent-box.fbx")
});

var resetButtonID = Entities.addEntity({
    type: "Model",
    name: "stopwatch/startStop",
    parentID: stopwatchID,
    dimensions: Vec3.multiply(scale, { x: 0.6, y: 0.6, z: 0.8 }),
    localPosition: Vec3.multiply(scale, { x: -1.5, y: -0.1, z: -1.2 }),
    localRotation: Quat.fromVec3Degrees({ x: 0, y: 36, z: 0 }),
    modelURL: Script.resolvePath("models/transparent-box.fbx")
});

Entities.editEntity(stopwatchID, {
    userData: JSON.stringify({
        secondHandID: secondHandID,
        minuteHandID: minuteHandID
    }),
    serverScripts: Script.resolvePath("stopwatchServer.js")
});

Entities.editEntity(startStopButtonID, {
    userData: JSON.stringify({
        stopwatchID: stopwatchID,
        grabbableKey: { wantsTrigger: true }
    }),
    script: Script.resolvePath("stopwatchStartStop.js")
});

Entities.editEntity(resetButtonID, {
    userData: JSON.stringify({
        stopwatchID: stopwatchID,
        grabbableKey: { wantsTrigger: true }
    }),
    script: Script.resolvePath("stopwatchReset.js")
});

Script.stop()
