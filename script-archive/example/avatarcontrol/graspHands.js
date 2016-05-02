//  graspHands.js
//
//  Created by James B. Pollack @imgntn -- 11/19/2015
//  Copyright 2015 High Fidelity, Inc.
// 
//  Shows how to use the animation API to grasp an Avatar's hands.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

//choose a hand.  set it programatically if you'd like
var handToGrasp = 'LEFT_HAND';

//this is our handler, where we do the actual work of changing animation settings
function graspHand(animationProperties) {
    var result = {};
    //full alpha on overlay for this hand
    //set grab to true
    //set idle to false
    //full alpha on the blend btw open and grab
    if (handToGrasp === 'RIGHT_HAND') {
        result['rightHandOverlayAlpha'] = 1.0;
        result['isRightHandGrab'] = true;
        result['isRightHandIdle'] = false;
        result['rightHandGrabBlend'] = 1.0;
    } else if (handToGrasp === 'LEFT_HAND') {
        result['leftHandOverlayAlpha'] = 1.0;
        result['isLeftHandGrab'] = true;
        result['isLeftHandIdle'] = false;
        result['leftHandGrabBlend'] = 1.0;
    }
    //return an object with our updated settings
    return result;
}

//keep a reference to this so we can clear it
var handler;

//register our handler with the animation system
function startHandGrasp() {
    if (handToGrasp === 'RIGHT_HAND') {
        handler = MyAvatar.addAnimationStateHandler(graspHand, ['isRightHandGrab']);
    } else if (handToGrasp === 'LEFT_HAND') {
        handler = MyAvatar.addAnimationStateHandler(graspHand, ['isLeftHandGrab']);
    }
}

function endHandGrasp() {
    // Tell the animation system we don't need any more callbacks.
    MyAvatar.removeAnimationStateHandler(handler);
}

//make sure to clean this up when the script ends so we don't get stuck.
Script.scriptEnding.connect(function() {
    Script.clearInterval(graspInterval);
    endHandGrasp();
})

//set an interval and toggle grasping
var isGrasping = false;
var graspInterval = Script.setInterval(function() {
    if (isGrasping === false) {
        startHandGrasp();
        isGrasping = true;
    } else {
        endHandGrasp();
        isGrasping = false
    }
}, 1000)