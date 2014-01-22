//
//  editParticleExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/31/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates creating and editing a particle
//

var count = 0;

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
             blue: 0 }

};

var positionDelta = { x: 0.05, y: 0, z: 0 };


var particleID = Particles.addParticle(originalProperties);

function moveParticle() {
    if (count >= 100) {
        //Agent.stop();

        // delete it...
        if (count == 100) {
            print("calling Particles.deleteParticle()");
            Particles.deleteParticle(particleID);
        }

        // stop it...
        if (count >= 200) {
            print("calling Agent.stop()");
            Agent.stop();
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
    
    // also check to see if we can "find" particles...
    var searchAt = { x: 9, y: 0, z: 0};
    var searchRadius = 2;
    var foundParticle = Particles.findClosestParticle(searchAt, searchRadius);
    if (foundParticle.isKnownID) {
        print("found particle:" + foundParticle.id);
    } else {
        print("could not find particle in or around x=9 to x=11:");
    }
}


// register the call back so it fires before each data send
Agent.willSendVisualDataCallback.connect(moveParticle);

