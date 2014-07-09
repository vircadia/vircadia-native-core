//
//  cameraExample.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//
//  This sample script watches your hydra hands and makes clapping sound when they come close together fast 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var clapAnimation = "https://s3-us-west-1.amazonaws.com/highfidelity-public/animations/ClapAnimations/ClapHands_Standing.fbx";
var startEndFrames = [];
startEndFrames.push({ start: 0, end: 8});
startEndFrames.push({ start: 10, end: 20});
startEndFrames.push({ start: 20, end: 28});
startEndFrames.push({ start: 30, end: 37});
startEndFrames.push({ start: 41, end: 46});
startEndFrames.push({ start: 53, end: 58});

var lastClapFrame = 0;
var lastAnimFrame = 0;

var claps = [];
claps.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/claps/Clap1Reverb.wav"));
claps.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/claps/Clap2Reverb.wav"));
claps.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/claps/Clap3Reverb.wav"));
claps.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/claps/Clap4Reverb.wav"));
claps.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/claps/Clap5Reverb.wav"));
claps.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/claps/Clap6Reverb.wav"));
var numberOfSounds = claps.length;

var clappingNow = false;
var collectedClicks = 0;

var clickClappingNow = false; 
var CLAP_START_RATE = 15.0;
var clapRate = CLAP_START_RATE; 
var startedTimer = false; 

var clapping = new Array();
clapping[0] = false; 
clapping[1] = false; 

function maybePlaySound(deltaTime) {
	//  Set the location and other info for the sound to play

	var animationDetails = MyAvatar.getAnimationDetails(clapAnimation);

	var frame = Math.floor(animationDetails.frameIndex);

	if (frame != lastAnimFrame) {
		print("frame " + frame);
		lastAnimFrame = frame;
	}
	for (var i = 0; i < startEndFrames.length; i++) {
		if (frame == startEndFrames[i].start && (frame != lastClapFrame)) {
			playClap(1.0, Camera.getPosition());
			lastClapFrame = frame; 
		}
	}

	var palm1Position = MyAvatar.getLeftPalmPosition();
	var palm2Position = MyAvatar.getRightPalmPosition();
	var distanceBetween = Vec3.length(Vec3.subtract(palm1Position, palm2Position));

	var palm1Velocity = Controller.getSpatialControlVelocity(1);
	var palm2Velocity = Controller.getSpatialControlVelocity(3);
	var closingVelocity = Vec3.length(Vec3.subtract(palm1Velocity, palm2Velocity));

	const CLAP_SPEED = 0.7;
	const CLAP_DISTANCE = 0.15; 

	if ((closingVelocity > CLAP_SPEED) && (distanceBetween < CLAP_DISTANCE) && !clappingNow) {
		var volume = closingVelocity / 2.0;
		if (volume > 1.0) volume = 1.0;
		playClap(volume, palm1Position);
		clappingNow = true; 
	} else if (clappingNow && (distanceBetween > CLAP_DISTANCE * 1.2)) {
		clappingNow = false;
	}
}

function playClap(volume, position) {
	var options = new AudioInjectionOptions();
	options.position = position;
	options.volume = 1.0;
	var clip = Math.floor(Math.random() * numberOfSounds);
	Audio.playSound(claps[clip], options);
}

function keepClapping() {
	playClap(1.0, Camera.getPosition());
} 

Controller.keyPressEvent.connect(function(event) {
	if(event.text == "SHIFT") {
		if (!clickClappingNow) {
			playClap(1.0, Camera.getPosition());
			var whichClip = Math.floor(Math.random() * startEndFrames.length);
			lastClapFrame = 0;
			MyAvatar.startAnimation(clapAnimation, clapRate, 1.0, true, false);
			clickClappingNow = true;
		} else {
			clapRate *= 1.25;
			MyAvatar.stopAnimation(clapAnimation);
			MyAvatar.startAnimation(clapAnimation, clapRate, 1.0, true, false);
			collectedClicks = collectedClicks + 1;
		}
	}
});

var CLAP_END_WAIT_MSECS = 500;
Controller.keyReleaseEvent.connect(function(event) {
    if (event.text == "SHIFT") {
    	if (!startedTimer) {
   		startedTimer = true;
    		collectedClicks = 0;
    		Script.setTimeout(stopClapping, CLAP_END_WAIT_MSECS);
    	}
    }
});

function stopClapping() {
	if (collectedClicks == 0) {
		startedTimer = false;
	   	MyAvatar.stopAnimation(clapAnimation);
       	clapRate = CLAP_START_RATE;
       	clickClappingNow = false;
	}  else {
		startedTimer = false;
	} 
}

// Connect a call back that happens every frame
Script.update.connect(maybePlaySound);
//Controller.keyPressEvent.connect(keyPressEvent);