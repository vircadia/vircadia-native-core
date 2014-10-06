//
//  particleModelExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates creating and editing a particle
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("libraries/globals.js");

var count = 0;
var stopAfter = 100;

var modelPropertiesA = {
    position: { x: 1, y: 1, z: 1 },
    velocity: { x: 0.5, y: 0, z: 0.5 },
    gravity: { x: 0, y: 0, z: 0 },
    damping: 0, 
    radius : 0.25,
    modelURL: HIFI_PUBLICK_BUCKET + "meshes/Feisar_Ship.FBX",
    lifetime: 20
};

var modelPropertiesB = {
    position: { x: 1, y: 1.5, z: 1 },
    velocity: { x: 0.5, y: 0, z: 0.5 },
    gravity: { x: 0, y: 0, z: 0 },
    damping: 0, 
    radius : 0.25,
    modelURL: HIFI_PUBLIC_BUCKET + "meshes/newInvader16x16.svo",
    modelScale: 450,
    modelTranslation: { x: -1.3, y: -1.3, z: -1.3 },
    lifetime: 20
};

var ballProperties = {
    position: { x: 1, y: 0.5, z: 1 },
    velocity: { x: 0.5, y: 0, z: 0.5 },
    gravity: { x: 0, y: 0, z: 0 },
    damping: 0, 
    radius : 0.25,
    color: { red: 255, green: 0, blue: 0 },
    lifetime: 20
};

var modelAParticleID = Particles.addParticle(modelPropertiesA);
var modelBParticleID = Particles.addParticle(modelPropertiesB);
var ballParticleID = Particles.addParticle(ballProperties);

function endAfterAWhile(deltaTime) {
    // stop it...
    if (count >= stopAfter) {
        print("calling Script.stop()");
        Script.stop();
    }

    print("count =" + count);
    count++;
}


// register the call back so it fires before each data send
Script.update.connect(endAfterAWhile);

