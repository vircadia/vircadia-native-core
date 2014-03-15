//
//  crazylegs.js
//  hifi
//
//  Created by Andrzej Kapolka on 3/6/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

var FREQUENCY = 5.0;

var AMPLITUDE = 45.0;

var cumulativeTime = 0.0;

Script.update.connect(function(deltaTime) {
    cumulativeTime += deltaTime;
    MyAvatar.setJointData("joint_R_hip", Quat.fromPitchYawRollDegrees(0.0, 0.0, AMPLITUDE * Math.sin(cumulativeTime * FREQUENCY)));
    MyAvatar.setJointData("joint_L_hip", Quat.fromPitchYawRollDegrees(0.0, 0.0, -AMPLITUDE * Math.sin(cumulativeTime * FREQUENCY)));
    MyAvatar.setJointData("joint_R_knee", Quat.fromPitchYawRollDegrees(0.0, 0.0,
        AMPLITUDE * (1.0 + Math.sin(cumulativeTime * FREQUENCY))));
    MyAvatar.setJointData("joint_L_knee", Quat.fromPitchYawRollDegrees(0.0, 0.0,
        AMPLITUDE * (1.0 - Math.sin(cumulativeTime * FREQUENCY))));
});

Script.scriptEnding.connect(function() {
    MyAvatar.clearJointData("joint_R_hip");
    MyAvatar.clearJointData("joint_L_hip");
    MyAvatar.clearJointData("joint_R_knee");
    MyAvatar.clearJointData("joint_L_knee");
});
