//
//  ambiance.js
//  examples
//
//  Created by Clément Brisset on 11/18/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var soundURL = "http://hifi-public.s3.amazonaws.com/ryan/Cat_Purring_Deep_Low_Snor.wav";
var position = { x: 551.48, y: 495.60, z: 502.08};
var audioOptions = {
    position: position,
    volume: .03,
    loop: true
};

var sound = SoundCache.getSound(soundURL);
var injector = null;
var count = 300;

Script.update.connect(function() {
  if (count > 0) {
    count--;
    return;
  }
  
  if (sound.downloaded && injector === null) {
    print("Sound downloaded.");
    injector = Audio.playSound(sound, audioOptions);
    print("Playing: " + injector);
  }
});

Script.scriptEnding.connect(function() {
    if (injector !== null) {
        injector.stop();
    }
});