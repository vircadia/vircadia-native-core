//
//  addVoxelOnMouseClickExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/6/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Camera and Voxels class to implement
//  clicking on a voxel and adding a new voxel on the clicked on face
//
//

function mousePressEvent(event) {
    print("mousePressEvent event.x,y=" + event.x + ", " + event.y);
    var pickRay = Camera.computePickRay(event.x, event.y);
    var intersection = Voxels.findRayIntersection(pickRay);
    if (intersection.intersects) {

        // Note: due to the current C++ "click on voxel" behavior, these values may be the animated color for the voxel
        print("clicked on voxel.red/green/blue=" + intersection.voxel.red + ", "
                    + intersection.voxel.green + ", " + intersection.voxel.blue);
        print("clicked on voxel.x/y/z/s=" + intersection.voxel.x + ", "
                    + intersection.voxel.y + ", " + intersection.voxel.z+ ": " + intersection.voxel.s);
        print("clicked on face=" + intersection.face);
        print("clicked on distance=" + intersection.distance);

        var newVoxel = {
                x: intersection.voxel.x,
                y: intersection.voxel.y,
                z: intersection.voxel.z,
                s: intersection.voxel.s,
                red: 255,
                green: 0,
                blue: 255 };

        if (intersection.face == "MIN_X_FACE") {
            newVoxel.x -= newVoxel.s;
        } else if (intersection.face == "MAX_X_FACE") {
            newVoxel.x += newVoxel.s;
        } else if (intersection.face == "MIN_Y_FACE") {
            newVoxel.y -= newVoxel.s;
        } else if (intersection.face == "MAX_Y_FACE") {
            newVoxel.y += newVoxel.s;
        } else if (intersection.face == "MIN_Z_FACE") {
            newVoxel.z -= newVoxel.s;
        } else if (intersection.face == "MAX_Z_FACE") {
            newVoxel.z += newVoxel.s;
        }

        print("Voxels.setVoxel("+newVoxel.x + ", "
                + newVoxel.y + ", " + newVoxel.z + ", " + newVoxel.s + ", "
                + newVoxel.red + ", " + newVoxel.green + ", " + newVoxel.blue + ")" );

        Voxels.setVoxel(newVoxel.x, newVoxel.y, newVoxel.z, newVoxel.s, newVoxel.red, newVoxel.green, newVoxel.blue);
    }
}

Controller.mousePressEvent.connect(mousePressEvent);

function scriptEnding() {
}
Script.scriptEnding.connect(scriptEnding);
