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

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

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

var heyManWave = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/sounds/KenDoll_1%2303.wav");

var chords = new Array();
// Nylon string guitar
chords[1] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Nylon+A.raw");
chords[2] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Nylon+B.raw");
chords[3] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Nylon+E.raw");
chords[4] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Nylon+G.raw");

// Electric guitar
chords[5] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Metal+A+short.raw");
chords[6] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Metal+B+short.raw");
chords[7] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Metal+E+short.raw");
chords[8] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Metal+G+short.raw");

//  Steel Guitar
chords[9] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Steel+A.raw");
chords[10] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Steel+B.raw");
chords[11] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Steel+E.raw");
chords[12] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Guitars/Guitar+-+Steel+G.raw");

var NUM_CHORDS = 4;
var NUM_GUITARS = 3;
var guitarSelector = NUM_CHORDS;
var whichChord = 1;

var leftHanded = false;
var strumHand, chordHand, strumTrigger, chordTrigger;
if (leftHanded) {
    strumHand = Controller.Standard.LeftHand;
    chordHand = Controller.Standard.RightHand;
    strumTrigger = Controller.Standard.LT;
    chordTrigger = Controller.Standard.RT;
    changeGuitar = Controller.Standard.RB;
    chord1 = Controller.Standard.X;
    chord2 = Controller.Standard.Y;
    chord3 = Controller.Standard.A;
    chord4 = Controller.Standard.B;
} else {
    strumHand = Controller.Standard.RightHand;
    chordHand = Controller.Standard.LeftHand;
    strumTrigger = Controller.Standard.RT;
    chordTrigger = Controller.Standard.LT;
    changeGuitar = Controller.Standard.LB;
    chord1 = Controller.Standard.DU; // these may not be correct, maybe we should map directly to Hydra??
    chord2 = Controller.Standard.DD;
    chord3 = Controller.Standard.DL;
    chord4 = Controller.Standard.DR;
}

var lastPosition = { x: 0.0,
                 y: 0.0,
                 z: 0.0 };

var audioInjector = null;
var selectorPressed = false;
var position;

MyAvatar.attach(guitarModel, "Hips", {x: leftHanded ? -0.2 : 0.2, y: 0.0, z: 0.1}, Quat.fromPitchYawRollDegrees(90, 00, leftHanded ? 75 : -75), 1.0);

function checkHands(deltaTime) {
    var strumVelocity = Controller.getPoseValue(strumHand).velocity;
    var volume = length(strumVelocity) / 5.0;

    if (volume == 0.0) {
        volume = 1.0;
    }

    var strumHandPosition = leftHanded ? MyAvatar.leftHandPosition : MyAvatar.rightHandPosition;
    var myPelvis = MyAvatar.position;
    var strumming = Controller.getValue(strumTrigger);
    var chord = Controller.getValue(chordTrigger);

    if (volume > 1.0) volume = 1.0;
    if ((chord > 0.1) && audioInjector && audioInjector.playing) {
        // If chord finger trigger pulled, stop current chord
        print("stopping chord because cord trigger pulled");
        audioInjector.stop();
    }

    //  Change guitars if button FWD (5) pressed
    if (Controller.getValue(changeGuitar)) {
        if (!selectorPressed) {
            print("changeGuitar:" + changeGuitar);
            guitarSelector += NUM_CHORDS;
            if (guitarSelector >= NUM_CHORDS * NUM_GUITARS) {
                guitarSelector = 0;
            }
            print("new guitarBase: " + guitarSelector);
            stopAudio(true);
            selectorPressed = true;
        }
    } else {
        selectorPressed = false;
    }

    if (Controller.getValue(chord1)) {
        whichChord = 1;
        stopAudio(true);
    } else if (Controller.getValue(chord2)) {
        whichChord = 2;
        stopAudio(true);
    } else if (Controller.getValue(chord3)) {
        whichChord = 3;
        stopAudio(true);
    } else if (Controller.getValue(chord4)) {
        whichChord = 4;
        stopAudio(true);
    }

    var STRUM_HEIGHT_ABOVE_PELVIS = 0.10;
    var strummingHeight = myPelvis.y + STRUM_HEIGHT_ABOVE_PELVIS;

    var strumNowAbove = strumHandPosition.y > strummingHeight;
    var strumNowBelow = strumHandPosition.y <= strummingHeight;
    var strumWasAbove = lastPosition.y > strummingHeight;
    var strumWasBelow = lastPosition.y <= strummingHeight;
    var strumUp = strumNowAbove && strumWasBelow;
    var strumDown = strumNowBelow && strumWasAbove;

    if ((strumUp || strumDown) && (strumming > 0.1)) {
        // If hand passes downward or upward through 'strings', and finger trigger pulled, play
        playChord(strumHandPosition, volume);
    }
    lastPosition = strumHandPosition;
}

function stopAudio(killInjector) {
    if (audioInjector && audioInjector.playing) {
        print("stopped sound");
        audioInjector.stop();
    }
    if (killInjector) {
        audioInjector = null;
    }
}


function playChord(position, volume) {
    stopAudio();
    print("Played sound: " + whichChord + " at volume " + volume);
    if (!audioInjector) {
        var index = guitarSelector + whichChord;
        var chord = chords[guitarSelector + whichChord];
        audioInjector = Audio.playSound(chord, { position: position, volume: volume });
    } else {
        audioInjector.restart();
    }
}

function keyPressEvent(event) {
    // check for keypresses and use those to trigger sounds if not hydra
    keyVolume = 0.4;
    if (event.text == "1") {
        whichChord = 1;
        stopAudio(true);
        playChord(MyAvatar.position, keyVolume);
    } else if (event.text == "2") {
        whichChord = 2;
        stopAudio(true);
        playChord(MyAvatar.position, keyVolume);
    } else if (event.text == "3") {
        whichChord = 3;
        stopAudio(true);
        playChord(MyAvatar.position, keyVolume);
    } else if (event.text == "4") {
        whichChord = 4;
        stopAudio(true);
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
