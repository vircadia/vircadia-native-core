//
//  dancing_bot.js
//  examples
//
//  Created by Andrzej Kapolka on 4/16/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates an NPC avatar running an FBX animation loop.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var animation = AnimationCache.getAnimation("http://www.fungibleinsight.com/faces/gangnam_style_2.fbx");

Avatar.skeletonModelURL = "http://www.fungibleinsight.com/faces/beta.fst";

Agent.isAvatar = true;

var jointMapping;

var currentFrame = 0.0;

var FRAME_RATE = 30.0; // frames per second

Script.update.connect(function(deltaTime) {
    if (!jointMapping) {
        var avatarJointNames = Avatar.jointNames;
        var animationJointNames = animation.jointNames;
        if (avatarJointNames.length === 0 || animationJointNames.length === 0) {
            return;
        }
        jointMapping = new Array(avatarJointNames.length);
        for (var i = 0; i < avatarJointNames.length; i++) {
            jointMapping[i] = animationJointNames.indexOf(avatarJointNames[i]);
        }
    }
    currentFrame += deltaTime * FRAME_RATE;
    var frames = animation.frames;
    var rotations = frames[Math.floor(currentFrame) % frames.length].rotations;
    for (var j = 0; j < jointMapping.length; j++) {
        var rotationIndex = jointMapping[j];
        if (rotationIndex != -1) {
            Avatar.setJointData(j, rotations[rotationIndex]);
        }
    }
});


