//
//  dialTone.js
//  examples
//
//  Created by Stephen Birarda on 06/08/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// setup the local sound we're going to use
var connectSound = SoundCache.getSound("file://" + Paths.resources + "sounds/short1.wav");

// setup the options needed for that sound
var connectSoundOptions = {
    localOnly: true
}

// play the sound locally once we get the first audio packet from a mixer
Audio.receivedFirstPacket.connect(function(){
    Audio.playSound(connectSound, connectSoundOptions);
});
