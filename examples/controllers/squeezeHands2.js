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

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var rightHandAnimation = HIFI_PUBLIC_BUCKET + "animations/RightHandAnimPhilip.fbx";
var leftHandAnimation = HIFI_PUBLIC_BUCKET + "animations/LeftHandAnimPhilip.fbx";

var LEFT = 0;
var RIGHT = 1;

var lastLeftFrame = 0; 
var lastRightFrame = 0;

var leftDirection = true;
var rightDirection = true;

var LAST_FRAME = 15.0;    //  What is the number of the last frame we want to use in the animation?
var SMOOTH_FACTOR = 0.0;
var MAX_FRAMES = 30.0;

var LEFT_HAND_CLICK = Controller.findAction("LEFT_HAND_CLICK");
var RIGHT_HAND_CLICK = Controller.findAction("RIGHT_HAND_CLICK");

Script.update.connect(function(deltaTime) {
    var leftTriggerValue = Controller.getActionValue(LEFT_HAND_CLICK);
    var rightTriggerValue = Controller.getActionValue(RIGHT_HAND_CLICK);

    var leftFrame, rightFrame;

    //  Average last few trigger frames together for a bit of smoothing
    leftFrame = (leftTriggerValue * LAST_FRAME) * (1.0 - SMOOTH_FACTOR) + lastLeftFrame * SMOOTH_FACTOR;
    rightFrame = (rightTriggerValue * LAST_FRAME) * (1.0 - SMOOTH_FACTOR) + lastRightFrame * SMOOTH_FACTOR;

    if (!leftDirection) {
        leftFrame = MAX_FRAMES - leftFrame;
    }
    if (!rightDirection) {
        rightFrame = MAX_FRAMES - rightFrame;
    }

    if ((leftTriggerValue == 1.0) && (leftDirection == true)) {
        leftDirection = false;
        lastLeftFrame = MAX_FRAMES - leftFrame;
    } else if ((leftTriggerValue == 0.0) && (leftDirection == false)) {
        leftDirection = true;
        lastLeftFrame = leftFrame;
    }
    if ((rightTriggerValue == 1.0) && (rightDirection == true)) {
        rightDirection = false;
        lastRightFrame = MAX_FRAMES - rightFrame;
    } else if ((rightTriggerValue == 0.0) && (rightDirection == false)) {
        rightDirection = true;
        lastRightFrame = rightFrame;
    }

    if ((leftFrame != lastLeftFrame) && leftHandAnimation.length){
        MyAvatar.startAnimation(leftHandAnimation, 30.0, 1.0, false, true, leftFrame, leftFrame);
    }
    if ((rightFrame != lastRightFrame) && rightHandAnimation.length) {
    	MyAvatar.startAnimation(rightHandAnimation, 30.0, 1.0, false, true, rightFrame, rightFrame);
    }

    lastLeftFrame = leftFrame;
    lastRightFrame = rightFrame;
});

Script.scriptEnding.connect(function() {
	MyAvatar.stopAnimation(leftHandAnimation);
	MyAvatar.stopAnimation(rightHandAnimation);
});