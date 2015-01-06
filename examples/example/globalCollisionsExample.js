//
//  globalCollisionsExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 1/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates use of the Controller class
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


print("hello...");

function entityCollisionWithVoxel(entity, voxel, collision) {
    print("entityCollisionWithVoxel()..");
    print("   entity.getID()=" + entity.id);
    print("   voxel color...=" + voxel.red + ", " + voxel.green + ", " + voxel.blue);
    Vec3.print('penetration=', collision.penetration);
    Vec3.print('contactPoint=', collision.contactPoint);
}

function entityCollisionWithEntity(entityA, entityB, collision) {
    print("entityCollisionWithParticle()..");
    print("   entityA.getID()=" + entityA.id);
    print("   entityB.getID()=" + entityB.id);
    Vec3.print('penetration=', collision.penetration);
    Vec3.print('contactPoint=', collision.contactPoint);
}

Entities.entityCollisionWithVoxel.connect(entityCollisionWithVoxel);
Entities.entityCollisionWithEntity.connect(entityCollisionWithEntity);

print("here... hello...");
