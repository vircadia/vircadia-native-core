//
//  playSoundLoop.js
//  examples
//
//  Created by David Rowe on 5/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This example script plays a sound in a continuous loop.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var sound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Guitars/Guitar+-+Nylon+A.raw");

var soundPlaying = false;

function keyPressEvent(event) {
    if (event.text === "1") {
        if (!Audio.isInjectorPlaying(soundPlaying)) {
            var options = new AudioInjectionOptions();
            options.position = MyAvatar.position;
            options.volume = 0.5;
            options.loop = true;
            soundPlaying = Audio.playSound(sound, options);
            print("Started sound loop");
        } else {
            Audio.stopInjector(soundPlaying);
            print("Stopped sound loop");
        }
    }
}

function scriptEnding() {
    if (Audio.isInjectorPlaying(soundPlaying)) {
        Audio.stopInjector(soundPlaying);
        print("Stopped sound loop");
    }
}

// Connect a call back that happens every frame
Script.scriptEnding.connect(scriptEnding);
Controller.keyPressEvent.connect(keyPressEvent);
