//
//  proceduralFootPoseExample.js
//  examples/controllers
//
//  Created by Brad Hefta-Gaub on 2015/12/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var MAPPING_NAME = "com.highfidelity.examples.proceduralFootPose";
var mapping = Controller.newMapping(MAPPING_NAME);
var puppetOffset = { x: 0, y: -1, z: 0 };

mapping.from(function() {
    var leftHandPose = Controller.getPoseValue(Controller.Standard.LeftHand);

    var pose = {
            translation: Vec3.sum(leftHandPose.translation, puppetOffset),
            rotation: { x: 0, y: 0, z: 0, w: 0 }, //leftHandPose.rotation,
            velocity: { x: 0, y: 0, z: 0 },
            angularVelocity: { x: 0, y: 0, z: 0 }
        };
    return pose;
}).to(Controller.Standard.LeftFoot);


Controller.enableMapping(MAPPING_NAME);


Script.scriptEnding.connect(function(){
    mapping.disable();
});
