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

var soundClip = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Voxels/voxel create 3.raw");

var currentTime = 1.570079; // pi/2
var deltaTime = 0.05;
var distance = 1;
var debug = 0;

function playSound() {
    var options = new AudioInjectionOptions();
    currentTime += deltaTime;

	var s = distance * Math.sin(currentTime);
	var c = distance * Math.cos(currentTime);

    var soundOffset = { x:s, y:0, z:c };

    if (debug) {
    	print("t=" + currentTime + "offset=" + soundOffset.x + "," + soundOffset.y + "," + soundOffset.z);
    }

    var avatarPosition = MyAvatar.position;
    var soundPosition = Vec3.sum(avatarPosition,soundOffset);

    options.position = soundPosition
    options.volume = 1.0;
    Audio.playSound(soundClip, options);
}

Script.setInterval(playSound, 250);


