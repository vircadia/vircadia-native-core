//
//  acAudioSearchCompatibleEntitySpawner.js
//  audio/acAudioSearching
//
//  Created by Eric Levin 2/2/2016
//  Copyright 2016 High Fidelity, Inc.

//  This is a client script which spawns entities with a field in userdata compatible with the AcAudioSearchAndInject script
//  These entities specify data about the sound they want to play, such as url, volume, and whether to loop or not
//  The position of the entity determines the position from which the sound plays from
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../../libraries/utils.js");

var SOUND_DATA_KEY = "soundKey";

var soundEntity = Entities.addEntity({
    type: "Box",
    position: {x: 0, y: 0, z: 0},
    color: {red: 200, green: 10, blue: 200},
    dimensions: {x: .1, y: .1, z: .1},
    userData: JSON.stringify({
        soundKey: {
            url: "http://hifi-public.s3.amazonaws.com/ryan/demo/0619_Fireplace__Tree_B.L.wav"
        }
    })
});

function cleanup() {
    Entities.deleteEntity(soundEntity);
}

Script.scriptEnding.connect(cleanup);