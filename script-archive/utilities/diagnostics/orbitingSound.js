//
//  orbitingSound.js
//  examples
//
//  Created by Philip Rosedale on December 4, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  An object playing a sound appears and circles you, changing brightness with the audio playing.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
var RADIUS = 2.0;
var orbitCenter = Vec3.sum(Camera.position, Vec3.multiply(Quat.getFront(Camera.getOrientation()), RADIUS));
var time = 0;
var SPEED = 1.0;
var currentPosition = { x: 0, y: 0, z: 0 };
var trailingLoudness = 0.0;

var soundClip = SoundCache.getSound("http://public.highfidelity.io/sounds/Rats_Squeaks_Active.wav");

var properties = { 
    type: "Box",
    position: orbitCenter, 
    dimensions: { x: 0.25, y: 0.25, z: 0.25 },
    color: { red: 100, green: 0, blue : 0 }
};

var objectId = Entities.addEntity(properties);
var soundOptions = { position: orbitCenter, loop: true, volume: 0.5 };
var sound;

function update(deltaTime) {
    if (!soundClip.downloaded) {
        return;
    }
    if (!sound) {
        sound = Audio.playSound(soundClip, soundOptions); // Not until downloaded
    }
    time += deltaTime; 
    currentPosition = { x: orbitCenter.x + Math.cos(time * SPEED) * RADIUS, y: orbitCenter.y, z: orbitCenter.z + Math.sin(time * SPEED) * RADIUS };
    trailingLoudness = 0.9 * trailingLoudness + 0.1 * sound.loudness;
    Entities.editEntity( objectId, { position: currentPosition, color: { red: Math.min(trailingLoudness * 2000, 255), green: 0, blue: 0 } } );
    soundOptions.position = currentPosition;
    sound.setOptions(soundOptions);
}

Script.scriptEnding.connect(function() {
    Entities.deleteEntity(objectId);
});

Script.update.connect(update);
