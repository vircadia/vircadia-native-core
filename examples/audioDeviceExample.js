//
//  audioDeviceExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 3/22/14
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Menu object
//



var inputDevices = AudioDevice.getInputDevices();
var defaultInputDevice = AudioDevice.getDefaultInputDevice();
print("Input Devices:");
for(var i = 0; i < inputDevices.length; i++) {
    if (inputDevices[i] == defaultInputDevice) {
        print("    " + inputDevices[i] + " << default");
    } else {
        print("    " + inputDevices[i]);
    }
}

var outputDevices = AudioDevice.getInputDevices();
var defaultOutputDevice = AudioDevice.getDefaultInputDevice();
print("Output Devices:");
for(var i = 0; i < outputDevices.length; i++) {
    if (outputDevices[i] == defaultOutputDevice) {
        print("    " + outputDevices[i] + " << default");
    } else {
        print("    " + outputDevices[i]);
    }
}

print("Audio Input Device: " + AudioDevice.getInputDevice());
AudioDevice.setInputDevice("Shure Digital");
print("Audio Input Device: " + AudioDevice.getInputDevice());

print("Audio Output Device: " + AudioDevice.getOutputDevice());
AudioDevice.setOutputDevice("Shure Digital");
print("Audio Output Device: " + AudioDevice.getOutputDevice());

Script.stop();