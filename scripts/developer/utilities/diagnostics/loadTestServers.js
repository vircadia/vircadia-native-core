//
//  loadTestServers.js 
//  examples/utilities/diagnostics
//
//  Created by Stephen Birarda on 05/08/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  This script is made for load testing HF servers. It connects to the HF servers and sends/receives data.
//  Run this on an assignment-client.
//

var count = 0;
var yawDirection = -1;
var yaw = 45;
var yawMax = 70;
var yawMin = 20;
var vantagePoint = {x: 5000, y: 500, z: 5000};

var isLocal = false;

// set up our EntityViewer with a position and orientation
var orientation = Quat.fromPitchYawRollDegrees(0, yaw, 0);

EntityViewer.setPosition(vantagePoint);
EntityViewer.setOrientation(orientation);
EntityViewer.queryOctree();

Agent.isListeningToAudioStream = true;
Agent.isAvatar = true;

function getRandomInt(min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

function keepLooking(deltaTime) {
    count++;
    
    if (count % getRandomInt(5, 15) == 0) {
        yaw += yawDirection;
        orientation = Quat.fromPitchYawRollDegrees(0, yaw, 0);
        
        if (yaw > yawMax || yaw < yawMin) {
            yawDirection = yawDirection * -1;
        }

        EntityViewer.setOrientation(orientation);
        EntityViewer.queryOctree();
            
    }
    
    // approximately every second, consider stopping
    if (count % 60 == 0) {
        print("considering stop.... elementCount:" + EntityViewer.getOctreeElementsCount());
        
        var stopProbability = 0.05; // 5% chance of stopping
        
        if (Math.random() < stopProbability) {
            print("stopping.... elementCount:" + EntityViewer.getOctreeElementsCount());
            Script.stop();
        } 
    }
}

// register the call back so it fires before each data send
Script.update.connect(keepLooking);
