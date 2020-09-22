//
//  injectorTests.js
//  examples
//
//  Created by Stephen Birarda on 11/16/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var soundURL = Script.getExternalPath(Script.ExternalPaths.HF_Public, "/birarda/medium-crowd.wav");
var audioOptions = {
    position: { x: 0.0, y: 0.0, z: 0.0 },
    volume: 0.5
};

var sound = SoundCache.getSound(soundURL);
var injector = null;
var restarting = false;

Script.update.connect(function(){
  if (sound.downloaded) {
    if (!injector) {
        injector = Audio.playSound(sound, audioOptions);
    } else if (!injector.isPlaying && !restarting) {
        restarting = true;

        Script.setTimeout(function(){
            print("Calling restart for a stopped injector from script.");
            injector.restart();
        }, 1000);
    } else if (injector.isPlaying) {
        restarting = false;

        if (Math.random() < 0.0001) {
            print("Calling restart for a running injector from script.");
            injector.restart();
        }
    }
  }
})
