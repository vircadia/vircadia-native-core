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

//  Load sounds that will be played

var chords = new Array();
// Nylon string guitar
chords[1] = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Guitars/Guitar+-+Nylon+A.raw");
chords[2] = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Guitars/Guitar+-+Nylon+B.raw");
chords[3] = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Guitars/Guitar+-+Nylon+E.raw");
// Electric guitar
chords[4] = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Guitars/Guitar+-+Metal+A+short.raw");
chords[5] = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Guitars/Guitar+-+Metal+B+short.raw");
chords[6] = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Guitars/Guitar+-+Metal+E+short.raw");

var guitarSelector = 3;

var whichChord = chords[guitarSelector + 1]; 

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

var soundPlaying = false; 
var selectorPressed = false;

function checkHands(deltaTime) {
	for (var palm = 0; palm < 2; palm++) {
		var palmVelocity = Controller.getSpatialControlVelocity(palm * 2 + 1);
		var speed = length(palmVelocity) / 4.0;
		var position = Controller.getSpatialControlPosition(palm * 2 + 1);
		var myPelvis = MyAvatar.position;
		var trigger = Controller.getTriggerValue(strumHand);
		var chord = Controller.getTriggerValue(chordHand);

		if ((chord > 0.1) && Audio.isInjectorPlaying(soundPlaying)) {
			// If chord finger trigger pulled, stop current chord
			Audio.stopInjector(soundPlaying);
		}

		var BUTTON_COUNT = 6;

		//  Change guitars if button FWD (5) pressed
		if (Controller.isButtonPressed(chordHand * BUTTON_COUNT + 5)) {
			if (!selectorPressed) {
				if (guitarSelector == 0) {
					guitarSelector = 3;
				} else {
					guitarSelector = 0;
				}
				selectorPressed = true;
			}
		} else {
			selectorPressed = false;
		}

		if (Controller.isButtonPressed(chordHand * BUTTON_COUNT + 1)) {
			whichChord = chords[guitarSelector + 1];
		} else if (Controller.isButtonPressed(chordHand * BUTTON_COUNT + 2)) {
			whichChord = chords[guitarSelector + 2];
		} else if (Controller.isButtonPressed(chordHand * BUTTON_COUNT + 3)) {
			whichChord = chords[guitarSelector + 3];
		}

		if (palm == strumHand) {

			var STRUM_HEIGHT_ABOVE_PELVIS = 0.00;
			var strumTriggerHeight = myPelvis.y + STRUM_HEIGHT_ABOVE_PELVIS;
			//printVector(position);
			if ( ( ((position.y < strumTriggerHeight) && (lastPosition.y >= strumTriggerHeight)) ||
			       ((position.y > strumTriggerHeight) && (lastPosition.y <= strumTriggerHeight)) ) && (trigger > 0.1) ){
				// If hand passes downward or upward through 'strings', and finger trigger pulled, play
				var options = new AudioInjectionOptions();
				options.position = position;
				if (speed > 1.0) { speed = 1.0; }
				options.volume = speed;
				if (Audio.isInjectorPlaying(soundPlaying)) {
					Audio.stopInjector(soundPlaying);
				}
				soundPlaying = Audio.playSound(whichChord, options);	
			}
			lastPosition = Controller.getSpatialControlPosition(palm * 2 + 1);
		} 
	}
}

// Connect a call back that happens every frame
Script.update.connect(checkHands);