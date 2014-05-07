//
//  airGuitar.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//
//  This example musical instrument script plays guitar chords based on a strum motion and hand position
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function length(v) {
    return Math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}


function printVector(v) {
	print(v.x + ", " + v.y + ", " + v.z);
	return;
}

function vMinus(a, b) { 
	var rval = { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z };
	return rval;
}

//  First, load two percussion sounds to be used on the sticks

var chord1 = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Guitars/Guitar+-+Nylon+A.raw");
var chord2 = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Guitars/Guitar+-+Nylon+B.raw");
var chord3 = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Guitars/Guitar+-+Nylon+E.raw");

var whichChord = chord1; 

var leftHanded = false; 
if (leftHanded) {
	var strumHand = 0;
	var chordHand = 1; 
} else {
	var strumHand = 1;
	var chordHand = 0;
}

var lastPosition = { x: 0.0,
                 y: 0.0, 
                 z: 0.0 }; 


function checkHands(deltaTime) {
	for (var palm = 0; palm < 2; palm++) {
		var palmVelocity = Controller.getSpatialControlVelocity(palm * 2 + 1);
		var speed = length(palmVelocity);
		var position = Controller.getSpatialControlPosition(palm * 2 + 1);
		var myPelvis = MyAvatar.position;

		if (palm == strumHand) {

			var STRUM_HEIGHT_ABOVE_PELVIS = 0.15;
			var strumTriggerHeight = myPelvis.y + STRUM_HEIGHT_ABOVE_PELVIS;
			//printVector(position);
			if ((position.y < strumTriggerHeight) && (lastPosition.y >= strumTriggerHeight)) {
				// If hand passes downward through guitar strings, play a chord!
				var options = new AudioInjectionOptions();
				options.position = position;
				if (speed > 1.0) { speed = 1.0; }
				options.volume = speed;
				Audio.playSound(whichChord, options);
			}
			lastPosition = Controller.getSpatialControlPosition(palm * 2 + 1);
		} else {
			//  This is the chord controller
			var distanceFromPelvis = Vec3.length(Vec3.subtract(position, myPelvis));
			//print(distanceFromPelvis);
			if (distanceFromPelvis > 0.50) {
				whichChord = chord3;
			} else if (distanceFromPelvis > 0.35) {
				whichChord = chord2;
			} else {
				whichChord = chord1;
			}
		}
	}
}

// Connect a call back that happens every frame
Script.update.connect(checkHands);