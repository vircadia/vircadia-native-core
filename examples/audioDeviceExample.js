//
//  audioDeviceExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 3/22/14
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Menu object
//


var outputDevices = AudioDevice.getOutputDevices();
var defaultOutputDevice = AudioDevice.getDefaultOutputDevice();
var selectOutputDevice = outputDevices[0];
print("Output Devices:");
for(var i = 0; i < outputDevices.length; i++) {
    if (outputDevices[i] == defaultOutputDevice) {
        print("    " + outputDevices[i] + " << default");
    } else {
        print("    " + outputDevices[i]);
    }
}

print("Default Output Device:" + defaultOutputDevice);
print("Selected Output Device:" + selectOutputDevice);
print("Current Audio Output Device: " + AudioDevice.getOutputDevice());
AudioDevice.setOutputDevice(selectOutputDevice);
print("Audio Output Device: " + AudioDevice.getOutputDevice());

var inputDevices = AudioDevice.getInputDevices();
var selectInputDevice = inputDevices[0];
var defaultInputDevice = AudioDevice.getDefaultInputDevice();
print("Input Devices:");
for(var i = 0; i < inputDevices.length; i++) {
    if (inputDevices[i] == defaultInputDevice) {
        print("    " + inputDevices[i] + " << default");
    } else {
        print("    " + inputDevices[i]);
    }
}

print("Default Input Device:" + defaultInputDevice);
print("Selected Input Device:" + selectInputDevice);
print("Current Audio Input Device: " + AudioDevice.getInputDevice());
AudioDevice.setInputDevice(selectInputDevice);
print("Audio Input Device: " + AudioDevice.getInputDevice());

print("Audio Input Device Level: " + AudioDevice.getInputVolume());
AudioDevice.setInputVolume(AudioDevice.getInputVolume() * 2); // twice as loud!
print("Audio Input Device Level: " + AudioDevice.getInputVolume());

Script.stop();