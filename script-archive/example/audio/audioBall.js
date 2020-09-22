//
//  audioBall.js
//  examples
//
//  Created by Athanasios Gaitatzes on 2/10/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This script creates a entity in front of the user that stays in front of
//  the user's avatar as they move, and animates it's size and color
//  in response to the audio intensity.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var VIRCADIA_PUBLIC_CDN = networkingConstants.PUBLIC_BUCKET_CDN_URL;

var sound = SoundCache.getSound(VIRCADIA_PUBLIC_CDN + "sounds/Animals/mexicanWhipoorwill.raw");
var CHANCE_OF_PLAYING_SOUND = 0.01;

var FACTOR = 0.05;

var countEntities = 0;    // the first time around we want to create the entity and thereafter to modify it.
var entityID;

function updateEntity(deltaTime) {
    // the entity should be placed in front of the user's avatar
    var avatarFront = Quat.getFront(MyAvatar.orientation);

    // move entity three units in front of the avatar
    var entityPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(avatarFront, 3));

    if (Math.random() < CHANCE_OF_PLAYING_SOUND) {
        // play a sound at the location of the entity
        Audio.playSound(sound, {
          position: entityPosition,
          volume: 0.75
        });
    }

    var audioAverageLoudness = MyAvatar.audioAverageLoudness * FACTOR;
    print ("Audio Loudness = " + MyAvatar.audioLoudness + " -- Audio Average Loudness = " + MyAvatar.audioAverageLoudness);

    if (countEntities < 1) {
        var entityProperties = {
            type: "Sphere",
            position: entityPosition // the entity should stay in front of the user's avatar as he moves
        ,   color: { red: 0, green: 255, blue: 0 }
        ,   dimensions: {x: audioAverageLoudness, y: audioAverageLoudness, z: audioAverageLoudness }
        ,   velocity: { x: 0.0, y: 0.0, z: 0.0 }
        ,   gravity: { x: 0.0, y: 0.0, z: 0.0 }
        ,   damping: 0.0
        }

        entityID = Entities.addEntity(entityProperties);
        countEntities++;
    }
    else {
        // animates the particles size and color in response to the changing audio intensity
        var newProperties = {
            position: entityPosition // the entity should stay in front of the user's avatar as he moves
        ,   color: { red: 0, green: 255 * audioAverageLoudness, blue: 0 }
        ,   dimensions: {x: audioAverageLoudness, y: audioAverageLoudness, z: audioAverageLoudness }
        };

        Entities.editEntity(entityID, newProperties);
    }
}

// register the call back so it fires before each data send
Script.update.connect(updateEntity);

// register our scriptEnding callback
Script.scriptEnding.connect(function scriptEnding() {
    Entities.deleteEntity(entityID);
});
