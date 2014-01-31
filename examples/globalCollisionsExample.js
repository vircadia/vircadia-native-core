//
//  globalCollisionsExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/29/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Controller class
//
//


function particleCollisionWithVoxel(particle, voxel) {
    print("particleCollisionWithVoxel()..");
    print("   particle.getID()=" + particle.id);
    print("   voxel color...=" + voxel.red + ", " + voxel.green + ", " + voxel.blue);
}

function particleCollisionWithParticle(particleA, particleB) {
    print("particleCollisionWithParticle()..");
    print("   particleA.getID()=" + particleA.id);
    print("   particleB.getID()=" + particleB.id);
}

Particles.particleCollisionWithVoxel.connect(particleCollisionWithVoxel);
Particles.particleCollisionWithParticle.connect(particleCollisionWithParticle);
