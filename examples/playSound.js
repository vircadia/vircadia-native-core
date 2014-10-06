//
//  playSound.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//  Plays a sample audio file at the avatar's current location
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

Script.include("libraries/globals.js");

//  First, load a sample sound from a URL
var bird = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Animals/bushtit_1.raw");

function maybePlaySound(deltaTime) {
	if (Math.random() < 0.01) {
		//  Set the location and other info for the sound to play
		var options = new AudioInjectionOptions();
		var position = MyAvatar.position; 
		options.position = position;
		options.volume = 0.5;
		Audio.playSound(bird, options);
	}
}

// Connect a call back that happens every frame
Script.update.connect(maybePlaySound);