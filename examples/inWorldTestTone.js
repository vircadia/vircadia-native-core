//
//  inWorldTestTone.js
//  
//
//  Created by Philip Rosedale on 5/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This example script plays a test tone that is useful for debugging audio dropout.  220Hz test tone played at the domain origin.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("libraries/globals.js");

var sound = new Sound(HIFI_PUBLIC_BUCKET + "sounds/220Sine.wav");

var soundPlaying = false;

function update(deltaTime) {
    if (!Audio.isInjectorPlaying(soundPlaying)) {
        soundPlaying = Audio.playSound(sound, {
          loop: true
        });
        print("Started sound loop");
    } 
}

function scriptEnding() {
    if (Audio.isInjectorPlaying(soundPlaying)) {
        Audio.stopInjector(soundPlaying);
        print("Stopped sound loop");
    }
}

Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);

