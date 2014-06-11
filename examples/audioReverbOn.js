//
//  audioReverbOn.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

// http://wiki.audacityteam.org/wiki/GVerb#Instant_reverb_settings
var audioOptions = new AudioEffectOptions();

// Square Meters
audioOptions.maxRoomSize = 50;
audioOptions.roomSize = 50;

// Seconds
audioOptions.reverbTime = 4;

// Between 0 - 1
audioOptions.damping = 0.50;
audioOptions.inputBandwidth = 0.75;

// dB
audioOptions.earlyLevel = -22;
audioOptions.tailLevel = -28;
audioOptions.dryLevel = 0;
audioOptions.wetLevel = 6;

AudioDevice.setReverbOptions(audioOptions);
AudioDevice.setReverb(true);
print("Reverb is now on with the updated options.");
