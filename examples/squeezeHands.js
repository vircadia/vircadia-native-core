//
// squeezeHands.js
// examples
//
//  Created by Philip Rosedale on June 4, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var rightHandAnimation = "https://s3-us-west-1.amazonaws.com/highfidelity-public/animations/HandAnim.fbx";
var leftHandAnimation = "";

var LEFT = 0;
var RIGHT = 1;

var lastLeftFrame = 0; 
var lastRightFrame = 0;

var LAST_FRAME = 15.0;    //  What is the number of the last frame we want to use in the animation?

Script.update.connect(function(deltaTime) {
    var leftTriggerValue = Controller.getTriggerValue(LEFT);
    var rightTriggerValue = Controller.getTriggerValue(RIGHT);

    var leftFrame, rightFrame;

    //  Average last two trigger frames together for a bit of smoothing
    leftFrame = (leftTriggerValue * LAST_FRAME) * 0.5 + lastLeftFrame * 0.5;
    rightFrame = (rightTriggerValue * LAST_FRAME) * 0.5 + lastRightFrame * 0.5;
    
    if ((leftFrame != lastLeftFrame) && leftHandAnimation.length){
   		MyAvatar.stopAnimation(leftHandAnimation);
    	MyAvatar.startAnimation(leftHandAnimation, 30.0, 1.0, false, true, leftFrame, leftFrame);
    }
    if ((rightFrame != lastRightFrame) && rightHandAnimation.length) {
   		MyAvatar.stopAnimation(rightHandAnimation);
    	MyAvatar.startAnimation(rightHandAnimation, 30.0, 1.0, false, true, rightFrame, rightFrame);
    }

    lastLeftFrame = leftFrame;
    lastRightFrame = rightFrame;
});

Script.scriptEnding.connect(function() {
	MyAvatar.stopAnimation(leftHandAnimation);
	MyAvatar.stopAnimation(rightHandAnimation);
});