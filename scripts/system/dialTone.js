"use strict";

//
//  dialTone.js
//  examples
//
//  Created by Stephen Birarda on 06/08/15.
//  Added disconnect HRS 6/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE

// setup the local sound we're going to use
var connectSound = SoundCache.getSound(Script.resolvePath("assets/sounds/hello.wav"));
var disconnectSound = SoundCache.getSound(Script.resolvePath("assets/sounds/goodbye.wav"));
var micMutedSound = SoundCache.getSound(Script.resolvePath("assets/sounds/goodbye.wav"));

// setup the options needed for that sound
var soundOptions = {
    localOnly: true
};

// play the sound locally once we get the first audio packet from a mixer
Audio.receivedFirstPacket.connect(function(){
    Audio.playSound(connectSound, soundOptions);
});

Audio.disconnected.connect(function(){
    Audio.playSound(disconnectSound, soundOptions);
});

Audio.mutedChanged.connect(function () {
    if (Audio.muted) {
        Audio.playSound(micMutedSound, soundOptions);
    }
});

}()); // END LOCAL_SCOPE
