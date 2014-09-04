//
//  rayPickExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 2/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates use of the Camera class
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function mouseMoveEvent(event) {
    print("mouseMoveEvent event.x,y=" + event.x + ", " + event.y);
    var pickRay = Camera.computePickRay(event.x, event.y);
    print("called Camera.computePickRay()");
    print("computePickRay origin=" + pickRay.origin.x + ", " + pickRay.origin.y + ", " + pickRay.origin.z);
    print("computePickRay direction=" + pickRay.direction.x + ", " + pickRay.direction.y + ", " + pickRay.direction.z);
    var pickRay = Camera.computePickRay(event.x, event.y);
    var intersection = Voxels.findRayIntersection(pickRay);
    if (!intersection.accurate) {
        print(">>> NOTE: intersection not accurate. will try calling Voxels.findRayIntersectionBlocking()");
        intersection = Voxels.findRayIntersectionBlocking(pickRay);
        print(">>> AFTER BLOCKING CALL intersection.accurate=" + intersection.accurate);
    }
    
    if (intersection.intersects) {
        print("intersection voxel.red/green/blue=" + intersection.voxel.red + ", " 
                    + intersection.voxel.green + ", " + intersection.voxel.blue);
        print("intersection voxel.x/y/z/s=" + intersection.voxel.x + ", " 
                    + intersection.voxel.y + ", " + intersection.voxel.z+ ": " + intersection.voxel.s);
        print("intersection face=" + intersection.face);
        print("intersection distance=" + intersection.distance);
        print("intersection intersection.x/y/z=" + intersection.intersection.x + ", " 
                    + intersection.intersection.y + ", " + intersection.intersection.z);

        // also test the getVoxelAt() api which should find and return same voxel

        var voxelAt = Voxels.getVoxelAt(intersection.voxel.x, intersection.voxel.y, intersection.voxel.z, intersection.voxel.s);
        print("voxelAt.x/y/z/s=" + voxelAt.x + ", " + voxelAt.y + ", " + voxelAt.z + ": " + voxelAt.s);
        print("voxelAt.red/green/blue=" + voxelAt.red + ", " + voxelAt.green + ", " + voxelAt.blue);
    }

    intersection = Entities.findRayIntersection(pickRay);
    if (!intersection.accurate) {
        print(">>> NOTE: intersection not accurate. will try calling Entities.findRayIntersectionBlocking()");
        intersection = Entities.findRayIntersectionBlocking(pickRay);
        print(">>> AFTER BLOCKING CALL intersection.accurate=" + intersection.accurate);
    }
    
    if (intersection.intersects) {
        print("intersection entityID.id=" + intersection.entityID.id);
        print("intersection properties.modelURL=" + intersection.properties.modelURL);
        print("intersection face=" + intersection.face);
        print("intersection distance=" + intersection.distance);
        print("intersection intersection.x/y/z=" + intersection.intersection.x + ", " 
                    + intersection.intersection.y + ", " + intersection.intersection.z);
    }
}

Controller.mouseMoveEvent.connect(mouseMoveEvent);

function scriptEnding() {
}
Script.scriptEnding.connect(scriptEnding);

