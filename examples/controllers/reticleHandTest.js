//
//  reticleTest.js
//  examples/controllers
//
//  Created by Brad Hefta-Gaub on 2015/12/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function moveReticle(pitch, yaw) {
    //print("pitch:" + pitch);
    //print("yaw:" + yaw);

    var globalPos = Controller.getReticlePosition();
    globalPos.x += yaw;
    globalPos.y += pitch;
    Controller.setReticlePosition(globalPos);
}

var MAPPING_NAME = "com.highfidelity.testing.reticleWithHand";
var mapping = Controller.newMapping(MAPPING_NAME);
mapping.from(Controller.Standard.RightHand).peek().to(function(pose) {
    var angularVelocityEADs = Quat.safeEulerAngles(pose.angularVelocity); // degrees
    var yaw = isNaN(angularVelocityEADs.y) ? 0 : (-angularVelocityEADs.y / 10);
    var pitch = isNaN(angularVelocityEADs.x) ? 0 : (-angularVelocityEADs.x / 10);
    moveReticle(pitch, yaw);
});
mapping.enable();


Script.scriptEnding.connect(function(){
    mapping.disable();
});
