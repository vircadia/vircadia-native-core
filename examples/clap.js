//
//  clap.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//
//  This sample script watches your hydra hands and makes clapping sound when they come close together fast, 
//  and also watches for the 'shift' key and claps when that key is pressed.  Clapping multiple times by pressing 
//  the shift key again makes the animation and sound match your pace of clapping. 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("libraries/globals.js");

var clapAnimation = HIFI_PUBLIC_BUCKET + "animations/ClapAnimations/ClapHands_Standing.fbx";
var ANIMATION_FRAMES_PER_CLAP = 10.0;
var startEndFrames = [];
startEndFrames.push({ start: 0, end: 10});
startEndFrames.push({ start: 10, end: 20});
startEndFrames.push({ start: 20, end: 30});
startEndFrames.push({ start: 30, end: 40});
startEndFrames.push({ start: 41, end: 51});
startEndFrames.push({ start: 53, end: 0});

var lastClapFrame = 0;
var lastAnimFrame = 0;

var claps = [];
claps.push(SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/claps/BClap1Rvb.wav"));
claps.push(SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/claps/BClap2Rvb.wav"));
claps.push(SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/claps/BClap3Rvb.wav"));
claps.push(SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/claps/BClap4Rvb.wav"));
claps.push(SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/claps/BClap5Rvb.wav"));
claps.push(SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/claps/BClap6Rvb.wav"));
claps.push(SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/claps/BClap7Rvb.wav"));
claps.push(SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/claps/BClap8Rvb.wav"));
claps.push(SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/claps/BClap9Rvb.wav"));
claps.push(SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/claps/BClap10Rvb.wav"));
var numberOfSounds = claps.length;

var clappingNow = false;
var collectedClicks = 0;

var clickStartTime, clickEndTime;
var clickClappingNow = false; 
var CLAP_START_RATE = 15.0;
var clapRate = CLAP_START_RATE; 
var startedTimer = false; 

function maybePlaySound(deltaTime) {
	//  Set the location and other info for the sound to play

	var animationDetails = MyAvatar.getAnimationDetails(clapAnimation);

	var frame = Math.floor(animationDetails.frameIndex);

	if (frame != lastAnimFrame) {
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
	var clip = Math.floor(Math.random() * numberOfSounds);
	Audio.playSound(claps[clip], {
	  position: position,
    volume: volume
	});
}

var FASTEST_CLAP_INTERVAL = 150.0;
var SLOWEST_CLAP_INTERVAL = 750.0;

Controller.keyPressEvent.connect(function(event) {
	if(event.text == "SHIFT") {
		if (!clickClappingNow) {
			clickClappingNow = true;
			clickStartTime = new Date();
			lastClapFrame = 0;
		} else {
			//  start or adjust clapping speed based on the duration between clicks
			clickEndTime = new Date();
			var milliseconds = Math.max(clickEndTime - clickStartTime, FASTEST_CLAP_INTERVAL);
			clickStartTime = new Date();
			if (milliseconds < SLOWEST_CLAP_INTERVAL) {
				clapRate = ANIMATION_FRAMES_PER_CLAP * (1000.0 / milliseconds);
				playClap(1.0, Camera.getPosition());
				MyAvatar.stopAnimation(clapAnimation);
				MyAvatar.startAnimation(clapAnimation, clapRate, 1.0, true, false);
			}
			collectedClicks = collectedClicks + 1;
		}
	}
});

var CLAP_END_WAIT_MSECS = 300;
Controller.keyReleaseEvent.connect(function(event) {
    if (event.text == "SHIFT") {
    	collectedClicks = 0;
    	if (!startedTimer) {
    		collectedClicks = 0;
    		Script.setTimeout(stopClapping, CLAP_END_WAIT_MSECS);
    		startedTimer = true;
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