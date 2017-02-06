//
//  spawnStopwatch.js
//
//  Created by Ryan Huffman on 1/20/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var positionToSpawn = Vec3.sum(MyAvatar.position, Quat.getFront(MyAvatar.rotation));

var stopwatchID = Entities.addEntity({
    type: "Model",
    name: "stopwatch/base",
    position: positionToSpawn,
    modelURL: "http://hifi-content.s3.amazonaws.com/alan/dev/Stopwatch.fbx",
    dimensions: {"x":4.129462242126465,"y":1.058512806892395,"z":5.773681640625}
});

var secondHandID = Entities.addEntity({
    type: "Model",
    name: "stopwatch/seconds",
    parentID: stopwatchID,
    localPosition: {"x":-0.004985813982784748,"y":0.39391064643859863,"z":0.8312804698944092},
    dimensions: {"x":0.14095762372016907,"y":0.02546107769012451,"z":1.6077008247375488},
    registrationPoint: {"x":0.5,"y":0.5,"z":1},
    modelURL: "http://hifi-content.s3.amazonaws.com/alan/dev/Stopwatch-sec-hand.fbx",
});

var minuteHandID = Entities.addEntity({
    type: "Model",
    name: "stopwatch/minutes",
    parentID: stopwatchID,
    localPosition: {"x":-0.0023056098725646734,"y":0.3308190703392029,"z":0.21810021996498108},
    dimensions: {"x":0.045471154153347015,"y":0.015412690117955208,"z":0.22930574417114258},
    registrationPoint: {"x":0.5,"y":0.5,"z":1},
    modelURL: "http://hifi-content.s3.amazonaws.com/alan/dev/Stopwatch-min-hand.fbx",
});

Entities.editEntity(stopwatchID, {
    userData: JSON.stringify({
        secondHandID: secondHandID,
        minuteHandID: minuteHandID,
    }),
    script: Script.resolvePath("stopwatchClient.js"),
    serverScripts: Script.resolvePath("stopwatchServer.js")
});
