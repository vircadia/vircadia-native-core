"use strict";
//
//  modes.js
//  scripts/system/
//
//  Created by Gabriel Calero & Cristian Duarte on Jan 23, 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function() { // BEGIN LOCAL_SCOPE

var modeButton;
var currentMode;
var barQml;

var SETTING_CURRENT_MODE_KEY = 'Android/Mode';
var MODE_VR = "VR", MODE_RADAR = "RADAR", MODE_MY_VIEW = "MY VIEW";
var DEFAULT_MODE = MODE_MY_VIEW;
var nextMode = {};
nextMode[MODE_RADAR]=MODE_MY_VIEW;
nextMode[MODE_MY_VIEW]=MODE_RADAR;
var modeLabel = {};
modeLabel[MODE_RADAR]="TOP VIEW";
modeLabel[MODE_MY_VIEW]="MY VIEW";

var logEnabled = false;
var radar = Script.require('./radar.js');
var uniqueColor = Script.require('./uniqueColor.js');
var displayNames = Script.require('./displayNames.js');
var clickWeb = Script.require('./clickWeb.js');

function printd(str) {
    if (logEnabled) {       
        print("[modes.js] " + str);
    }
}

function init() {
    radar.setUniqueColor(uniqueColor);
    radar.init();
    
    barQml = new QmlFragment({
        qml: "hifi/modesbar.qml"
    });
    modeButton = barQml.addButton({
        icon: "icons/myview-a.svg",
        activeBgOpacity: 0.0,
        hoverBgOpacity: 0.0,
        activeHoverBgOpacity: 0.0,
        text: "MODE",
        height:240,
        bottomMargin: 16,
        textSize: 38,
        fontFamily: "Raleway",
        fontBold: true

    });
    
    switchToMode(getCurrentModeSetting());
    
    modeButton.entered.connect(modeButtonPressed);
    modeButton.clicked.connect(modeButtonClicked);
}

function shutdown() {
    modeButton.entered.disconnect(modeButtonPressed);
    modeButton.clicked.disconnect(modeButtonClicked);
}

function modeButtonPressed() {
   Controller.triggerHapticPulseOnDevice(Controller.findDevice("TouchscreenVirtualPad"), 0.1, 40.0, 0);
}

function modeButtonClicked() {
    switchToMode(nextMode[currentMode]);
}

function saveCurrentModeSetting(mode) {
    Settings.setValue(SETTING_CURRENT_MODE_KEY, mode);
}

function getCurrentModeSetting(mode) {
    return Settings.getValue(SETTING_CURRENT_MODE_KEY, DEFAULT_MODE);
}

function switchToMode(newMode) {
    // before leaving radar mode
    if (currentMode == MODE_RADAR) {
        radar.endRadarMode();
    }
    currentMode = newMode;
    modeButton.text = modeLabel[currentMode];

    saveCurrentModeSetting(currentMode);

    if (currentMode == MODE_RADAR) {
        radar.startRadarMode();
        displayNames.ending();
        clickWeb.ending();
    } else  if (currentMode == MODE_MY_VIEW) {
        // nothing to do yet
        displayNames.init();
        clickWeb.init();
    } else {
        printd("Unknown view mode " + currentMode);
    }
    
}

function sendToQml(o) {
    if(barQml) {
      barQml.sendToQml(o);  
    }
}

Script.scriptEnding.connect(function () {
    shutdown();
});

init();

}()); // END LOCAL_SCOPE