//
//  flowArtsHutSpawner.js
//  examples
//
//  Created by Eric Levin on 5/14/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  This script creates a special flow arts hut with a numch of flow art toys people can go in and play with
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


Script.include("../../libraries/utils.js");
Script.include("lightBall/LightBall.js");
Script.include("raveStick/RaveStick.js");

var basePosition = Vec3.sum(MyAvatar.position, Vec3.multiply(1, Quat.getFront(Camera.getOrientation())));
basePosition.y = MyAvatar.position.y + 1;

// RAVE ITEMS
var lightBall = new LightBall(basePosition);
var raveStick = new RaveStick(Vec3.sum(basePosition, {x: 1, y: 0.5, z: 1}));


var modelURL = "file:///C:/Users/Eric/Desktop/RaveRoom.fbx?v1" + Math.random();

var roomDimensions = {x: 30.58, y: 15.29, z: 30.58}; 

var raveRoom = Entities.addEntity({
    type: "Model",
    modelURL: modelURL,
    position: basePosition,
    dimensions:roomDimensions,
    visible: true
});

var floor = Entities.addEntity({
    type: "Box",
    position: Vec3.sum(basePosition, {x: 0, y: -1.5, z: 0}),
    dimensions: {x: roomDimensions.x, y: 0.6, z: roomDimensions.z},
    color: {red: 50, green: 10, blue: 100},
    shapeType: 'box'
});



var lightZone = Entities.addEntity({
    type: "Zone",
    shapeType: 'box',
    keyLightIntensity: 0.2,
    keyLightColor: {
        red: 50,
        green: 0,
        blue: 50
    },
    keyLightAmbientIntensity: .2,
    position: MyAvatar.position,
    dimensions: {
        x: 100,
        y: 100,
        z: 100
    }
});


function cleanup() {

    Entities.deleteEntity(raveRoom);
    Entities.deleteEntity(lightZone)
    Entities.deleteEntity(floor);
    lightBall.cleanup();
    raveStick.cleanup();
}

Script.scriptEnding.connect(cleanup);