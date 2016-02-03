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
var userData = {
    soundKey: {
        url: "https://s3-us-west-1.amazonaws.com/hifi-content/eric/Sounds/dove.wav",
        volume: 0.3,
        loop: false,
        interval: 4,
        intervalSpread: 2 
    }
}

var entityProps = {
    type: "Box",
    position: {
        x: Math.random(),
        y: 0,
        z: 0
    },
    color: {
        red: 200,
        green: 10,
        blue: 200
    },
    dimensions: {
        x: 0.1,
        y: 0.1,
        z: 0.1
    },
    userData: JSON.stringify(userData),
    script: SCRIPT_URL
}

var soundEntity = Entities.addEntity(entityProps);


function cleanup() {
    Entities.deleteEntity(soundEntity);
}

Script.scriptEnding.connect(cleanup);