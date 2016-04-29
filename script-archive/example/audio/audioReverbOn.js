//
//  audioReverbOn.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//
//
//  Gives the ability to be set various reverb settings.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

// http://wiki.audacityteam.org/wiki/GVerb#Instant_reverb_settings
var audioOptions = new AudioEffectOptions({
                                          // Square Meters
                                          maxRoomSize: 50,
                                          roomSize: 50,
                                          
                                          // Seconds
                                          reverbTime: 10,
                                          
                                          // Between 0 - 1
                                          damping: 0.50,
                                          inputBandwidth: 0.75,
                                          
                                          // dB
                                          earlyLevel: -22,
                                          tailLevel: -28,
                                          dryLevel: 0,
                                          wetLevel: 6
});

AudioDevice.setReverbOptions(audioOptions);
AudioDevice.setReverb(true);
print("Reverb is now on with the updated options.");

function scriptEnding() {
    AudioDevice.setReverb(false);
    print("Reberb is now off.");
}

Script.scriptEnding.connect(scriptEnding);