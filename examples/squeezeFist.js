//
//  squeezeFist.js
//  examples
//
//  Created by Philip Rosedale on 03-June-2014
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates an NPC avatar running an FBX animation loop.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var animation = AnimationCache.getAnimation("https://s3-us-west-1.amazonaws.com/highfidelity-public/animations/HandAnim.fbx");

var jointMapping;

var frameIndex = 0.0;

var FRAME_RATE = 30.0; // frames per second

Script.update.connect(function(deltaTime) {
    var triggerValue = Controller.getTriggerValue(0);
    print(triggerValue);
    if (!jointMapping) {
        var avatarJointNames = MyAvatar.jointNames;
        var animationJointNames = animation.jointNames;
        if (avatarJointNames.length === 0 || animationJointNames.length === 0) {
            return;
        }
        jointMapping = new Array(avatarJointNames.length);
        for (var i = 0; i < avatarJointNames.length; i++) {
            jointMapping[i] = animationJointNames.indexOf(avatarJointNames[i]);
        }
    }
    frameIndex += deltaTime * FRAME_RATE;
    var frames = animation.frames;
    var rotations = frames[Math.floor(frameIndex) % frames.length].rotations;
    for (var j = 0; j < jointMapping.length; j++) {
        var rotationIndex = jointMapping[j];
        if (rotationIndex != -1) {
            MyAvatar.setJointData(j, rotations[rotationIndex]);
        }
    }
});


