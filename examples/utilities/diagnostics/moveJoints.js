//
//  moveJoints.js 
//  examples/utilities/diagnostics
//
//  Created by Stephen Birarda on 05/01/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../../libraries/globals.js");

var testAnimation = HIFI_PUBLIC_BUCKET + "ozan/animations/forStephen/sniperJump.fbx";

var FRAME_RATE = 24.0; // frames per second
var isAnimating = false;

Controller.keyPressEvent.connect(function(event) {
  if (event.text == "SHIFT") {
    if (isAnimating) {
      isAnimating = false;
      MyAvatar.stopAnimation(testAnimation);
    } else {
      isAnimating = true;
      MyAvatar.startAnimation(testAnimation, FRAME_RATE, 1.0, true, false);
    }
  }
});
