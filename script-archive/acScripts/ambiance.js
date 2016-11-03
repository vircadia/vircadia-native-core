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

var soundURL = "https://s3.amazonaws.com/hifi-public/sounds/08_Funny_Bone.wav";
var position = { x: 700, y: 25, z: 725 };
var audioOptions = {
    position: position,
    volume: 0.4,
    loop: true
};

var sound = SoundCache.getSound(soundURL);
var injector = null;
var count = 100;

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