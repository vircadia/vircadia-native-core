//
//  seeingVoxelsExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/26/14
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script
//

var count = 0;
var yawDirection = -1;
var yaw = 45;
var yawMax = 70;
var yawMin = 20;

var isLocal = false;

// set up our VoxelViewer with a position and orientation
var orientation = Quat.fromPitchYawRollDegrees(0, yaw, 0);

function init() {
    if (isLocal) {
        MyAvatar.position = {x: 5000, y: 500, z: 5000};
        MyAvatar.orientation = orientation;
    } else {
        VoxelViewer.setPosition({x: 5000, y: 500, z: 5000});
        VoxelViewer.setOrientation(orientation);
        VoxelViewer.queryOctree();
        Agent.isAvatar = true;
    }
}

function keepLooking(deltaTime) {
    //print("count =" + count);

    if (count == 0) {
        init();
    }
    count++;
    if (count % 10 == 0) {
        yaw += yawDirection;
        orientation = Quat.fromPitchYawRollDegrees(0, yaw, 0);
        if (yaw > yawMax || yaw < yawMin) {
            yawDirection = yawDirection * -1;
        }

        print("calling VoxelViewer.queryOctree()... count=" + count + " yaw=" + yaw);

        if (isLocal) {
            MyAvatar.orientation = orientation;
        } else {
            VoxelViewer.setOrientation(orientation);
            VoxelViewer.queryOctree();
            print("VoxelViewer.getOctreeElementsCount()=" + VoxelViewer.getOctreeElementsCount());
        }
        
    }
}

function scriptEnding() {
    print("SCRIPT ENDNG!!!\n");
}

// register the call back so it fires before each data send
Script.update.connect(keepLooking);

// register our scriptEnding callback
Script.scriptEnding.connect(scriptEnding);


// test for local...
Menu.isOptionChecked("Voxels");
isLocal = true; // will only get here on local client
