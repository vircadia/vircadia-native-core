//
//  playSoundPath.js
//  examples
//
//  Created by Craig Hansen-Sturm on 05/27/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("libraries/globals.js");

var soundClip = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Voxels/voxel create 3.raw");

var currentTime = 1.570079; // pi/2
var deltaTime = 0.05;
var distance = 1;
var debug = 0;

function playSound() {
  currentTime += deltaTime;

	var s = distance * Math.sin(currentTime);
	var c = distance * Math.cos(currentTime);

  var soundOffset = { x:s, y:0, z:c };

  if (debug) {
  	print("t=" + currentTime + "offset=" + soundOffset.x + "," + soundOffset.y + "," + soundOffset.z);
  }

  var avatarPosition = MyAvatar.position;
  var soundPosition = Vec3.sum(avatarPosition,soundOffset);

  Audio.playSound(soundClip, {
    position: soundPosition
  });
}

Script.setInterval(playSound, 250);


