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
var orientation = Camera.getOrientation();
orientation = Quat.safeEulerAngles(orientation);
orientation.x = 0;
orientation = Quat.fromVec3Degrees(orientation);
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(orientation)));
// http://hifi-public.s3.amazonaws.com/ryan/demo/0619_Fireplace__Tree_B.L.wav
var SOUND_DATA_KEY = "soundKey";
var userData = {
    soundKey: {
        url: "http://hifi-content.s3.amazonaws.com/DomainContent/Junkyard/Sounds/ClothSail/cloth_sail3.L.wav",
        volume: 0.3,
        loop: false,
        playbackGap: 2000, // In ms - time to wait in between clip plays
        playbackGapRange: 500 // In ms - the range to wait in between clip plays
    }
}

var entityProps = {
    type: "Box",
    position: center,
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
    userData: JSON.stringify(userData)
}

var soundEntity = Entities.addEntity(entityProps);


function cleanup() {
    Entities.deleteEntity(soundEntity);
}

Script.scriptEnding.connect(cleanup);