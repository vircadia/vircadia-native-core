//
//  birdSongs.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//  Plays a sample audio file at the avatar's current location
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

//  First, load a sample sound from a URL
var birds = [];
var playing = [];

var lowerCorner = { x: 0, y: 8, z: 0 };
var upperCorner = { x: 10, y: 10, z: 10 };

var RATE = 0.035;
var numPlaying = 0;
var BIRD_SIZE = 0.1;
var BIRD_VELOCITY = 2.0;
var LIGHT_RADIUS = 10.0;
var BIRD_MASTER_VOLUME = 0.5;

var useLights = true;

function randomVector(scale) {
    return { x: Math.random() * scale - scale / 2.0, y: Math.random() * scale - scale / 2.0, z: Math.random() * scale - scale / 2.0 };
}

function maybePlaySound(deltaTime) {
    if (Math.random() < RATE) {
        //  Set the location and other info for the sound to play
        var whichBird = Math.floor(Math.random() * birds.length);
        //print("playing sound # " + whichBird);
        var position = {
      x: lowerCorner.x + Math.random() * (upperCorner.x - lowerCorner.x),
        y: lowerCorner.y + Math.random() * (upperCorner.y - lowerCorner.y),
      z: lowerCorner.z + Math.random() * (upperCorner.z - lowerCorner.z)
    };
        var options = {
          position: position,
      volume: BIRD_MASTER_VOLUME
        };
        var entityId = Entities.addEntity({
        type: "Sphere",
        position: position,
        dimensions: { x: BIRD_SIZE, y: BIRD_SIZE, z: BIRD_SIZE },
        color: birds[whichBird].color,
        lifetime: 10
    });

        if (useLights) {
            var lightId = Entities.addEntity({
                type: "Light",
                position: position,
                dimensions: { x: LIGHT_RADIUS, y: LIGHT_RADIUS, z: LIGHT_RADIUS },

                isSpotlight: false,
                diffuseColor: birds[whichBird].color,
                ambientColor: { red: 0, green: 0, blue: 0 },
                specularColor: { red: 255, green: 255, blue: 255 },

                constantAttenuation: 0,
                linearAttenuation: 4.0,
                quadraticAttenuation: 2.0,
                lifetime: 10
            });
        }

        playing.push({ audioId: Audio.playSound(birds[whichBird].sound, options), entityId: entityId, lightId: lightId, color: birds[whichBird].color });
    }
    if (playing.length != numPlaying) {
        numPlaying = playing.length;
        //print("number playing = " + numPlaying);
    }
    for (var i = 0; i < playing.length; i++) {
        if (!playing[i].audioId.playing) {
            Entities.deleteEntity(playing[i].entityId);
            if (useLights) {
                Entities.deleteEntity(playing[i].lightId);
            }
            playing.splice(i, 1);
        } else {
            var loudness = playing[i].audioId.loudness;
            var newColor = { red: playing[i].color.red, green: playing[i].color.green, blue: playing[i].color.blue };
            if (loudness > 0.05) {
                newColor.red *= (1.0 - loudness);
                newColor.green *= (1.0 - loudness);
                newColor.blue *= (1.0 - loudness);
            }
            var properties = Entities.getEntityProperties(playing[i].entityId);
            var newPosition = Vec3.sum(properties.position, randomVector(BIRD_VELOCITY * deltaTime));
            if (properties) {
                properties.position = newPosition;
                Entities.editEntity(playing[i].entityId, { position: properties.position, color: newColor });
            }
            if (useLights) {
                var lightProperties = Entities.getEntityProperties(playing[i].lightId);
                if (lightProperties) {
                    Entities.editEntity(playing[i].lightId, { position: newPosition, diffuseColor: newColor });
                }
            }
        }
    }
}

loadBirds();
// Connect a call back that happens every frame
Script.update.connect(maybePlaySound);

//  Delete our little friends if script is stopped
Script.scriptEnding.connect(function() {
    for (var i = 0; i < playing.length; i++) {
        Entities.deleteEntity(playing[i].entityId);
        if (useLights) {
            Entities.deleteEntity(playing[i].lightId);
        }
    }
});

function loadBirds() {
  var sound_filenames = ["bushtit_1.raw", "bushtit_2.raw", "bushtit_3.raw", "mexicanWhipoorwill.raw",
                           "rosyfacedlovebird.raw", "saysphoebe.raw", "westernscreechowl.raw", "bandtailedpigeon.wav", "bridledtitmouse.wav",
                           "browncrestedflycatcher.wav", "commonnighthawk.wav", "commonpoorwill.wav", "doublecrestedcormorant.wav",
                           "gambelsquail.wav", "goldcrownedkinglet.wav", "greaterroadrunner.wav","groovebilledani.wav","hairywoodpecker.wav",
                           "housewren.wav","hummingbird.wav", "mountainchickadee.wav", "nightjar.wav", "piebilledgrieb.wav", "pygmynuthatch.wav",
                           "whistlingduck.wav", "woodpecker.wav"];

  var colors = [
              { red: 242, green: 207, blue: 013 },
            { red: 238, green: 94, blue: 11 },
            { red: 81, green: 30, blue: 7 },
            { red: 195, green: 176, blue: 81 },
            { red: 235, green: 190, blue: 152 },
            { red: 167, green: 99, blue: 52 },
            { red: 199, green: 122, blue: 108 },
            { red: 246, green: 220, blue: 189 },
            { red: 208, green: 145, blue: 65 },
            { red: 173, green: 120 , blue: 71 },
            { red: 132, green: 147, blue: 174 },
            { red: 164, green: 74, blue: 40 },
            { red: 131, green: 127, blue: 134 },
            { red: 209, green: 157, blue: 117 },
            { red: 205, green: 191, blue: 193 },
            { red: 193, green: 154, blue: 118 },
            { red: 205, green: 190, blue: 169 },
            { red: 199, green: 111, blue: 69 },
            { red: 221, green: 223, blue: 228 },
            { red: 115, green: 92, blue: 87 },
            { red: 214, green: 165, blue: 137 },
            { red: 160, green: 124, blue: 33 },
            { red: 117, green: 91, blue: 86 },
            { red: 113, green: 104, blue: 107 },
            { red: 216, green: 153, blue: 99 },
            { red: 242, green: 226, blue: 64 }
  ];

  var SOUND_BASE_URL = "http://public.highfidelity.io/sounds/Animals/";

  for (var i = 0; i < sound_filenames.length; i++) {
      birds.push({
        sound: SoundCache.getSound(SOUND_BASE_URL + sound_filenames[i]),
        color: colors[i]
      });
  }
}
