//
//  editParticleExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 12/31/13.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates creating and editing a particle
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var count = 0;
var moveUntil = 2000;
var stopAfter = moveUntil + 100;
var expectedLifetime = (moveUntil/60) + 1; // 1 second after done moving...

var originalProperties = {
    position: { x: 10,
                y: 0,
                z: 0 },

    velocity: { x: 0,
                y: 0,
                z: 0 },

    gravity: { x: 0,
                y: 0,
                z: 0 },


    radius : 0.1,

    color: { red: 0,
             green: 255,
             blue: 0 },
             
    lifetime: expectedLifetime

};

var positionDelta = { x: 0.05, y: 0, z: 0 };


var particleID = Particles.addParticle(originalProperties);

function moveParticle(deltaTime) {
    if (count >= moveUntil) {

        // delete it...
        if (count == moveUntil) {
            print("calling Particles.deleteParticle()");
            Particles.deleteParticle(particleID);
        }

        // stop it...
        if (count >= stopAfter) {
            print("calling Script.stop()");
            Script.stop();
        }

        count++;
        return; // break early
    }

    print("count =" + count);
    count++;

    print("particleID.creatorTokenID = " + particleID.creatorTokenID);

    var newProperties = {
        position: {
                x: originalProperties.position.x + (count * positionDelta.x),
                y: originalProperties.position.y + (count * positionDelta.y),
                z: originalProperties.position.z + (count * positionDelta.z)
        },
        radius : 0.25,

    };


    //print("particleID = " + particleID);
    print("newProperties.position = " + newProperties.position.x + "," + newProperties.position.y+ "," + newProperties.position.z);

    Particles.editParticle(particleID, newProperties);
}


// register the call back so it fires before each data send
Script.update.connect(moveParticle);

