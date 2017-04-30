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
function parseMenuItem(item) {
	const USE = "Use ";
	const FOR_INPUT = " for " + INPUT;
	const FOR_OUTPUT = " for " + OUTPUT;
	if (item.slice(0, USE.length) == USE) {
		if (item.slice(-FOR_INPUT.length) == FOR_INPUT) {
			return { device: item.slice(USE.length, -FOR_INPUT.length), mode: INPUT };
		} else if (item.slice(-FOR_OUTPUT.length) == FOR_OUTPUT) {
			return { device: item.slice(USE.length, -FOR_OUTPUT.length), mode: OUTPUT };
		}
	}
}

//
// VAR DEFINITIONS
//
var debugPrintStatements = true;
const INPUT_DEVICE_SETTING = "audio_input_device";
const OUTPUT_DEVICE_SETTING = "audio_output_device";
var audioDevicesList = [];
var wasHmdActive = false; // assume it's not active to start
var switchedAudioInputToHMD = false;
var switchedAudioOutputToHMD = false;
var previousSelectedInputAudioDevice = "";
var previousSelectedOutputAudioDevice = "";
var skipMenuEvents = true;

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
    skipMenuEvents = true;
    Script.setTimeout(function() { skipMenuEvents = false; }, 200);

    removeAudioMenus();

    // Setup audio input devices
    Menu.addSeparator("Audio", "Input Audio Device");
    var inputDevices = AudioDevice.getInputDevices();
    var currentInputDevice = AudioDevice.getInputDevice()
    for (var i = 0; i < inputDevices.length; i++) {
        var audioDeviceMenuString = "Use " + inputDevices[i] + " for Input";
        Menu.addMenuItem({
            menuName: "Audio",
            menuItemName: audioDeviceMenuString,
            isCheckable: true,
            isChecked: inputDevices[i] == currentInputDevice
        });
        audioDevicesList.push(audioDeviceMenuString);
    }

    // Setup audio output devices
    Menu.addSeparator("Audio", "Output Audio Device");
    var outputDevices = AudioDevice.getOutputDevices();
    var currentOutputDevice = AudioDevice.getOutputDevice()
    for (var i = 0; i < outputDevices.length; i++) {
        var audioDeviceMenuString = "Use " + outputDevices[i] + " for Output";
        Menu.addMenuItem({
            menuName: "Audio",
            menuItemName: audioDeviceMenuString,
            isCheckable: true,
            isChecked: outputDevices[i] == currentOutputDevice
        });
        audioDevicesList.push(audioDeviceMenuString);
    }
}

function checkDeviceMismatch() {
    var inputDeviceSetting = Settings.getValue(INPUT_DEVICE_SETTING);
    var interfaceInputDevice = AudioDevice.getInputDevice();
    if (interfaceInputDevice != inputDeviceSetting) {
        debug("Input Setting & Device mismatch! Input SETTING: " + inputDeviceSetting + "Input DEVICE IN USE: " + interfaceInputDevice);
        switchAudioDevice("Use " + inputDeviceSetting + " for Input");
    }

    var outputDeviceSetting = Settings.getValue(OUTPUT_DEVICE_SETTING);
    var interfaceOutputDevice = AudioDevice.getOutputDevice();
    if (interfaceOutputDevice != outputDeviceSetting) {
        debug("Output Setting & Device mismatch! Output SETTING: " + outputDeviceSetting + "Output DEVICE IN USE: " + interfaceOutputDevice);
        switchAudioDevice("Use " + outputDeviceSetting + " for Output");
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
    checkDeviceMismatch();
}

function onMenuEvent(audioDeviceMenuString) {
    if (!skipMenuEvents) {
        switchAudioDevice(audioDeviceMenuString);
    }
}

function onCurrentDeviceChanged() {
    debug("System audio device switched. ");
    setupAudioMenus()
}

function switchAudioDevice(audioDeviceMenuString) {
    // if the device is not plugged in, short-circuit
    if (!~audioDevicesList.indexOf(audioDeviceMenuString)) {
        return;
    }

    var selection = parseMenuItem(audioDeviceMenuString);
    if (!selection) {
        debug("Invalid Audio audioDeviceMenuString! Doesn't end with 'for Input' or 'for Output'");
        return;
    }

    // menu events can be triggered asynchronously; skip them for 200ms to avoid recursion and false switches
    skipMenuEvents = true;
    Script.setTimeout(function() { skipMenuEvents = false; }, 200);

    var selectedDevice = selection.device;
    if (selection.mode == INPUT) {
        var currentInputDevice = AudioDevice.getInputDevice();
        if (selectedDevice != currentInputDevice) {
            debug("Switching audio INPUT device from " + currentInputDevice + " to " + selectedDevice);
            Menu.setIsOptionChecked("Use " + currentInputDevice + " for Input", false);
            if (AudioDevice.setInputDevice(selectedDevice)) {
                Settings.setValue(INPUT_DEVICE_SETTING, selectedDevice);
                Menu.setIsOptionChecked(audioDeviceMenuString, true);
            } else {
                debug("Error setting audio input device!")
                Menu.setIsOptionChecked(audioDeviceMenuString, false);
            }
        } else {
            debug("Selected input device is the same as the current input device!")
            Settings.setValue(INPUT_DEVICE_SETTING, selectedDevice);
            Menu.setIsOptionChecked(audioDeviceMenuString, true);
            AudioDevice.setInputDevice(selectedDevice); // Still try to force-set the device (in case the user's trying to forcefully debug an issue)
        }
    } else if (selection.mode == OUTPUT) {
        var currentOutputDevice = AudioDevice.getOutputDevice();
        if (selectedDevice != currentOutputDevice) {
            debug("Switching audio OUTPUT device from " + currentOutputDevice + " to " + selectedDevice);
            Menu.setIsOptionChecked("Use " + currentOutputDevice + " for Output", false);
            if (AudioDevice.setOutputDevice(selectedDevice)) {
                Settings.setValue(OUTPUT_DEVICE_SETTING, selectedDevice);
                Menu.setIsOptionChecked(audioDeviceMenuString, true);
            } else {
                debug("Error setting audio output device!")
                Menu.setIsOptionChecked(audioDeviceMenuString, false);
            }
        } else {
            debug("Selected output device is the same as the current output device!")
            Settings.setValue(OUTPUT_DEVICE_SETTING, selectedDevice);
            Menu.setIsOptionChecked(audioDeviceMenuString, true);
            AudioDevice.setOutputDevice(selectedDevice); // Still try to force-set the device (in case the user's trying to forcefully debug an issue)
        }
    }
}

function restoreAudio() {
    if (switchedAudioInputToHMD) {
        debug("Switching back from HMD preferred audio input to: " + previousSelectedInputAudioDevice);
        switchAudioDevice("Use " +  previousSelectedInputAudioDevice + " for Input");
        switchedAudioInputToHMD = false;
    }
    if (switchedAudioOutputToHMD) {
        debug("Switching back from HMD preferred audio output to: " + previousSelectedOutputAudioDevice);
        switchAudioDevice("Use " + previousSelectedOutputAudioDevice + " for Output");
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
                    switchAudioDevice("Use " + hmdPreferredAudioInput + " for Input");
                }
            }
            if (hmdPreferredAudioOutput !== "") {
                debug("HMD has preferred audio output device.");
                previousSelectedOutputAudioDevice = Settings.getValue(OUTPUT_DEVICE_SETTING);
                debug("previousSelectedOutputAudioDevice: " + previousSelectedOutputAudioDevice);
                if (hmdPreferredAudioOutput != previousSelectedOutputAudioDevice) {
                    switchedAudioOutputToHMD = true;
                    switchAudioDevice("Use " + hmdPreferredAudioOutput + " for Output");
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
    checkDeviceMismatch();
    debug("Checking HMD audio status...")
    checkHMDAudio();
}, 3000);

debug("Connecting scriptEnding()");
Script.scriptEnding.connect(function () {
    restoreAudio();
    removeAudioMenus();
    Menu.menuItemEvent.disconnect(onMenuEvent);
    HMD.displayModeChanged.disconnect(checkHMDAudio);
    AudioDevice.deviceChanged.disconnect(onDevicechanged);
});

//
// END SCRIPT BODY
//

}()); // END LOCAL_SCOPE
