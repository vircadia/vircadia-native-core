"use strict";

//
//  rockethands.js
//  system
//
//  Created by Cain Kilgore on 30/06/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
	function logToConsole(message) {
		console.log("[rockethands.js] " + message);
	}
	
	logToConsole("Rockethands.js script is now active.");
	
	Script.setInterval(function() {
		if(Controller.Hardware.Vive) {
			var rightHand = Controller.getPoseValue(Controller.Hardware.Vive.RightHand).rotation.y;

			// logToConsole("Y VALUE: " + rightHand);
			if(rightHand < -0.1 && rightHand > -0.4) {
				logToConsole("Pointing down.. eye position: " + Camera.getOrientation().x + ", " + Camera.getOrientation().y + ", " + Camera.getOrientation().z + " - we have liftoff - I think!");
				    MyAvatar.motorReferenceFrame = "world";
					// MyAvatar.motorVelocity = {x: Camera.getOrientation().x, y: Camera.getOrientation().y*3, z: Camera.getOrientation().z};
					MyAvatar.motorVelocity = Camera.getOrientation()*3;
					MyAvatar.motorTimescale = 1.0;
			}
		}
	}, 1000);
}());
