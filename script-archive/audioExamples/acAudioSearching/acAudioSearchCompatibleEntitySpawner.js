"use strict";
/*jslint nomen: true, plusplus: true, vars: true*/
/*global Entities, Script, Quat, Vec3, Camera, MyAvatar, print, randFloat*/
//  acAudioSearchCompatibleEntitySpawner.js
//  audio/acAudioSearching
//
//  Created by Eric Levin 2/2/2016
//  Copyright 2016 High Fidelity, Inc.

//  This is a client script which spawns entities with a field in userData compatible with the AcAudioSearchAndInject script
//  These entities specify data about the sound they want to play, such as url, volume, and whether to loop or not
//  The position of the entity determines the position from which the sound plays from
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var N_SOUNDS = 1000;
var N_SILENT_ENTITIES_PER_SOUND = 10;
var ADD_PERIOD = 50; // ms between adding 1 sound + N_SILENT_ENTITIES_PER_SOUND, to not overrun entity server.
var SPATIAL_DISTRIBUTION = 10; // meters spread over how far to randomly distribute enties.
Script.include("../../libraries/utils.js");
var orientation = Camera.getOrientation();
orientation = Quat.safeEulerAngles(orientation);
orientation.x = 0;
orientation = Quat.fromVec3Degrees(orientation);
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(orientation)));
// http://hifi-public.s3.amazonaws.com/ryan/demo/0619_Fireplace__Tree_B.L.wav
var SOUND_DATA_KEY = "io.highfidelity.soundKey";
var userData = {
    soundKey: {
        url: "http://hifi-content.s3.amazonaws.com/DomainContent/Junkyard/Sounds/ClothSail/cloth_sail3.L.wav",
        volume: 0.3,
        loop: false,
        playbackGap: 20000, // In ms - time to wait in between clip plays
        playbackGapRange: 5000 // In ms - the range to wait in between clip plays
    }
};

var userDataString = JSON.stringify(userData);
var entityProps = {
    type: "Box",
    name: 'audioSearchEntity',
    color: {
        red: 200,
        green: 10,
        blue: 200
    },
    dimensions: {
        x: 0.1,
        y: 0.1,
        z: 0.1
    }
};

var entities = [], nSounds = 0;
Script.include("../../libraries/utils.js");
function addOneSet() {
    function randomizeDimension(coordinate) {
        return coordinate + randFloat(-SPATIAL_DISTRIBUTION / 2, SPATIAL_DISTRIBUTION / 2);
    }
    function randomize() {
        return {x: randomizeDimension(center.x), y: randomizeDimension(center.y), z: randomizeDimension(center.z)};
    }
    function addOne() {
        entityProps.position = randomize();
        entities.push(Entities.addEntity(entityProps));
    }
    var i;
    entityProps.userData = userDataString;
    entityProps.color.red = 200;
    addOne();
    delete entityProps.userData;
    entityProps.color.red = 10;
    for (i = 0; i < N_SILENT_ENTITIES_PER_SOUND; i++) {
        addOne();
    }
    if (++nSounds < N_SOUNDS) {
        Script.setTimeout(addOneSet, ADD_PERIOD);
    }
}
addOneSet();

function cleanup() {
    entities.forEach(Entities.deleteEntity);
}
// In console:
// Entities.findEntities(MyAvatar.position, 100).forEach(function (id) { if (Entities.getEntityProperties(id).name === 'audioSearchEntity') Entities.deleteEntity(id); })

Script.scriptEnding.connect(cleanup);
