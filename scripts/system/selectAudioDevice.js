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

const INPUT = "Input";
const OUTPUT = "Output";
const SELECT_AUDIO_SCRIPT_STARTUP_TIMEOUT = 300;
//
// VAR DEFINITIONS
//
var debugPrintStatements = true;
const INPUT_DEVICE_SETTING = "audio_input_device";
const OUTPUT_DEVICE_SETTING = "audio_output_device";
var audioDevicesList = []; // placeholder for menu items
var wasHmdActive = false; // assume it's not active to start
var switchedAudioInputToHMD = false;
var switchedAudioOutputToHMD = false;
var previousSelectedInputAudioDevice = "";
var previousSelectedOutputAudioDevice = "";

var interfaceInputDevice = "";
var interfaceOutputDevice = "";

//
// BEGIN FUNCTION DEFINITIONS
//
function debug() {
    if (debugPrintStatements) {
        print.apply(null, [].concat.apply(["selectAudioDevice.js:"], [].map.call(arguments, JSON.stringify)));
    }
}


function setupAudioMenus() {
    // menu events can be triggered asynchronously; skip them for 200ms to avoid recursion and false switches
    removeAudioMenus();

    // Setup audio input devices
    Menu.addSeparator("Audio", "Input Audio Device");
    var currentInputDevice = AudioDevice.getInputDevice()
    for (var i = 0; i < AudioDevice.inputAudioDevices.length; i++) {
        var audioDeviceMenuString = "Use " + AudioDevice.inputAudioDevices[i] + " for Input";
        Menu.addMenuItem({
            menuName: "Audio",
            menuItemName: audioDeviceMenuString,
            isCheckable: true,
            isChecked: AudioDevice.inputAudioDevices[i] == currentInputDevice
        });
        audioDevicesList.push(audioDeviceMenuString);
    }

    // Setup audio output devices
    Menu.addSeparator("Audio", "Output Audio Device");
    var currentOutputDevice = AudioDevice.getOutputDevice()
    for (var i = 0; i < AudioDevice.outputAudioDevices.length; i++) {
        var audioDeviceMenuString = "Use " + AudioDevice.outputAudioDevices[i] + " for Output";
        Menu.addMenuItem({
            menuName: "Audio",
            menuItemName: audioDeviceMenuString,
            isCheckable: true,
            isChecked: AudioDevice.outputAudioDevices[i] == currentOutputDevice
        });
        audioDevicesList.push(audioDeviceMenuString);
    }
}

function removeAudioMenus() {
    Menu.removeSeparator("Audio", "Input Audio Device");
    Menu.removeSeparator("Audio", "Output Audio Device");

    for (var index = 0; index < audioDevicesList.length; index++) {
        if (Menu.menuItemExists("Audio", audioDevicesList[index])) {
            Menu.removeMenuItem("Audio", audioDevicesList[index]);
        }
    }

    Menu.removeMenu("Audio > Devices");

    audioDevicesList = [];
}

function onDevicechanged() {
    debug("System audio devices changed. Removing and replacing Audio Menus...");
    setupAudioMenus();
}

function onMenuEvent(audioDeviceMenuString) {
    if (Menu.isOptionChecked(audioDeviceMenuString) &&
            (audioDeviceMenuString !== interfaceInputDevice &&
             audioDeviceMenuString !== interfaceOutputDevice)) {
        AudioDevice.setDeviceFromMenu(audioDeviceMenuString)
    }
}

function onCurrentDeviceChanged() {
    debug("System audio device switched. ");
    interfaceInputDevice = "Use " + AudioDevice.getInputDevice() + " for Input";
    interfaceOutputDevice = "Use " + AudioDevice.getOutputDevice() + " for Output";
    for (var index = 0; index < audioDevicesList.length; index++) {
        if (audioDevicesList[index] === interfaceInputDevice ||
                audioDevicesList[index] === interfaceOutputDevice) {
            if (Menu.isOptionChecked(audioDevicesList[index]) === false)
                Menu.setIsOptionChecked(audioDevicesList[index], true);
        } else {
            if (Menu.isOptionChecked(audioDevicesList[index]) === true)
                Menu.setIsOptionChecked(audioDevicesList[index], false);
        }
    }
}

function restoreAudio() {
    if (switchedAudioInputToHMD) {
        debug("Switching back from HMD preferred audio input to: " + previousSelectedInputAudioDevice);
        AudioDevice.setInputDeviceAsync(previousSelectedInputAudioDevice)
        switchedAudioInputToHMD = false;
    }
    if (switchedAudioOutputToHMD) {
        debug("Switching back from HMD preferred audio output to: " + previousSelectedOutputAudioDevice);
        AudioDevice.setOutputDeviceAsync(previousSelectedOutputAudioDevice)
        switchedAudioOutputToHMD = false;
    }
}

// Some HMDs (like Oculus CV1) have a built in audio device. If they
// do, then this function will handle switching to that device automatically
// when you goActive with the HMD active.
function checkHMDAudio() {
    // HMD Active state is changing; handle switching
    if (HMD.active != wasHmdActive) {
        debug("HMD Active state changed!");

        // We're putting the HMD on; switch to those devices
        if (HMD.active) {
            debug("HMD is now Active.");
            var hmdPreferredAudioInput = HMD.preferredAudioInput();
            var hmdPreferredAudioOutput = HMD.preferredAudioOutput();
            debug("hmdPreferredAudioInput: " + hmdPreferredAudioInput);
            debug("hmdPreferredAudioOutput: " + hmdPreferredAudioOutput);

            if (hmdPreferredAudioInput !== "") {
                debug("HMD has preferred audio input device.");
                previousSelectedInputAudioDevice = Settings.getValue(INPUT_DEVICE_SETTING);
                debug("previousSelectedInputAudioDevice: " + previousSelectedInputAudioDevice);
                if (hmdPreferredAudioInput != previousSelectedInputAudioDevice) {
                    switchedAudioInputToHMD = true;
                    AudioDevice.setInputDeviceAsync(hmdPreferredAudioInput)
                }
            }
            if (hmdPreferredAudioOutput !== "") {
                debug("HMD has preferred audio output device.");
                previousSelectedOutputAudioDevice = Settings.getValue(OUTPUT_DEVICE_SETTING);
                debug("previousSelectedOutputAudioDevice: " + previousSelectedOutputAudioDevice);
                if (hmdPreferredAudioOutput != previousSelectedOutputAudioDevice) {
                    switchedAudioOutputToHMD = true;
                    AudioDevice.setOutputDeviceAsync(hmdPreferredAudioOutput)
                }
            }
        } else {
            debug("HMD no longer active. Restoring audio I/O devices...");
            restoreAudio();
        }
    }
    wasHmdActive = HMD.active;
}

//
// END FUNCTION DEFINITIONS
//

//
// BEGIN SCRIPT BODY
//
// Wait for the C++ systems to fire up before trying to do anything with audio devices
Script.setTimeout(function () {
    debug("Connecting deviceChanged(), displayModeChanged(), and switchAudioDevice()...");
    AudioDevice.deviceChanged.connect(onDevicechanged);
    AudioDevice.currentInputDeviceChanged.connect(onCurrentDeviceChanged);
    AudioDevice.currentOutputDeviceChanged.connect(onCurrentDeviceChanged);
    HMD.displayModeChanged.connect(checkHMDAudio);
    Menu.menuItemEvent.connect(onMenuEvent);
    debug("Setting up Audio I/O menu for the first time...");
    setupAudioMenus();
    debug("Checking HMD audio status...")
    checkHMDAudio();
}, SELECT_AUDIO_SCRIPT_STARTUP_TIMEOUT);

debug("Connecting scriptEnding()");
Script.scriptEnding.connect(function () {
    restoreAudio();
    removeAudioMenus();
    Menu.menuItemEvent.disconnect(onMenuEvent);
    HMD.displayModeChanged.disconnect(checkHMDAudio);
    AudioDevice.currentInputDeviceChanged.disconnect(onCurrentDeviceChanged);
    AudioDevice.currentOutputDeviceChanged.disconnect(onCurrentDeviceChanged);
    AudioDevice.deviceChanged.disconnect(onDevicechanged);
});

//
// END SCRIPT BODY
//

}()); // END LOCAL_SCOPE
