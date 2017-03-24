"use strict";

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

(function() { // BEGIN LOCAL_SCOPE

if (typeof String.prototype.startsWith != 'function') {
    String.prototype.startsWith = function (str){
        return this.slice(0, str.length) == str;
    };
}

if (typeof String.prototype.endsWith != 'function') {
    String.prototype.endsWith = function (str){
        return this.slice(-str.length) == str;
    };
}

if (typeof String.prototype.trimStartsWith != 'function') {
    String.prototype.trimStartsWith = function (str){
        if (this.startsWith(str)) {
            return this.substr(str.length);
        }
        return this;
    };
}

if (typeof String.prototype.trimEndsWith != 'function') {
    String.prototype.trimEndsWith = function (str){
        if (this.endsWith(str)) {
            return this.substr(0,this.length - str.length);
        }
        return this;
    };
}

/****************************************
    VAR DEFINITIONS
****************************************/
const INPUT_DEVICE_SETTING = "audio_input_device";
const OUTPUT_DEVICE_SETTING = "audio_output_device";
var selectedInputMenu = "";
var selectedOutputMenu = "";
var audioDevicesList = [];
// Some HMDs (like Oculus CV1) have a built in audio device. If they
// do, then this function will handle switching to that device automatically
// when you goActive with the HMD active.
var wasHmdActive = false; // assume it's not active to start
var switchedAudioInputToHMD = false;
var switchedAudioOutputToHMD = false;
var previousSelectedInputAudioDevice = "";
var previousSelectedOutputAudioDevice = "";

/****************************************
    BEGIN FUNCTION DEFINITIONS
****************************************/
function setupAudioMenus() {
    removeAudioMenus();

    /* Setup audio input devices */
    Menu.addSeparator("Audio", "Input Audio Device");
    var inputDeviceSetting = Settings.getValue(INPUT_DEVICE_SETTING);
    var inputDevices = AudioDevice.getInputDevices();
    var selectedInputDevice = AudioDevice.getInputDevice();
    if (inputDevices.indexOf(inputDeviceSetting) != -1 && selectedInputDevice != inputDeviceSetting) {
        print ("Audio input device SETTING does not match Input AudioDevice. Attempting to change Input AudioDevice...")
        if (AudioDevice.setInputDevice(inputDeviceSetting)) {
            selectedInputDevice = inputDeviceSetting;
        } else {
            print("Error setting audio input device!")
        }
    }
    print("Audio input devices: " + inputDevices);
    for(var i = 0; i < inputDevices.length; i++) {
        var thisDeviceSelected = (inputDevices[i] == selectedInputDevice);
        var menuItem = "Use " + inputDevices[i] + " for Input";
        Menu.addMenuItem({
            menuName: "Audio",
            menuItemName: menuItem,
            isCheckable: true,
            isChecked: thisDeviceSelected
        });
        audioDevicesList.push(menuItem);
        if (thisDeviceSelected) {
            selectedInputMenu = menuItem;
        }
    }

    /* Setup audio output devices */
    Menu.addSeparator("Audio", "Output Audio Device");
    var outputDeviceSetting = Settings.getValue(OUTPUT_DEVICE_SETTING);
    var outputDevices = AudioDevice.getOutputDevices();
    var selectedOutputDevice = AudioDevice.getOutputDevice();
    if (outputDevices.indexOf(outputDeviceSetting) != -1 && selectedOutputDevice != outputDeviceSetting) {
        print("Audio output device SETTING does not match Output AudioDevice. Attempting to change Output AudioDevice...")
        if (AudioDevice.setOutputDevice(outputDeviceSetting)) {
            selectedOutputDevice = outputDeviceSetting;
        } else {
            print("Error setting audio output device!")
        }
    }
    print("Audio output devices: " + outputDevices);
    for (var i = 0; i < outputDevices.length; i++) {
        var thisDeviceSelected = (outputDevices[i] == selectedOutputDevice);
        var menuItem = "Use " + outputDevices[i] + " for Output";
        Menu.addMenuItem({
            menuName: "Audio",
            menuItemName: menuItem,
            isCheckable: true,
            isChecked: thisDeviceSelected
        });
        audioDevicesList.push(menuItem);
        if (thisDeviceSelected) {
            selectedOutputMenu = menuItem;
        }
    }
}

function removeAudioMenus() {
    Menu.removeSeparator("Audio", "Input Audio Device");
    Menu.removeSeparator("Audio", "Output Audio Device");

    for (var index = 0; index < audioDevicesList.length; index++) {
        Menu.removeMenuItem("Audio", audioDevicesList[index]);
    }

    Menu.removeMenu("Audio > Devices");

    audioDevicesList = [];
}

function onDevicechanged() {
    print("System audio device changed. Removing and replacing Audio Menus...");
    setupAudioMenus();
}

function menuItemEvent(menuItem) {
    if (menuItem.startsWith("Use ")) {
        if (menuItem.endsWith(" for Input")) {
            var selectedDevice = menuItem.trimStartsWith("Use ").trimEndsWith(" for Input");
            print("User selected a new Audio Input Device: " + selectedDevice);
            Menu.menuItemEvent.disconnect(menuItemEvent);
            Menu.setIsOptionChecked(selectedInputMenu, false);
            selectedInputMenu = menuItem;
            Menu.setIsOptionChecked(selectedInputMenu, true);
            if (AudioDevice.setInputDevice(selectedDevice)) {
                Settings.setValue(INPUT_DEVICE_SETTING, selectedDevice);
            } else {
                print("Error setting audio input device!")
            }
            Menu.menuItemEvent.connect(menuItemEvent);
        } else if (menuItem.endsWith(" for Output")) {
            var selectedDevice = menuItem.trimStartsWith("Use ").trimEndsWith(" for Output");
            print("User selected a new Audio Output Device: " + selectedDevice);
            Menu.menuItemEvent.disconnect(menuItemEvent);
            Menu.setIsOptionChecked(selectedOutputMenu, false);
            selectedOutputMenu = menuItem;
            Menu.setIsOptionChecked(selectedOutputMenu, true);
            if (AudioDevice.setOutputDevice(selectedDevice)) {
                Settings.setValue(OUTPUT_DEVICE_SETTING, selectedDevice);
            } else {
                print("Error setting audio output device!")
            }
            Menu.menuItemEvent.connect(menuItemEvent);
        }
    }
}

function restoreAudio() {
    if (switchedAudioInputToHMD) {
        print("Switching back from HMD preferred audio input to: " + previousSelectedInputAudioDevice);
        menuItemEvent("Use " + previousSelectedInputAudioDevice + " for Input");
        switchedAudioInputToHMD = false;
    }
    if (switchedAudioOutputToHMD) {
        print("Switching back from HMD preferred audio output to: " + previousSelectedOutputAudioDevice);
        menuItemEvent("Use " + previousSelectedOutputAudioDevice + " for Output");
        switchedAudioOutputToHMD = false;
    }
}

function checkHMDAudio() {
    // HMD Active state is changing; handle switching
    if (HMD.active != wasHmdActive) {
        print("HMD Active state changed!");

        // We're putting the HMD on; switch to those devices
        if (HMD.active) {
            print("HMD is now Active.");
            var hmdPreferredAudioInput = HMD.preferredAudioInput();
            var hmdPreferredAudioOutput = HMD.preferredAudioOutput();
            print("hmdPreferredAudioInput: " + hmdPreferredAudioInput);
            print("hmdPreferredAudioOutput: " + hmdPreferredAudioOutput);


            if (hmdPreferredAudioInput !== "") {
                print("HMD has preferred audio input device.");
                previousSelectedInputAudioDevice = Settings.getValue(INPUT_DEVICE_SETTING);
                print("previousSelectedInputAudioDevice: " + previousSelectedInputAudioDevice);
                if (hmdPreferredAudioInput != previousSelectedInputAudioDevice) {
                    print("Switching Audio Input device to HMD preferred device: " + hmdPreferredAudioInput);
                    switchedAudioInputToHMD = true;
                    menuItemEvent("Use " + hmdPreferredAudioInput + " for Input");
                }
            }
            if (hmdPreferredAudioOutput !== "") {
                print("HMD has preferred audio output device.");
                previousSelectedOutputAudioDevice = Settings.getValue(OUTPUT_DEVICE_SETTING);
                print("previousSelectedOutputAudioDevice: " + previousSelectedOutputAudioDevice);
                if (hmdPreferredAudioOutput != previousSelectedOutputAudioDevice) {
                    print("Switching Audio Output device to HMD preferred device: " + hmdPreferredAudioOutput);
                    switchedAudioOutputToHMD = true;
                    menuItemEvent("Use " + hmdPreferredAudioOutput + " for Output");
                }
            }
        } else {
            print("HMD no longer active. Restoring audio I/O devices...");
            restoreAudio();
        }
    }
    wasHmdActive = HMD.active;
}
/****************************************
    END FUNCTION DEFINITIONS
****************************************/

/****************************************
    BEGIN SCRIPT BODY
****************************************/
// Have a small delay before the menus get setup so the audio devices can switch to the last selected ones
Script.setTimeout(function () {
    print("Connecting deviceChanged() and displayModeChanged()");
    AudioDevice.deviceChanged.connect(onDevicechanged);
    HMD.displayModeChanged.connect(checkHMDAudio);
    print("Setting up Audio > Devices menu for the first time");
    setupAudioMenus();
}, 2000);

print("Connecting menuItemEvent() and scriptEnding()");
Menu.menuItemEvent.connect(menuItemEvent);
Script.scriptEnding.connect(function () {
    restoreAudio();
    removeAudioMenus();
    Menu.menuItemEvent.disconnect(menuItemEvent);
    HMD.displayModeChanged.disconnect(checkHMDAudio);
});

}()); // END LOCAL_SCOPE
