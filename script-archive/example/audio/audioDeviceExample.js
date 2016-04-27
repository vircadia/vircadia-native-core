//
//  audioDeviceExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 3/22/14
//  Copyright 2013 High Fidelity, Inc.
//
//  This is an example script that demonstrates use of the Menu object
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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