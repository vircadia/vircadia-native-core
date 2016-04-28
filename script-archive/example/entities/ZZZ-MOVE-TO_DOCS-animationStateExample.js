//
//  animationStateExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 7/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that runs in a loop and displays a counter to the log
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//

var count = 0;

function displayAnimationDetails(deltaTime) {
    print("count =" + count + " deltaTime=" + deltaTime);
    count++;
    var animationDetails = MyAvatar.getAnimationDetailsByRole("idle");
    print("animationDetails.currentFrame=" + animationDetails.currentFrame);
}

function scriptEnding() {
    print("SCRIPT ENDNG!!!\n");
}

// register the call back so it fires before each data send
Script.update.connect(displayAnimationDetails);

// register our scriptEnding callback
Script.scriptEnding.connect(scriptEnding);
