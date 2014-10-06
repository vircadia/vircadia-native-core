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

Script.include("libraries/globals.js");

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

//  The model file to be used for the guitar 
var guitarModel = HIFI_PUBLIC_BUCKET + "models/attachments/guitar.fst";

//  Load sounds that will be played

var chords = new Array();
// Nylon string guitar
chords[1] = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Nylon+A.raw");
chords[2] = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Nylon+B.raw");
chords[3] = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Nylon+E.raw");
chords[4] = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Nylon+G.raw");

// Electric guitar
chords[5] = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Metal+A+short.raw");
chords[6] = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Metal+B+short.raw");
chords[7] = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Metal+E+short.raw");
chords[8] = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Metal+G+short.raw");

//  Steel Guitar 
chords[9] = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Steel+A.raw");
chords[10] = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Steel+B.raw");
chords[11] = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Steel+E.raw");
chords[12] = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Steel+G.raw");

var NUM_CHORDS = 4;
var NUM_GUITARS = 3;
var guitarSelector = NUM_CHORDS;
var whichChord = 1;

var leftHanded = true; 
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
var position;

MyAvatar.attach(guitarModel, "Hips", {x: -0.0, y: -0.0, z: 0.0}, Quat.fromPitchYawRollDegrees(0, 0, 0), 1.0);

function checkHands(deltaTime) {
	for (var palm = 0; palm < 2; palm++) {
		var palmVelocity = Controller.getSpatialControlVelocity(palm * 2 + 1);
		var volume = length(palmVelocity) / 5.0;
		var position = Controller.getSpatialControlPosition(palm * 2 + 1);
		var myPelvis = MyAvatar.position;
		var trigger = Controller.getTriggerValue(strumHand);
		var chord = Controller.getTriggerValue(chordHand);

		if (volume > 1.0) volume = 1.0;
		if ((chord > 0.1) && Audio.isInjectorPlaying(soundPlaying)) {
			// If chord finger trigger pulled, stop current chord
			print("stopped sound");
			Audio.stopInjector(soundPlaying);
		}

		var BUTTON_COUNT = 6;

		//  Change guitars if button FWD (5) pressed
		if (Controller.isButtonPressed(chordHand * BUTTON_COUNT + 5)) {
			if (!selectorPressed) {
				guitarSelector += NUM_CHORDS;
				if (guitarSelector >= NUM_CHORDS * NUM_GUITARS) {
					guitarSelector = 0;
				} 
				selectorPressed = true;
			}
		} else {
			selectorPressed = false;
		}

		if (Controller.isButtonPressed(chordHand * BUTTON_COUNT + 1)) {
			whichChord = 1;
		} else if (Controller.isButtonPressed(chordHand * BUTTON_COUNT + 2)) {
			whichChord = 2;
		} else if (Controller.isButtonPressed(chordHand * BUTTON_COUNT + 3)) {
			whichChord = 3;
		} else if (Controller.isButtonPressed(chordHand * BUTTON_COUNT + 4)) {
		  	whichChord = 4;
		}

		if (palm == strumHand) {

			var STRUM_HEIGHT_ABOVE_PELVIS = 0.10;
			var strumTriggerHeight = myPelvis.y + STRUM_HEIGHT_ABOVE_PELVIS;
			//printVector(position);
			if ( ( ((position.y < strumTriggerHeight) && (lastPosition.y >= strumTriggerHeight)) ||
			       ((position.y > strumTriggerHeight) && (lastPosition.y <= strumTriggerHeight)) ) && (trigger > 0.1) ){
				// If hand passes downward or upward through 'strings', and finger trigger pulled, play
				playChord(position, volume);
			}
			lastPosition = Controller.getSpatialControlPosition(palm * 2 + 1);
		} 
	}
}

function playChord(position, volume) {
	var options = new AudioInjectionOptions();
	options.position = position;
	options.volume = volume;
	if (Audio.isInjectorPlaying(soundPlaying)) {
		print("stopped sound");
		Audio.stopInjector(soundPlaying);
	}
	print("Played sound: " + whichChord + " at volume " + options.volume);
	soundPlaying = Audio.playSound(chords[guitarSelector + whichChord], options);	
}

function keyPressEvent(event) {
    // check for keypresses and use those to trigger sounds if not hydra
    keyVolume = 0.4;
    if (event.text == "1") {
        whichChord = 1;
        playChord(MyAvatar.position, keyVolume);
    } else if (event.text == "2") {
    	whichChord = 2;
    	playChord(MyAvatar.position, keyVolume);
    } else if (event.text == "3") {
    	whichChord = 3;
    	playChord(MyAvatar.position, keyVolume);
    } else if (event.text == "4") {
        whichChord = 4;
    	playChord(MyAvatar.position, keyVolume);
    }
}

function scriptEnding() {
    MyAvatar.detachOne(guitarModel);
}
// Connect a call back that happens every frame
Script.update.connect(checkHands);
Script.scriptEnding.connect(scriptEnding);
Controller.keyPressEvent.connect(keyPressEvent);

