//
//  audioBall.js
//  hifi
//
//  Created by Athanasios Gaitatzes on 2/10/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This script creates a particle in front of the user that stays in front of
//  the user's avatar as they move, and animates it's radius and color
//  in response to the audio intensity.
//

// add two vectors
function vPlus(a, b) { 
    var rval = { x: a.x + b.x, y: a.y + b.y, z: a.z + b.z };
    return rval;
}

// multiply scalar with vector
function vsMult(s, v) {
    var rval = { x: s * v.x, y: s * v.y, z: s * v.z };
    return rval;
}

var sound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/mexicanWhipoorwill.raw");
var FACTOR = 0.20;

var countParticles = 0;    // the first time around we want to create the particle and thereafter to modify it.
var particleID;

function updateParticle()
{
    // the particle should be placed in front of the user's avatar
	var avatarFront = Quat.getFront(MyAvatar.orientation);

    // move particle three units in front of the avatar
    var particlePosition = vPlus(MyAvatar.position, vsMult (3, avatarFront));

    // play a sound at the location of the particle
    var options = new AudioInjectionOptions();
    options.position = particlePosition;
    options.volume = 0.75;
    Audio.playSound(sound, options);

	var audioCardAverageLoudness = MyAvatar.audioCardAverageLoudness * FACTOR;

    if (countParticles < 1)
    {
        var particleProperies = {
            position: particlePosition // the particle should stay in front of the user's avatar as he moves
        ,   color: { red: 0, green: 255, blue: 0 }
        ,   radius: audioCardAverageLoudness
        ,   velocity: { x: 0.0, y: 0.0, z: 0.0 }
        ,   gravity: { x: 0.0, y: 0.0, z: 0.0 }
        ,   damping: 0.0
        }

        particleID = Particles.addParticle (particleProperies);
        countParticles++;
    }
    else
    {
        // animates the particles radius and color in response to the changing audio intensity
        var newProperties = {
            position: particlePosition // the particle should stay in front of the user's avatar as he moves
        ,   color: { red: 0, green: 255 * audioCardAverageLoudness, blue: 0 }
        ,   radius: audioCardAverageLoudness
        };

        Particles.editParticle (particleID, newProperties);
    }
}

// register the call back so it fires before each data send
Script.willSendVisualDataCallback.connect(updateParticle);

// register our scriptEnding callback
Script.scriptEnding.connect(function scriptEnding() {});
