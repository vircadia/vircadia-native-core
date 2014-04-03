//
//  count.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/31/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that runs in a loop and displays a counter to the log
//

var count = 0;

function displayCount(deltaTime) {
    print("count =" + count + " deltaTime=" + deltaTime);
    count++;
}

function scriptEnding() {
    print("SCRIPT ENDNG!!!\n");
}

// register the call back so it fires before each data send
Script.update.connect(displayCount);

// register our scriptEnding callback
Script.scriptEnding.connect(scriptEnding);
