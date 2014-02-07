//
//  rayPickExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/6/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Camera class
//
//

function mouseMoveEvent(event) {
    print("mouseMoveEvent event.x,y=" + event.x + ", " + event.y);
    var pickRay = Camera.computePickRay(event.x, event.y);
    print("called Camera.computePickRay()");
    print("computePickRay origin=" + pickRay.origin.x + ", " + pickRay.origin.y + ", " + pickRay.origin.z);
    print("computePickRay direction=" + pickRay.direction.x + ", " + pickRay.direction.y + ", " + pickRay.direction.z);
    var pickRay = Camera.computePickRay(event.x, event.y);
    var intersection = Voxels.findRayIntersection(pickRay);
    if (intersection.intersects) {
        print("intersection voxel.red/green/blue=" + intersection.voxel.red + ", " 
                    + intersection.voxel.green + ", " + intersection.voxel.blue);
    }
}

Controller.mouseMoveEvent.connect(mouseMoveEvent);

function scriptEnding() {
}
Script.scriptEnding.connect(scriptEnding);

