//
//  particleModelExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates creating and editing a particle
//

var count = 0;
var stopAfter = 100;

var modelProperties = {
    position: { x: 1, y: 1, z: 1 },
    velocity: { x: 0.5, y: 0, z: 0.5 },
    gravity: { x: 0, y: 0, z: 0 },
    damping: 0, 
    radius : 0.25,
    modelURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/Feisar_Ship.FBX",
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

var modelParticleID = Particles.addParticle(modelProperties);
var ballParticleID = Particles.addParticle(ballProperties);

function endAfterAWhile() {
    // stop it...
    if (count >= stopAfter) {
        print("calling Script.stop()");
        Script.stop();
    }

    print("count =" + count);
    count++;
}


// register the call back so it fires before each data send
Script.willSendVisualDataCallback.connect(endAfterAWhile);

