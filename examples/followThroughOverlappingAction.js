	//
//  follow-through-and-overlapping-action.js
//
//	Simple demonstration showing the visual effect of adding the
//  follow through and overlapping action technique to avatar movement.
//
//  Designed and created by Ozan Serim and Davedub, August 2014
//
//	Version 1.0
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


// always at the top...
var pathToOverlays = "https://s3-us-west-1.amazonaws.com/highfidelity-public/ArmSwingScript/ArmSwingOverlays/";

// animation
var LEFT = 1;
var RIGHT = 2;
var DIRECTION = 0;

// animation effect variables
var handEffect = 3.4;  // 0 to jointEffectMax
var forearmEffect = 2.5;  // 0 to jointEffectMax
var limbWeight = 8; // will only use nearest integer, as defines an array length
var effectStrength = 1;  // 0 to 1 - overall effect strength
var overShoot = false; // false uses upper arm as driver for forearm and hand, true uses upper arm for forearm and lower arm as driver for hand.

// min max
var weightMin = 1;
var weightMax = 20;
var jointEffectMax = 5;

// animate self (tap the 'r' key)
var animateSelf = false;
var selfAnimateFrequency = 7.5;

// overlay values
var handleValue = 0;
var controlPosX = Window.innerWidth/2 - 500;
var controlPosY = 0;
var minSliderX = controlPosX+18;
var sliderRangeX = 190;
var minHandleX = controlPosX-50;
var handleRangeX = 350/2;
var handlePosition = 0;

// background overlay
var controllerBackground = Overlays.addOverlay("image", {
                    bounds: { x: controlPosX, y: controlPosY, width: 250, height: 380},
                    imageURL: pathToOverlays+"flourish-augmentation-control-overlay.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
var controllerRadioSelectedBackground = Overlays.addOverlay("image", {
                    bounds: { x: controlPosX, y: controlPosY, width: 250, height: 380},
                    imageURL: pathToOverlays+"flourish-augmentation-control-radio-selected-overlay.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    visible: false
                });
// handle overlay
var applyMotionHandle = Overlays.addOverlay("image", {
                    bounds: { x: minHandleX+handleRangeX-39, y: controlPosY+232, width: 79, height: 100},
                    imageURL: pathToOverlays+"flourish-augmentation-handle-overlay.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
// slider overlays
var handEffectSlider = Overlays.addOverlay("image", {
                    bounds: { x: minSliderX + (handEffect / jointEffectMax * sliderRangeX), y: controlPosY+46, width: 25, height: 25},
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
var forearmEffectSlider = Overlays.addOverlay("image", {
                    bounds: { x: minSliderX + (forearmEffect / jointEffectMax * sliderRangeX), y: controlPosY+86, width: 25, height: 25},
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
var limbWeightSlider = Overlays.addOverlay("image", {
                    bounds: { x: minSliderX + (limbWeight / weightMax * sliderRangeX), y: controlPosY+126, width: 25, height: 25},
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });
var effectStrengthSlider = Overlays.addOverlay("image", {
                    bounds: { x: minSliderX + (effectStrength * sliderRangeX), y: controlPosY+206, width: 25, height: 25},
                    imageURL: pathToOverlays+"ddao-slider-handle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });


// main loop - using averaging filters to add limb element follow-through
var upperArmPDampingFilter = [];
upperArmPDampingFilter.length = parseInt(limbWeight);   // sets amount of damping for upper arm pitch
var forearmPDampingFilter = [];
forearmPDampingFilter.length = parseInt(limbWeight) + 2;  // sets amount of damping for lower arm pitch

var cumulativeTime = 0;

Script.update.connect(function(deltaTime) {

	// used for self animating (press r to invoke)
	cumulativeTime += deltaTime;

	// blend three keyframes using handle position to determine progress between keyframes
	var animationProgress = handleValue;

	if(animateSelf) {
		animationProgress = Math.sin(cumulativeTime * selfAnimateFrequency);
		animationProgress++;
		animationProgress/=2;
	}

	var keyframeOneWeight = 0;
	var keyframeTwoWeight = 0;
	var keyframeThreeWeight = 0;

	if(movingHandle || animateSelf) {
		keyframeOneWeight = 0;
		keyframeTwoWeight = animationProgress;
		keyframeThreeWeight = 1-animationProgress;
	}
	else if(!movingHandle) {
		// idle
		keyframeOneWeight = 1;
		keyframeTwoWeight = 0;
		keyframeThreeWeight = 0;
	}

	var shoulderPitch =
		keyframeOneWeight * keyFrameOne.joints[8].pitchOffset +
		keyframeTwoWeight * keyFrameTwo.joints[8].pitchOffset +
		keyframeThreeWeight * keyFrameThree.joints[8].pitchOffset;

	var upperArmPitch =
		keyframeOneWeight * keyFrameOne.joints[9].pitchOffset +
		keyframeTwoWeight * keyFrameTwo.joints[9].pitchOffset +
		keyframeThreeWeight * keyFrameThree.joints[9].pitchOffset;

	// get the change in upper arm pitch and use to add weight effect to forearm (always) and hand (only for overShoot 1)
	var deltaUpperArmPitch = effectStrength * (upperArmPDampingFilter[upperArmPDampingFilter.length-1] - upperArmPDampingFilter[0]);

	var forearmPitch =
		keyframeOneWeight * keyFrameOne.joints[10].pitchOffset +
		keyframeTwoWeight * keyFrameTwo.joints[10].pitchOffset +
		keyframeThreeWeight * keyFrameThree.joints[10].pitchOffset -
		(deltaUpperArmPitch/(jointEffectMax - forearmEffect));

	// there are two methods for calculating the hand follow through
	var handPitch = 0;
	if(overShoot) {
		// get the change in forearm pitch and use to add weight effect to hand
		var deltaForearmPitch = effectStrength * (forearmPDampingFilter[forearmPDampingFilter.length-1] - forearmPDampingFilter[0]);
		handPitch =
		keyframeOneWeight * keyFrameOne.joints[11].pitchOffset +
		keyframeTwoWeight * keyFrameTwo.joints[11].pitchOffset +
		keyframeThreeWeight * keyFrameThree.joints[11].pitchOffset +
		(deltaForearmPitch/(jointEffectMax - handEffect)); // driven by forearm
	} else {
		handPitch =
		keyframeOneWeight * keyFrameOne.joints[11].pitchOffset +
		keyframeTwoWeight * keyFrameTwo.joints[11].pitchOffset +
		keyframeThreeWeight * keyFrameThree.joints[11].pitchOffset -
		(deltaUpperArmPitch/(jointEffectMax - handEffect)); // driven by upper arm
	}

	var shoulderYaw =
		keyframeOneWeight * keyFrameOne.joints[8].yawOffset +
		keyframeTwoWeight * keyFrameTwo.joints[8].yawOffset +
		keyframeThreeWeight * keyFrameThree.joints[8].yawOffset;

	var upperArmYaw =
		keyframeOneWeight * keyFrameOne.joints[9].yawOffset +
		keyframeTwoWeight * keyFrameTwo.joints[9].yawOffset +
		keyframeThreeWeight * keyFrameThree.joints[9].yawOffset;

	var lowerArmYaw =
		keyframeOneWeight * keyFrameOne.joints[10].yawOffset +
		keyframeTwoWeight * keyFrameTwo.joints[10].yawOffset +
		keyframeThreeWeight * keyFrameThree.joints[10].yawOffset;

	var handYaw =
		keyframeOneWeight * keyFrameOne.joints[11].yawOffset +
		keyframeTwoWeight * keyFrameTwo.joints[11].yawOffset +
		keyframeThreeWeight * keyFrameThree.joints[11].yawOffset;

	var shoulderRoll =
		keyframeOneWeight * keyFrameOne.joints[8].rollOffset +
		keyframeTwoWeight * keyFrameTwo.joints[8].rollOffset +
		keyframeThreeWeight * keyFrameThree.joints[8].rollOffset;

	var upperArmRoll =
		keyframeOneWeight * keyFrameOne.joints[9].rollOffset +
		keyframeTwoWeight * keyFrameTwo.joints[9].rollOffset +
		keyframeThreeWeight * keyFrameThree.joints[9].rollOffset;

	var lowerArmRoll =
		keyframeOneWeight * keyFrameOne.joints[10].rollOffset +
		keyframeTwoWeight * keyFrameTwo.joints[10].rollOffset +
		keyframeThreeWeight * keyFrameThree.joints[10].rollOffset;

	var handRoll =
		keyframeOneWeight * keyFrameOne.joints[11].rollOffset +
		keyframeTwoWeight * keyFrameTwo.joints[11].rollOffset +
		keyframeThreeWeight * keyFrameThree.joints[11].rollOffset;

	// filter upper arm pitch
	upperArmPDampingFilter.push(upperArmPitch);
	upperArmPDampingFilter.shift();
	var upperArmPitchFiltered = 0;
	for(ea in upperArmPDampingFilter) upperArmPitchFiltered += upperArmPDampingFilter[ea];
	upperArmPitchFiltered /= upperArmPDampingFilter.length;
	upperArmPitch = (effectStrength*upperArmPitchFiltered) + ((1-(effectStrength)) * upperArmPitch);

	// filter forearm pitch only if using for hand follow-though
	if(overShoot) {
		forearmPDampingFilter.push(forearmPitch);
		forearmPDampingFilter.shift();
		var forearmPitchFiltered = 0;
		for(ea in forearmPDampingFilter) forearmPitchFiltered += forearmPDampingFilter[ea];
		forearmPitchFiltered /= forearmPDampingFilter.length;
		forearmPitch = (effectStrength*forearmPitchFiltered) + ((1-(effectStrength)) * forearmPitch);
	}

	// apply the new rotation data to the joints
	MyAvatar.setJointData("RightShoulder", Quat.fromPitchYawRollDegrees( shoulderPitch, shoulderYaw, shoulderRoll));
	MyAvatar.setJointData("RightArm", Quat.fromPitchYawRollDegrees( upperArmPitch, -upperArmYaw, upperArmRoll));
	MyAvatar.setJointData("RightForeArm", Quat.fromPitchYawRollDegrees( forearmPitch, lowerArmYaw, lowerArmRoll));
	MyAvatar.setJointData("RightHand", Quat.fromPitchYawRollDegrees( handPitch, handYaw, handRoll));
});


// mouse handling
var movingHandEffectSlider = false;
var movingForearmEffectSlider = false;
var movingLimbWeightSlider = false;
var movingDampingSlider = false;
var movingEffectStrengthSlider = false;
var movingHandle = false;

function mousePressEvent(event) {

	var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});

	if(clickedOverlay===applyMotionHandle) movingHandle = true;
	else if(clickedOverlay===handEffectSlider) movingHandEffectSlider = true;
	else if(clickedOverlay===forearmEffectSlider) movingForearmEffectSlider = true;
	else if(clickedOverlay===limbWeightSlider) movingLimbWeightSlider = true;
	else if(clickedOverlay===effectStrengthSlider) movingEffectStrengthSlider = true;
	else if(clickedOverlay===controllerRadioSelectedBackground &&
			event.x>477&&event.x<497&&event.y>338&&event.y<360) {
		overShoot = false;
		Overlays.editOverlay(controllerBackground, {visible:true});
		Overlays.editOverlay(controllerRadioSelectedBackground, {visible:false});
	}
	else if(clickedOverlay===controllerBackground &&
			event.x>477&&event.x<497&&event.y>338&&event.y<360){
		overShoot = true;
		Overlays.editOverlay(controllerBackground, {visible:false});
		Overlays.editOverlay(controllerRadioSelectedBackground, {visible:true});
	}
}
function mouseMoveEvent(event) {

	if(movingHandle) {
		var thumbClickOffsetX = event.x - minHandleX;
		var thumbPositionNormalised = (thumbClickOffsetX - handleRangeX + 39) / handleRangeX;
		if(thumbPositionNormalised<=-1) thumbPositionNormalised = -1;
		else if(thumbPositionNormalised>1) thumbPositionNormalised = 1;

		if(thumbPositionNormalised<0) DIRECTION = LEFT;
		else DIRECTION = RIGHT;

		handleValue = (thumbPositionNormalised+1)/2;
		var handleX = (thumbPositionNormalised * handleRangeX) + handleRangeX - 39; // sets range
		Overlays.editOverlay(applyMotionHandle, { x: handleX + minHandleX} );
		return;
	}
	else if(movingHandEffectSlider) {
		var thumbClickOffsetX = event.x - minSliderX;
		var thumbPositionNormalised = thumbClickOffsetX / sliderRangeX;
		if(thumbPositionNormalised<0) thumbPositionNormalised = 0;
		if(thumbPositionNormalised>1) thumbPositionNormalised = 1;
		handEffect = (thumbPositionNormalised-0.08) * jointEffectMax;
		var sliderX = thumbPositionNormalised * sliderRangeX ; // sets range
		Overlays.editOverlay(handEffectSlider, { x: sliderX + minSliderX} );
	}
	else if(movingForearmEffectSlider) {
		var thumbClickOffsetX = event.x - minSliderX;
		var thumbPositionNormalised = thumbClickOffsetX / sliderRangeX;
		if(thumbPositionNormalised<0) thumbPositionNormalised = 0;
		if(thumbPositionNormalised>1) thumbPositionNormalised = 1;
		forearmEffect = (thumbPositionNormalised-0.1) * jointEffectMax;
		var sliderX = thumbPositionNormalised * sliderRangeX ; // sets range
		Overlays.editOverlay(forearmEffectSlider, { x: sliderX + minSliderX} );
	}
	else if(movingLimbWeightSlider) {
		var thumbClickOffsetX = event.x - minSliderX;
		var thumbPositionNormalised = thumbClickOffsetX / sliderRangeX;
		if(thumbPositionNormalised<0) thumbPositionNormalised = 0;
		if(thumbPositionNormalised>1) thumbPositionNormalised = 1;
		limbWeight = thumbPositionNormalised * weightMax;
		if(limbWeight<weightMin) limbWeight = weightMin;
		upperArmPDampingFilter.length = parseInt(limbWeight);
		var sliderX = thumbPositionNormalised * sliderRangeX ; // sets range
		Overlays.editOverlay(limbWeightSlider, { x: sliderX + minSliderX} );
	}
	else if(movingEffectStrengthSlider) {
		var thumbClickOffsetX = event.x - minSliderX;
		var thumbPositionNormalised = thumbClickOffsetX / sliderRangeX;
		if(thumbPositionNormalised<0) thumbPositionNormalised = 0;
		if(thumbPositionNormalised>1) thumbPositionNormalised = 1;
		effectStrength = thumbPositionNormalised;
		var sliderX = thumbPositionNormalised * sliderRangeX ; // sets range
		Overlays.editOverlay(effectStrengthSlider, { x: sliderX + minSliderX} );
		return;
	}
}
function mouseReleaseEvent(event) {

	if(movingHandle) {
		movingHandle = false;
		handleValue = 0;
		Overlays.editOverlay(applyMotionHandle, { x: minHandleX+handleRangeX-39} );
	}
	else if(movingHandEffectSlider) movingHandEffectSlider = false;
	else if(movingForearmEffectSlider) movingForearmEffectSlider = false;
	else if(movingLimbWeightSlider) movingLimbWeightSlider = false;
	else if(movingEffectStrengthSlider) movingEffectStrengthSlider = false;
	else if(movingDampingSlider) movingDampingSlider = false;
}

// set up mouse and keyboard callbacks
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
Controller.keyPressEvent.connect(keyPressEvent);

// keyboard command
function keyPressEvent(event) {

	if (event.text == "q") {
		print('hand effect = ' + handEffect + '\n');
        print('forearmEffect = ' + forearmEffect + '\n');
        print('limbWeight = ' + limbWeight + '\n');
        print('effectStrength = ' + effectStrength + '\n');
    }
    else if (event.text == "r") {
		animateSelf = !animateSelf;
    }
    else if (event.text == "[") {
		selfAnimateFrequency+=0.5
		print('selfAnimateFrequency = '+selfAnimateFrequency);
	}
	else if (event.text == "]") {
			selfAnimateFrequency-=0.5
			print('selfAnimateFrequency = '+selfAnimateFrequency);
	}
}


// zero out all joints
function resetJoints() {

    var avatarJointNames = MyAvatar.getJointNames();
    for (var i = 0; i < avatarJointNames.length; i++) {
        MyAvatar.clearJointData(avatarJointNames[i]);
    }
}

// Script ending
Script.scriptEnding.connect(function() {

	// delete the overlays
    Overlays.deleteOverlay(controllerBackground);
    Overlays.deleteOverlay(controllerRadioSelectedBackground);
    Overlays.deleteOverlay(handEffectSlider);
    Overlays.deleteOverlay(forearmEffectSlider);
    Overlays.deleteOverlay(limbWeightSlider);
    Overlays.deleteOverlay(effectStrengthSlider);
    Overlays.deleteOverlay(applyMotionHandle);

    // leave the avi in zeroed out stance
    resetJoints();
});


// animation  / joint data. animation keyframes produced using davedub's procedural animation controller
MyAvatar.setJointData("LeftArm", Quat.fromPitchYawRollDegrees(80,0,0)); // stand nicely please
var keyFrameOne = {"name":"FemaleStandingOne","settings":{"baseFrequency":70,"flyingHipsPitch":60,"takeFlightVelocity":40,"maxBankingAngle":40},"adjusters":{"legsSeparation":{"strength":-0.03679245283018867,"separationAngle":50},"stride":{"strength":0,"upperLegsPitch":30,"lowerLegsPitch":15,"upperLegsPitchOffset":0.2,"lowerLegsPitchOffset":1.5}},"joints":[{"name":"hips","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0,"thrust":0,"bob":0,"sway":0,"thrustPhase":180,"bobPhase":0,"swayPhase":-90,"thrustOffset":0,"bobOffset":0,"swayOffset":0},{"name":"upperLegs","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"lowerLegs","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"feet","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"toes","pitch":2.0377358490566038,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":4.415094339622641,"yawOffset":0,"rollOffset":0},{"name":"spine","pitch":1.660377358490566,"yaw":0,"roll":0,"pitchPhase":-180,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"spine1","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"spine2","pitch":2.1132075471698113,"yaw":0,"roll":0,"pitchPhase":-0.6792452830188722,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"shoulders","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0.6792452830188678,"yawOffset":-5.20754716981132,"rollOffset":-2.9433962264150937},{"name":"upperArms","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":77.77358490566039,"yawOffset":9.169811320754715,"rollOffset":0},{"name":"lowerArms","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"hands","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":1.6981132075471694,"yawOffset":-1.0188679245283017,"rollOffset":1.0188679245283017},{"name":"head","pitch":0,"yaw":1.7358490566037734,"roll":1.5094339622641508,"pitchPhase":-90.33962264150944,"yawPhase":94.41509433962267,"rollPhase":0,"pitchOffset":1.6981132075471694,"yawOffset":0,"rollOffset":0}]};
var keyFrameTwo = {"name":"FemaleStandingOne","settings":{"baseFrequency":70,"flyingHipsPitch":60,"takeFlightVelocity":40,"maxBankingAngle":40},"adjusters":{"legsSeparation":{"strength":-0.03679245283018867,"separationAngle":50},"stride":{"strength":0,"upperLegsPitch":30,"lowerLegsPitch":15,"upperLegsPitchOffset":0.2,"lowerLegsPitchOffset":1.5}},"joints":[{"name":"hips","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0,"thrust":0,"bob":0,"sway":0,"thrustPhase":180,"bobPhase":0,"swayPhase":-90,"thrustOffset":0,"bobOffset":0,"swayOffset":0},{"name":"upperLegs","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"lowerLegs","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"feet","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"toes","pitch":2.0377358490566038,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":4.415094339622641,"yawOffset":0,"rollOffset":0},{"name":"spine","pitch":1.660377358490566,"yaw":0,"roll":0,"pitchPhase":-180,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"spine1","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"spine2","pitch":2.1132075471698113,"yaw":0,"roll":0,"pitchPhase":-0.6792452830188722,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"shoulders","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0.6792452830188678,"yawOffset":-5.20754716981132,"rollOffset":-2.9433962264150937},{"name":"upperArms","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":49.584905660377345,"yawOffset":9.169811320754715,"rollOffset":0},{"name":"lowerArms","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"hands","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":-13,"yawOffset":-1.0188679245283017,"rollOffset":1.0188679245283017},{"name":"head","pitch":0,"yaw":1.7358490566037734,"roll":1.5094339622641508,"pitchPhase":-90.33962264150944,"yawPhase":94.41509433962267,"rollPhase":0,"pitchOffset":1.6981132075471694,"yawOffset":0,"rollOffset":0}]};
var keyFrameThree = {"name":"FemaleStandingOne","settings":{"baseFrequency":70,"flyingHipsPitch":60,"takeFlightVelocity":40,"maxBankingAngle":40},"adjusters":{"legsSeparation":{"strength":-0.03679245283018867,"separationAngle":50},"stride":{"strength":0,"upperLegsPitch":30,"lowerLegsPitch":15,"upperLegsPitchOffset":0.2,"lowerLegsPitchOffset":1.5}},"joints":[{"name":"hips","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0,"thrust":0,"bob":0,"sway":0,"thrustPhase":180,"bobPhase":0,"swayPhase":-90,"thrustOffset":0,"bobOffset":0,"swayOffset":0},{"name":"upperLegs","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"lowerLegs","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"feet","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"toes","pitch":2.0377358490566038,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":4.415094339622641,"yawOffset":0,"rollOffset":0},{"name":"spine","pitch":1.660377358490566,"yaw":0,"roll":0,"pitchPhase":-180,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"spine1","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"spine2","pitch":2.1132075471698113,"yaw":0,"roll":0,"pitchPhase":-0.6792452830188722,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"shoulders","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":-21.0566037735849,"yawOffset":-5.20754716981132,"rollOffset":-2.9433962264150937},{"name":"upperArms","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":-33.28301886792452,"yawOffset":9.169811320754715,"rollOffset":0},{"name":"lowerArms","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":0,"yawOffset":0,"rollOffset":0},{"name":"hands","pitch":0,"yaw":0,"roll":0,"pitchPhase":0,"yawPhase":0,"rollPhase":0,"pitchOffset":-13,"yawOffset":-1.0188679245283017,"rollOffset":1.0188679245283017},{"name":"head","pitch":0,"yaw":1.7358490566037734,"roll":1.5094339622641508,"pitchPhase":-90.33962264150944,"yawPhase":94.41509433962267,"rollPhase":0,"pitchOffset":1.6981132075471694,"yawOffset":0,"rollOffset":0}]};