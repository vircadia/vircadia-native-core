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
var menuConnected = false;

/****************************************
    BEGIN FUNCTION DEFINITIONS
****************************************/
function setupAudioMenus() {
    if (menuConnected) {
        Menu.menuItemEvent.disconnect(menuItemEvent);
        menuConnected = false;
    }
    removeAudioMenus();

    /* Setup audio input devices */
    Menu.addSeparator("Audio", "Input Audio Device");
    var inputDevices = AudioDevice.getInputDevices();
    print("selectAudioDevice: Audio input devices: " + inputDevices);
    var selectedInputDevice = AudioDevice.getInputDevice();
    var inputDeviceSetting = Settings.getValue(INPUT_DEVICE_SETTING);
    if (inputDevices.indexOf(inputDeviceSetting) != -1 && selectedInputDevice != inputDeviceSetting) {
        print("selectAudioDevice: Input Setting & Device mismatch! Input SETTING:", inputDeviceSetting, "Input DEVICE IN USE:", selectedInputDevice);
        switchAudioDevice(true, inputDeviceSetting);
    }
    for (var i = 0; i < inputDevices.length; i++) {
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
            print("selectAudioDevice: selectedInputMenu: " + selectedInputMenu);
        }
    }

    /* Setup audio output devices */
    Menu.addSeparator("Audio", "Output Audio Device");
    var outputDevices = AudioDevice.getOutputDevices();
    print("selectAudioDevice: Audio output devices: " + outputDevices);
    var selectedOutputDevice = AudioDevice.getOutputDevice();
    var outputDeviceSetting = Settings.getValue(OUTPUT_DEVICE_SETTING);
    if (outputDevices.indexOf(outputDeviceSetting) != -1 && selectedOutputDevice != outputDeviceSetting) {
        print("selectAudioDevice: Output Setting & Device mismatch! Output SETTING:", outputDeviceSetting, "Output DEVICE IN USE:", selectedOutputDevice);
        switchAudioDevice(false, outputDeviceSetting);
    }
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
            print("selectAudioDevice: selectedOutputMenu: " + selectedOutputMenu);
        }
    }
    if (!menuConnected) {
        Menu.menuItemEvent.connect(menuItemEvent);
        menuConnected = true;
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
    print("selectAudioDevice: System audio device changed. Removing and replacing Audio Menus...");
    setupAudioMenus();
}

function menuItemEvent(menuItem) {
    if (menuConnected) {
        Menu.menuItemEvent.disconnect(menuItemEvent);
        menuConnected = false;
    }
    if (menuItem.startsWith("Use ")) {
        if (menuItem.endsWith(" for Input")) {
            var selectedDevice = menuItem.trimStartsWith("Use ").trimEndsWith(" for Input");
            print("selectAudioDevice: User selected a new Audio Input Device: " + selectedDevice);
            switchAudioDevice(true, selectedDevice);
        } else if (menuItem.endsWith(" for Output")) {
            var selectedDevice = menuItem.trimStartsWith("Use ").trimEndsWith(" for Output");
            print("selectAudioDevice: User selected a new Audio Output Device: " + selectedDevice);
            switchAudioDevice(false, selectedDevice);
        } else {
            print("selectAudioDevice: Invalid Audio menuItem! Doesn't end with 'for Input' or 'for Output'")
        }
    } else {
        print("selectAudioDevice: Invalid Audio menuItem! Doesn't start with 'Use '")
    }
    if (!menuConnected) {
        Menu.menuItemEvent.connect(menuItemEvent);
        menuConnected = true;
    }
}

function switchAudioDevice(isInput, device) {
    if (menuConnected) {
        Menu.menuItemEvent.disconnect(menuItemEvent);
        menuConnected = false;
    }
    if (isInput) {
        print("selectAudioDevice: Switching audio INPUT device to:", device);
        if (AudioDevice.setInputDevice(device)) {
            Settings.setValue(INPUT_DEVICE_SETTING, device);
        } else {
            print("selectAudioDevice: Error setting audio input device!")
        }
    } else {
        print("selectAudioDevice: Switching audio OUTPUT device to:", device);
        if (AudioDevice.setOutputDevice(device)) {
            Settings.setValue(OUTPUT_DEVICE_SETTING, device);
        } else {
            print("selectAudioDevice: Error setting audio output device!")
        }
    }
    setupAudioMenus();
    if (!menuConnected) {
        Menu.menuItemEvent.connect(menuItemEvent);
        menuConnected = true;
    }
}

function restoreAudio() {
    if (switchedAudioInputToHMD) {
        print("selectAudioDevice: Switching back from HMD preferred audio input to: " + previousSelectedInputAudioDevice);
        switchAudioDevice(true, previousSelectedInputAudioDevice);
        switchedAudioInputToHMD = false;
    }
    if (switchedAudioOutputToHMD) {
        print("selectAudioDevice: Switching back from HMD preferred audio output to: " + previousSelectedOutputAudioDevice);
        switchAudioDevice(false, previousSelectedOutputAudioDevice);
        switchedAudioOutputToHMD = false;
    }
}

function checkHMDAudio() {
    if (menuConnected) {
        Menu.menuItemEvent.disconnect(menuItemEvent);
        menuConnected = false;
    }
    // HMD Active state is changing; handle switching
    if (HMD.active != wasHmdActive) {
        print("selectAudioDevice: HMD Active state changed!");

        // We're putting the HMD on; switch to those devices
        if (HMD.active) {
            print("selectAudioDevice: HMD is now Active.");
            var hmdPreferredAudioInput = HMD.preferredAudioInput();
            var hmdPreferredAudioOutput = HMD.preferredAudioOutput();
            print("selectAudioDevice: hmdPreferredAudioInput: " + hmdPreferredAudioInput);
            print("selectAudioDevice: hmdPreferredAudioOutput: " + hmdPreferredAudioOutput);

            if (hmdPreferredAudioInput !== "") {
                print("selectAudioDevice: HMD has preferred audio input device.");
                previousSelectedInputAudioDevice = Settings.getValue(INPUT_DEVICE_SETTING);
                print("selectAudioDevice: previousSelectedInputAudioDevice: " + previousSelectedInputAudioDevice);
                if (hmdPreferredAudioInput != previousSelectedInputAudioDevice) {
                    print("selectAudioDevice: Switching Audio Input device to HMD preferred device: " + hmdPreferredAudioInput);
                    switchedAudioInputToHMD = true;
                    switchAudioDevice(true, hmdPreferredAudioInput);
                }
            }
            if (hmdPreferredAudioOutput !== "") {
                print("selectAudioDevice: HMD has preferred audio output device.");
                previousSelectedOutputAudioDevice = Settings.getValue(OUTPUT_DEVICE_SETTING);
                print("selectAudioDevice: previousSelectedOutputAudioDevice: " + previousSelectedOutputAudioDevice);
                if (hmdPreferredAudioOutput != previousSelectedOutputAudioDevice) {
                    print("selectAudioDevice: Switching Audio Output device to HMD preferred device: " + hmdPreferredAudioOutput);
                    switchedAudioOutputToHMD = true;
                    switchAudioDevice(false, hmdPreferredAudioOutput);
                }
            }
        } else {
            print("selectAudioDevice: HMD no longer active. Restoring audio I/O devices...");
            restoreAudio();
        }
    }
    wasHmdActive = HMD.active;
    if (!menuConnected) {
        Menu.menuItemEvent.connect(menuItemEvent);
        menuConnected = true;
    }
}
/****************************************
    END FUNCTION DEFINITIONS
****************************************/

/****************************************
    BEGIN SCRIPT BODY
****************************************/
// Have a small delay before the menus get setup so the audio devices can switch to the last selected ones
Script.setTimeout(function () {
    print("selectAudioDevice: Connecting deviceChanged() and displayModeChanged()");
    AudioDevice.deviceChanged.connect(onDevicechanged);
    HMD.displayModeChanged.connect(checkHMDAudio);
    print ("selectAudioDevice: Checking HMD audio status...")
    checkHMDAudio();
    print("selectAudioDevice: Setting up Audio > Devices menu for the first time");
    setupAudioMenus();
}, 3000);

print("selectAudioDevice: Connecting menuItemEvent() and scriptEnding()");
Menu.menuItemEvent.connect(menuItemEvent);
menuConnected = true;
Script.scriptEnding.connect(function () {
    restoreAudio();
    removeAudioMenus();
    Menu.menuItemEvent.disconnect(menuItemEvent);
    HMD.displayModeChanged.disconnect(checkHMDAudio);
});

}()); // END LOCAL_SCOPE
