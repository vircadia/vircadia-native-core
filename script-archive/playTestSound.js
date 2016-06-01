//
//  playTestSound.js
//  examples
//
//  Created by Philip Rosedale
//  Copyright 2014 High Fidelity, Inc.
//
//  Creates an object in front of you that changes color and plays a light
//  at the start of a drum clip that loops.   As you move away it will tell you in the
//  log how many meters you are from the source.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var sound = SoundCache.getSound("https://s3.amazonaws.com/hifi-public/sounds/Drums/deepdrum1.wav");

var position = Vec3.sum(Vec3.sum(MyAvatar.position, { x: 0, y: 0.5, z: 0 }), Quat.getFront(MyAvatar.orientation));

var time;
var soundPlaying = null;

var baseColor = { red: 100, green: 100, blue: 100 };
var litColor =  { red: 255, green: 100, blue: 0 };

var lightTime = 250;

var distance = 0.0;

//  Create object for visual reference
var box = Entities.addEntity({
  type: "Box",
  dimensions: { x: 0.25, y: 0.5, z: 0.25 },
  color: baseColor,
  position:  position
  });


function checkSound(deltaTime) {
  var started = false;
    if (!sound.downloaded) {
      return;
    }
    if (soundPlaying == null) {
        soundPlaying = Audio.playSound(sound, {
                                        position: position,
                                        volume: 1.0,
                                        loop: false } );
        started = true;
    } else if (!soundPlaying.playing) {
        soundPlaying.restart();
        started = true;
    }
    if (started) {
      Entities.editEntity(box, { color: litColor });
      Entities.addEntity({
                    type: "Light",
                    intensity: 5.0,
                    falloffRadius: 10.0,
                    dimensions: {
                        x: 40,
                        y: 40,
                        z: 40
                    },
                    position: Vec3.sum(position, { x: 0, y: 1, z: 0 }),
                    color: litColor,
                    lifetime:  lightTime / 1000
                });
      Script.setTimeout(resetColor, lightTime);
    }
    var currentDistance = Vec3.distance(MyAvatar.position, position);
    if (Math.abs(currentDistance - distance) > 1.0) {
      print("Distance from source: " + currentDistance);
      distance = currentDistance;
    }
}

function resetColor() {
   Entities.editEntity(box, { color: baseColor });
}


function scriptEnding() {
  Entities.deleteEntity(box);
  if (soundPlaying) {
    print("stop injector");
    soundPlaying.stop();
  }
}



// Connect a call back that happens every frame
Script.scriptEnding.connect(scriptEnding);
Script.update.connect(checkSound);
