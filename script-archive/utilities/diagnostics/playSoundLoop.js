//
//  playSoundLoop.js
//  examples
//
//  Created by David Rowe on 5/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This example script plays a sound in a continuous loop, and creates a red sphere in front of you at the origin of the sound.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


//  A few sample files you may want to try: 

var sound = SoundCache.getSound(Script.getExternalPath(Script.ExternalPaths.Assets, "sounds/Guitars/Guitar+-+Nylon+A.raw"));
//var sound = SoundCache.getSound(Script.getExternalPath(Script.ExternalPaths.Assets, "sounds/220Sine.wav"));
//var sound = SoundCache.getSound(Script.getExternalPath(Script.ExternalPaths.Assets, "sounds/Cocktail+Party+Snippets/Bandcamp.wav"));

var soundPlaying = false;
var options = {
  position: Vec3.sum(Camera.getPosition(), Quat.getFront(MyAvatar.orientation)),
  volume: 0.5,
  loop: true
}

var playing = false;
var ball = false;

function maybePlaySound(deltaTime) {
    if (sound.downloaded && !soundPlaying) {
        var properties = {
                type: "Sphere",
                position: options.position, 
                 dimensions: { x: 0.2, y: 0.2, z: 0.2 },
                color: { red: 200, green: 0, blue: 0 }    
            };
        ball = Entities.addEntity(properties);
        soundPlaying = Audio.playSound(sound, options);
        print("Started sound looping.");
        Script.update.disconnect(maybePlaySound);
    }
}
        
function scriptEnding() {
  if (ball) {
    Entities.deleteEntity(ball);
  }
}

// Connect a call back that happens every frame
Script.scriptEnding.connect(scriptEnding);
Script.update.connect(maybePlaySound);

