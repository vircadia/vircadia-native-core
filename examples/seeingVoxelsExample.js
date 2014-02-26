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

var isLocal = true;

// set up our VoxelViewer with a position and orientation
var orientation = Quat.fromPitchYawRoll(0, yaw, 0);

if (isLocal) {
    MyAvatar.position = {x: 10, y: 0, z: 10};
    MyAvatar.orientation = orientation;
} else {
    VoxelViewer.setPosition({x: 10, y: 0, z: 10});
    VoxelViewer.setOrientation(orientation);
    VoxelViewer.queryOctree();
    Agent.isAvatar = true;
}

function keepLooking() {
    //print("count =" + count);
    count++;
    if (count % 10 == 0) {
        yaw += yawDirection;
        orientation = Quat.fromPitchYawRoll(0, yaw, 0);
        if (yaw > yawMax || yaw < yawMin) {
            yawDirection = yawDirection * -1;
        }

        print("calling VoxelViewer.queryOctree()... count=" + count + " yaw=" + yaw);

        if (isLocal) {
            MyAvatar.orientation = orientation;
        } else {
            VoxelViewer.setOrientation(orientation);
            VoxelViewer.queryOctree();
        }
        
    }
}

function scriptEnding() {
    print("SCRIPT ENDNG!!!\n");
}

// register the call back so it fires before each data send
Script.willSendVisualDataCallback.connect(keepLooking);

// register our scriptEnding callback
Script.scriptEnding.connect(scriptEnding);

