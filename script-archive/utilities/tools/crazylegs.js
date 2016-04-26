//
//  crazylegs.js
//  examples
//
//  Created by Andrzej Kapolka on 3/6/14.
//  Copyright 2014 High Fidelity, Inc.
// 
//  Outputs the joint index of an avatar, this is useful for avatar procedural animations 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var FREQUENCY = 5.0;

var AMPLITUDE = 45.0;

var cumulativeTime = 0.0;

var jointList = MyAvatar.getJointNames(); 
var jointMappings = "\n# Joint list start";
for (var i = 0; i < jointList.length; i++) {
    jointMappings = jointMappings + "\njointIndex = " + jointList[i] + " = " + i;
}
print(jointMappings + "\n# Joint list end");  

Script.update.connect(function(deltaTime) {
    cumulativeTime += deltaTime;
    MyAvatar.setJointData("RightUpLeg", Quat.fromPitchYawRollDegrees(AMPLITUDE * Math.sin(cumulativeTime * FREQUENCY), 0.0, 0.0));
    MyAvatar.setJointData("LeftUpLeg", Quat.fromPitchYawRollDegrees(-AMPLITUDE * Math.sin(cumulativeTime * FREQUENCY), 0.0, 0.0));
    MyAvatar.setJointData("RightLeg", Quat.fromPitchYawRollDegrees(
        AMPLITUDE * (1.0 + Math.sin(cumulativeTime * FREQUENCY)),0.0, 0.0));
    MyAvatar.setJointData("LeftLeg", Quat.fromPitchYawRollDegrees(
        AMPLITUDE * (1.0 - Math.sin(cumulativeTime * FREQUENCY)),0.0, 0.0));
});

Script.scriptEnding.connect(function() {
    MyAvatar.clearJointData("RightUpLeg");
    MyAvatar.clearJointData("LeftUpLeg");
    MyAvatar.clearJointData("RightLeg");
    MyAvatar.clearJointData("LeftLeg");
});
