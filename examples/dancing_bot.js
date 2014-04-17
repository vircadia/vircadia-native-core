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

var animation = AnimationCache.getAnimation("http://www.fungibleinsight.com/faces/hip_hop_dancing_2.fbx");

Avatar.skeletonModelURL = "http://www.fungibleinsight.com/faces/vincent.fst";

Agent.isAvatar = true;

var jointMapping;

Script.update.connect(function(deltaTime) {
    if (!jointMapping) {
        var avatarJointNames = Avatar.jointNames;
        var animationJointNames = animation.jointNames;
        if (avatarJointNames === 0 || animationJointNames.length === 0) {
            return;
        }
        print(avatarJointNames);
        print(animationJointNames);
        jointMapping = { };
    }
});


