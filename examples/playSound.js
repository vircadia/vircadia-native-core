//
//  This sample script loads a sound file and plays it at the 'fingertip' of the 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

//  First, load the clap sound from a URL 
var clap = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/bushtit_1.raw");	

function maybePlaySound(deltaTime) {
	if (Math.random() < 0.01) {
		//  Set the location and other info for the sound to play
		var options = new AudioInjectionOptions(); 
		var palmPosition = Controller.getSpatialControlPosition(0);
		options.position = palmPosition;
		options.volume = 0.5;
		Audio.playSound(clap, options);
	}
}

// Connect a call back that happens every frame
Script.update.connect(maybePlaySound);