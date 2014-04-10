//
//  seeingVoxelsExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 2/26/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var count = 0;
var yawDirection = -1;
var yaw = 45;
var yawMax = 70;
var yawMin = 20;
var vantagePoint = {x: 5000, y: 500, z: 5000};

var isLocal = false;

// set up our VoxelViewer with a position and orientation
var orientation = Quat.fromPitchYawRollDegrees(0, yaw, 0);

function getRandomInt(min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

function init() {
    if (isLocal) {
        MyAvatar.position = vantagePoint;
        MyAvatar.orientation = orientation;
    } else {
        VoxelViewer.setPosition(vantagePoint);
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
    if (count % getRandomInt(5, 15) == 0) {
        yaw += yawDirection;
        orientation = Quat.fromPitchYawRollDegrees(0, yaw, 0);
        if (yaw > yawMax || yaw < yawMin) {
            yawDirection = yawDirection * -1;
        }

        if (count % 10000 == 0) {
            print("calling VoxelViewer.queryOctree()... count=" + count + " yaw=" + yaw);
        }

        if (isLocal) {
            MyAvatar.orientation = orientation;
        } else {
            VoxelViewer.setOrientation(orientation);
            VoxelViewer.queryOctree();
            
            if (count % 10000 == 0) {
                print("VoxelViewer.getOctreeElementsCount()=" + VoxelViewer.getOctreeElementsCount());
            }
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
