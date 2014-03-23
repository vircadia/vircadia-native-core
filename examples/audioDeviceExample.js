//
//  audioDeviceExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 3/22/14
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Menu object
//




print("Audio Input Device: " + AudioDevice.getInputDevice());
AudioDevice.setInputDevice("Shure Digital");
print("Audio Input Device: " + AudioDevice.getInputDevice());

print("Audio Output Device: " + AudioDevice.getOutputDevice());
AudioDevice.setOutputDevice("Shure Digital");
print("Audio Output Device: " + AudioDevice.getOutputDevice());

Script.stop();