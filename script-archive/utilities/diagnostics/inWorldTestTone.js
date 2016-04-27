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

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var sound = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/220Sine.wav");

var soundPlaying = false;

var offset = Vec3.normalize(Quat.getFront(MyAvatar.orientation));
var position = Vec3.sum(MyAvatar.position, offset);

function update(deltaTime) {
  if (sound.downloaded && !soundPlaying) {
    print("Started sound loop");
    soundPlaying = Audio.playSound(sound, {
      position: position,
      loop: true
    });
  }
}

Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);

