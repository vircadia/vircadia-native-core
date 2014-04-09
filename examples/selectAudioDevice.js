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

var selectedInputMenu = "";
var selectedOutputMenu = "";

function setupAudioMenus() {
    Menu.addMenu("Tools > Audio");
    Menu.addSeparator("Tools > Audio","Output Audio Device");

    var outputDevices = AudioDevice.getOutputDevices();
    var selectedOutputDevice = AudioDevice.getOutputDevice();

    for(var i = 0; i < outputDevices.length; i++) {
        var thisDeviceSelected = (outputDevices[i] == selectedOutputDevice);
        var menuItem = "Use " + outputDevices[i] + " for Output";
        Menu.addMenuItem({ 
                            menuName: "Tools > Audio", 
                            menuItemName: menuItem, 
                            isCheckable: true, 
                            isChecked: thisDeviceSelected 
                        });
        if (thisDeviceSelected) {
            selectedOutputMenu = menuItem;
        }
    }

    Menu.addSeparator("Tools > Audio","Input Audio Device");

    var inputDevices = AudioDevice.getInputDevices();
    var selectedInputDevice = AudioDevice.getInputDevice();

    for(var i = 0; i < inputDevices.length; i++) {
        var thisDeviceSelected = (inputDevices[i] == selectedInputDevice);
        var menuItem = "Use " + inputDevices[i] + " for Input";
        Menu.addMenuItem({ 
                            menuName: "Tools > Audio", 
                            menuItemName: menuItem, 
                            isCheckable: true, 
                            isChecked: thisDeviceSelected 
                        });
        if (thisDeviceSelected) {
            selectedInputMenu = menuItem;
        }
    }
}

setupAudioMenus();

function scriptEnding() {
    Menu.removeMenu("Tools > Audio");
}
Script.scriptEnding.connect(scriptEnding);


function menuItemEvent(menuItem) {
    if (menuItem.startsWith("Use ")) {
        if (menuItem.endsWith(" for Output")) {
            var selectedDevice = menuItem.trimStartsWith("Use ").trimEndsWith(" for Output");
            print("output audio selection..." + selectedDevice);
            Menu.setIsOptionChecked(selectedOutputMenu, false);
            selectedOutputMenu = menuItem;
            Menu.setIsOptionChecked(selectedOutputMenu, true);
            AudioDevice.setOutputDevice(selectedDevice);
            
        } else if (menuItem.endsWith(" for Input")) {
            var selectedDevice = menuItem.trimStartsWith("Use ").trimEndsWith(" for Input");
            print("input audio selection..." + selectedDevice);
            Menu.setIsOptionChecked(selectedInputMenu, false);
            selectedInputMenu = menuItem;
            Menu.setIsOptionChecked(selectedInputMenu, true);
            AudioDevice.setInputDevice(selectedDevice);
        }
    }
}

Menu.menuItemEvent.connect(menuItemEvent);
