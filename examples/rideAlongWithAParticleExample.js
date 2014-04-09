//
//  rideAlongWithAParticleExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 1/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates "finding" particles
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var iteration = 0;
var lengthOfRide = 2000; // in iterations

var particleA = Particles.addParticle(
                    {
                        position: { x: 10, y: 0, z: 10 },
                        velocity: { x: 5, y: 0, z: 5 },
                        gravity: { x: 0, y: 0, z: 0 },
                        radius : 0.1,
                        color: { red: 0, green: 255, blue: 0 },
                        damping: 0,
                        lifetime: (lengthOfRide * 60) + 1
                    });

function rideWithParticle(deltaTime) {

    if (iteration <= lengthOfRide) {

        // Check to see if we've been notified of the actual identity of the particles we created
        if (!particleA.isKnownID) {
            particleA = Particles.identifyParticle(particleA);
        }

        var propertiesA = Particles.getParticleProperties(particleA);
        var newPosition = propertiesA.position;
        MyAvatar.position = { x: propertiesA.position.x, 
                              y: propertiesA.position.y + 2,
                              z: propertiesA.position.z  };
    } else {
        Script.stop();
    }
                              
    iteration++;
    print("iteration="+iteration);
}


// register the call back so it fires before each data send
Script.update.connect(rideWithParticle);

