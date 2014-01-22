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
    position: { x: 10/TREE_SCALE,
                y: 0/TREE_SCALE,
                z: 0/TREE_SCALE },

    velocity: { x: 0,
                y: 0,
                z: 0 },

    gravity: { x: 0,
                y: 0,
                z: 0 },


    radius : 0.1/TREE_SCALE,

    color: { red: 0,
             green: 255,
             blue: 0 }

};

var positionDelta = { x: 0.1/TREE_SCALE, y: 0, z: 0 };


var particleID = Particles.addParticle(originalProperties);

function moveParticle() {
    if (count >= 10) {
        //Agent.stop();

        // delete it...
        if (count == 10) {
            print("calling Particles.deleteParticle()");
            Particles.deleteParticle(particleID);
        }

        // stop it...
        if (count >= 100) {
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
        radius : 0.25/TREE_SCALE,

    };


    //print("particleID = " + particleID);
    print("newProperties.position = " + newProperties.position.x + "," + newProperties.position.y+ "," + newProperties.position.z);

    Particles.editParticle(particleID, newProperties);
    
    // also check to see if we can "find" particles...
    var searchAt = { x: 0, y: 0, z: 0};
    var searchRadius = 2;
    var foundParticle = Particles.findClosestParticle(searchAt, searchRadius);
    if (foundParticle.isKnownID) {
        print("found particle:" + foundParticle.id);
    }
}


// register the call back so it fires before each data send
Agent.willSendVisualDataCallback.connect(moveParticle);

