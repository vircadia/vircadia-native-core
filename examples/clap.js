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

function length(v) {
    return Math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}


function printVector(v) {
	print(v.x + ", " + v.y + ", " + v.z + "\n");
}

function vMinus(a, b) { 
	var rval = { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z };
	return rval;
}

//  First, load the clap sound from a URL 
var clap1 = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/claps/clap1.raw");
var clap2 = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/claps/clap2.raw");
var clap3 = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/claps/clap3.raw");
var clap4 = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/claps/clap4.raw");
var clap5 = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/claps/clap5.raw");
var clap6 = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/claps/clap6.raw");

var clapping = new Array();
clapping[0] = false; 
clapping[1] = false; 

function maybePlaySound(deltaTime) {
	//  Set the location and other info for the sound to play
	var palm1Position = Controller.getSpatialControlPosition(0);
	var palm2Position = Controller.getSpatialControlPosition(2);
	var distanceBetween = length(vMinus(palm1Position, palm2Position));

	for (var palm = 0; palm < 2; palm++) {
		var palmVelocity = Controller.getSpatialControlVelocity(palm * 2 + 1);
		var speed = length(palmVelocity);
		
		const CLAP_SPEED = 0.2;
		const CLAP_DISTANCE = 0.2; 

    	if (!clapping[palm] && (distanceBetween < CLAP_DISTANCE) && (speed > CLAP_SPEED)) {
    		var options = new AudioInjectionOptions();
			options.position = palm1Position;
			options.volume = speed / 2.0;
			if (options.volume > 1.0) options.volume = 1.0;
			which = Math.floor((Math.random() * 6) + 1);
			if (which == 1) {  Audio.playSound(clap1, options);  }
			else if (which == 2) { Audio.playSound(clap2, options); }
			else if (which == 3) { Audio.playSound(clap3, options); }
			else if (which == 4) { Audio.playSound(clap4, options); }
			else if (which == 5) { Audio.playSound(clap5, options); }
			else { Audio.playSound(clap6, options); }
			Audio.playSound(clap, options);
			clapping[palm] = true; 
    	} else if (clapping[palm] && (speed < (CLAP_SPEED / 4.0))) {
    		clapping[palm] = false;
    	}
	}
}

// Connect a call back that happens every frame
Script.update.connect(maybePlaySound);