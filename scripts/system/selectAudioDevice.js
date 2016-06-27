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

const INPUT_DEVICE_SETTING = "audio_input_device";
const OUTPUT_DEVICE_SETTING = "audio_output_device";

var selectedInputMenu = "";
var selectedOutputMenu = "";

function setupAudioMenus() {
    Menu.addMenu("Audio > Devices");
    Menu.addSeparator("Audio > Devices","Output Audio Device");

    var outputDeviceSetting = Settings.getValue(OUTPUT_DEVICE_SETTING);
    var outputDevices = AudioDevice.getOutputDevices();
    var selectedOutputDevice = AudioDevice.getOutputDevice();
    if (outputDevices.indexOf(outputDeviceSetting) != -1 && selectedOutputDevice != outputDeviceSetting) {
        if (AudioDevice.setOutputDevice(outputDeviceSetting)) {
            selectedOutputDevice = outputDeviceSetting;
        }
    }
    for(var i = 0; i < outputDevices.length; i++) {
        var thisDeviceSelected = (outputDevices[i] == selectedOutputDevice);
        var menuItem = "Use " + outputDevices[i] + " for Output";
        Menu.addMenuItem({ 
                            menuName: "Audio > Devices", 
                            menuItemName: menuItem, 
                            isCheckable: true, 
                            isChecked: thisDeviceSelected 
                        });
        if (thisDeviceSelected) {
            selectedOutputMenu = menuItem;
        }
    }

    Menu.addSeparator("Audio > Devices","Input Audio Device");

    var inputDeviceSetting = Settings.getValue(INPUT_DEVICE_SETTING);
    var inputDevices = AudioDevice.getInputDevices();
    var selectedInputDevice = AudioDevice.getInputDevice();
    if (inputDevices.indexOf(inputDeviceSetting) != -1 && selectedInputDevice != inputDeviceSetting) {
        if (AudioDevice.setInputDevice(inputDeviceSetting)) {
            selectedInputDevice = inputDeviceSetting;
        }
    }
    for(var i = 0; i < inputDevices.length; i++) {
        var thisDeviceSelected = (inputDevices[i] == selectedInputDevice);
        var menuItem = "Use " + inputDevices[i] + " for Input";
        Menu.addMenuItem({ 
                            menuName: "Audio > Devices", 
                            menuItemName: menuItem, 
                            isCheckable: true, 
                            isChecked: thisDeviceSelected 
                        });
        if (thisDeviceSelected) {
            selectedInputMenu = menuItem;
        }
    }
}

function onDevicechanged() {
    Menu.removeMenu("Audio > Devices");
    setupAudioMenus();
}

// Have a small delay before the menu's get setup and the audio devices can switch to the last selected ones
Script.setTimeout(function () {
    AudioDevice.deviceChanged.connect(onDevicechanged);
    setupAudioMenus();
}, 5000);

function scriptEnding() {
    Menu.removeMenu("Audio > Devices");
}
Script.scriptEnding.connect(scriptEnding);


function menuItemEvent(menuItem) {
    if (menuItem.startsWith("Use ")) {
        if (menuItem.endsWith(" for Output")) {
            var selectedDevice = menuItem.trimStartsWith("Use ").trimEndsWith(" for Output");
            print("output audio selection..." + selectedDevice);
            Menu.menuItemEvent.disconnect(menuItemEvent);
            Menu.setIsOptionChecked(selectedOutputMenu, false);
            selectedOutputMenu = menuItem;
            Menu.setIsOptionChecked(selectedOutputMenu, true);
            if (AudioDevice.setOutputDevice(selectedDevice)) {
                Settings.setValue(OUTPUT_DEVICE_SETTING, selectedDevice);
            }
            Menu.menuItemEvent.connect(menuItemEvent);
        } else if (menuItem.endsWith(" for Input")) {
            var selectedDevice = menuItem.trimStartsWith("Use ").trimEndsWith(" for Input");
            print("input audio selection..." + selectedDevice);
            Menu.menuItemEvent.disconnect(menuItemEvent);
            Menu.setIsOptionChecked(selectedInputMenu, false);
            selectedInputMenu = menuItem;
            Menu.setIsOptionChecked(selectedInputMenu, true);
            if (AudioDevice.setInputDevice(selectedDevice)) {
                Settings.setValue(INPUT_DEVICE_SETTING, selectedDevice);
            }
            Menu.menuItemEvent.connect(menuItemEvent);
        }
    }
}

Menu.menuItemEvent.connect(menuItemEvent);

// Some HMDs (like Oculus CV1) have a built in audio device. If they
// do, then this function will handle switching to that device automatically
// when you goActive with the HMD active.
var wasHmdMounted = false; // assume it's un-mounted to start
var switchedAudioInputToHMD = false;
var switchedAudioOutputToHMD = false;
var previousSelectedInputAudioDevice = "";
var previousSelectedOutputAudioDevice = "";

function restoreAudio() {
    if (switchedAudioInputToHMD) {
        print("switching back from HMD preferred audio input to:" + previousSelectedInputAudioDevice);
        menuItemEvent("Use " + previousSelectedInputAudioDevice + " for Input");
    }
    if (switchedAudioOutputToHMD) {
        print("switching back from HMD preferred audio output to:" + previousSelectedOutputAudioDevice);
        menuItemEvent("Use " + previousSelectedOutputAudioDevice + " for Output");
    }
}

function checkHMDAudio() {
    // Mounted state is changing... handle switching
    if (HMD.mounted != wasHmdMounted) {
        print("HMD mounted changed...");

        // We're putting the HMD on... switch to those devices
        if (HMD.mounted) {
            print("NOW mounted...");
            var hmdPreferredAudioInput = HMD.preferredAudioInput();
            var hmdPreferredAudioOutput = HMD.preferredAudioOutput();
            print("hmdPreferredAudioInput:" + hmdPreferredAudioInput);
            print("hmdPreferredAudioOutput:" + hmdPreferredAudioOutput);


            var hmdHasPreferredAudio = (hmdPreferredAudioInput !== "") || (hmdPreferredAudioOutput !== "");
            if (hmdHasPreferredAudio) {
                print("HMD has preferred audio!");
                previousSelectedInputAudioDevice = Settings.getValue(INPUT_DEVICE_SETTING);
                previousSelectedOutputAudioDevice = Settings.getValue(OUTPUT_DEVICE_SETTING);
                print("previousSelectedInputAudioDevice:" + previousSelectedInputAudioDevice);
                print("previousSelectedOutputAudioDevice:" + previousSelectedOutputAudioDevice);
                if (hmdPreferredAudioInput != previousSelectedInputAudioDevice && hmdPreferredAudioInput !== "") {
                    print("switching to HMD preferred audio input to:" + hmdPreferredAudioInput);
                    switchedAudioInputToHMD = true;
                    menuItemEvent("Use " + hmdPreferredAudioInput + " for Input");
                }
                if (hmdPreferredAudioOutput != previousSelectedOutputAudioDevice && hmdPreferredAudioOutput !== "") {
                    print("switching to HMD preferred audio output to:" + hmdPreferredAudioOutput);
                    switchedAudioOutputToHMD = true;
                    menuItemEvent("Use " + hmdPreferredAudioOutput + " for Output");
                }
            }
        } else {
            print("HMD NOW un-mounted...");
            restoreAudio();
        }
    }
    wasHmdMounted = HMD.mounted;
}

Script.update.connect(checkHMDAudio);

Script.scriptEnding.connect(function () {
    restoreAudio();
    Menu.menuItemEvent.disconnect(menuItemEvent);
    Script.update.disconnect(checkHMDAudio);
});
