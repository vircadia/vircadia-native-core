//
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

Script.include("../../libraries/utils.js");

var SOUND_DATA_KEY = "soundKey";
var MESSAGE_CHANNEL = "Hifi-Sound-Entity";
var SCRIPT_URL = Script.resolvePath("soundEntityScript.js?v1" + Math.random());
//http://hifi-public.s3.amazonaws.com/ryan/Water_Lap_River_Edge_Gentle.L.wav
var userData = {
    soundKey: {
        url: "https://s3-us-west-1.amazonaws.com/hifi-content/eric/Sounds/dove.wav",
        volume: 0.5,
        loop: true
    }
}

var entityProps = {
    type: "Box",
    position: {
        x: 0,
        y: 0,
        z: 0
    },
    color: {
        red: 200,
        green: 10,
        blue: 200
    },
    dimensions: {
        x: .1,
        y: .1,
        z: .1
    },
    userData: JSON.stringify(userData),
    script: SCRIPT_URL
}

var soundEntity1 = Entities.addEntity(entityProps);

userData.soundKey.url = "http://hifi-public.s3.amazonaws.com/ryan/demo/0619_Fireplace__Tree_B.L.wav";
entityProps.userData = JSON.stringify(userData);
entityProps.position.x += 0.3
//var soundEntity2 = Entities.addEntity(entityProps);

function cleanup() {
    Entities.deleteEntity(soundEntity1);
    //Entities.deleteEntity(soundEntity2);
}

Script.scriptEnding.connect(cleanup);