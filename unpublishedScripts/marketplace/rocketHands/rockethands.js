"use strict";

/*
	rockethands.js
	system

	Created by Cain Kilgore on 30/06/2017
	Copyright 2017 High Fidelity, Inc.

	Distributed under the Apache License, Version 2.0.
	See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
*/

(function() {
	var isRocketing = false;
	
	Script.setInterval(function() {
		if (Controller.Hardware.Vive) {
			var rightHand = Controller.getPoseValue(Controller.Hardware.Vive.RightHand);
			var getHip = MyAvatar.getJointPosition("Hips");
			var worldControllerPos = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, rightHand.translation));

			if ((worldControllerPos.y > (getHip.y - 0.1)) && (worldControllerPos.y < (getHip.y + 0.1))) {
				 if (rightHand.rotation.y < 0.25 && rightHand.rotation.y > -0.25) {
					isRocketing = true;
					Controller.triggerHapticPulse(0.1, 120, 2);
					MyAvatar.motorReferenceFrame = "world";
					var moveVector = Vec3.multiply(Quat.getFront(Camera.getOrientation()), 10);
					if(!MyAvatar.isFlying()) {
						moveVector = Vec3.sum(moveVector, {x: 0, y: 1, z: 0});
					}
					MyAvatar.motorVelocity = moveVector;
					MyAvatar.motorTimescale = 1.0;
				 } else {
					 if (isRocketing) {
						 MyAvatar.motorVelocity = 0;
						 isRocketing = false;
					 }
				 }
			}
		}
	}, 100);
}());