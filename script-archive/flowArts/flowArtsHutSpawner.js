//
//  flowArtsHutSpawner.js
//  examples/flowArts
//
//  Created by Eric Levin on 12/17/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This script creates a special flow arts hut with a bunch of flow art toys people can go in and play with
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


Script.include("../../libraries/utils.js");
Script.include("lightBall/lightBall.js");
Script.include("raveStick/raveStick.js");
Script.include("lightSaber/lightSaber.js");
Script.include("arcBall/arcBall.js");



var basePosition = Vec3.sum(MyAvatar.position, Vec3.multiply(1, Quat.getFront(Camera.getOrientation())));
basePosition.y = MyAvatar.position.y + 1;

// RAVE ITEMS
// var lightBall = new LightBall(basePosition);

var arcBall = new ArcBall(basePosition);
var arcBall2 = new ArcBall(Vec3.sum(basePosition, {x: -1, y: 0, z: 0}));
var raveStick = new RaveStick(Vec3.sum(basePosition, {x: 1, y: 0.5, z: 1}));
var lightSaber = new LightSaber(Vec3.sum(basePosition, {x: 3, y: 0.5, z: 1}));


var modelURL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/RaveRoom.fbx";

var roomDimensions = {x: 30.58, y: 15.29, z: 30.58}; 

var raveRoom = Entities.addEntity({
    type: "Model",
    name: "Rave Hut Room",
    modelURL: modelURL,
    position: basePosition,
    dimensions:roomDimensions,
    visible: true
});

var floor = Entities.addEntity({
    type: "Box",
    name: "Rave Floor",
    position: Vec3.sum(basePosition, {x: 0, y: -1.2, z: 0}),
    dimensions: {x: roomDimensions.x, y: 0.6, z: roomDimensions.z},
    color: {red: 50, green: 10, blue: 100},
    shapeType: 'box'
});



var lightZone = Entities.addEntity({
    type: "Zone",
    name: "Rave Hut Zone",
    shapeType: 'box',
    keyLightIntensity: 0.4,
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
    Entities.deleteEntity(lightZone);
    Entities.deleteEntity(floor);
    // lightBall.cleanup();
    arcBall.cleanup();
    arcBall2.cleanup();
    raveStick.cleanup();
    lightSaber.cleanup();
}

Script.scriptEnding.connect(cleanup);