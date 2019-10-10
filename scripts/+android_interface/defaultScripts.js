"use strict";
/* jslint vars: true, plusplus: true */

//
//  defaultScripts.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var DEFAULT_SCRIPTS_COMBINED = [
    "system/progress.js",
    "system/+android_interface/touchscreenvirtualpad.js",
    "system/+android_interface/actionbar.js",
    "system/+android_interface/audio.js" ,
    "system/+android_interface/modes.js",
    "system/makeUserConnection.js"/*,
    "system/away.js",
    "system/controllers/controllerDisplayManager.js",
    "system/controllers/handControllerGrabAndroid.js",
    "system/controllers/handControllerPointerAndroid.js",
    "system/controllers/squeezeHands.js",
    "system/controllers/grab.js",
    "system/controllers/teleport.js",
    "system/controllers/toggleAdvancedMovementForHandControllers.js",
    "system/dialTone.js",
    "system/firstPersonHMD.js",
    "system/bubble.js",
    "system/android.js",
    "developer/debugging/debugAndroidMouse.js"*/
];

var DEBUG_SCRIPTS = [
    "system/+android_interface/stats.js"
];

var DEFAULT_SCRIPTS_SEPARATE = [ ];

// add a menu item for debugging
var MENU_CATEGORY = "Developer";
var MENU_ITEM = "Debug defaultScripts.js";

var SETTINGS_KEY = '_debugDefaultScriptsIsChecked';
var previousSetting = Settings.getValue(SETTINGS_KEY);

if (previousSetting === '' || previousSetting === false || previousSetting === 'false') {
    previousSetting = false;
}

if (previousSetting === true || previousSetting === 'true') {
    previousSetting = true;
}

if (Menu.menuExists(MENU_CATEGORY) && !Menu.menuItemExists(MENU_CATEGORY, MENU_ITEM)) {
    Menu.addMenuItem({
        menuName: MENU_CATEGORY,
        menuItemName: MENU_ITEM,
        isCheckable: true,
        isChecked: previousSetting,
        grouping: "Advanced"
    });
}

function loadSeparateDefaults() {
    for (var i in DEFAULT_SCRIPTS_SEPARATE) {
        Script.load(DEFAULT_SCRIPTS_SEPARATE[i]);
    }
}

function runDefaultsTogether() {
    for (var i in DEFAULT_SCRIPTS_COMBINED) {
        Script.include(DEFAULT_SCRIPTS_COMBINED[i]);
    }
    if (Script.isDebugMode()) {
        for (var i in DEBUG_SCRIPTS) {
            Script.include(DEBUG_SCRIPTS[i]);
        }
    }
    loadSeparateDefaults();
}

function runDefaultsSeparately() {
    for (var i in DEFAULT_SCRIPTS_COMBINED) {
        Script.load(DEFAULT_SCRIPTS_COMBINED[i]);
    }
    if (Script.isDebugMode()) {
        for (var i in DEBUG_SCRIPTS) {
            Script.load(DEBUG_SCRIPTS[i]);
        }
    }
    loadSeparateDefaults();
}

// start all scripts
if (Menu.isOptionChecked(MENU_ITEM)) {
    // we're debugging individual default scripts
    // so we load each into its own ScriptEngine instance
    runDefaultsSeparately();
} else {
    // include all default scripts into this ScriptEngine
    runDefaultsTogether();
}

function menuItemEvent(menuItem) {
    if (menuItem === MENU_ITEM) {
        var isChecked = Menu.isOptionChecked(MENU_ITEM);
        if (isChecked === true) {
            Settings.setValue(SETTINGS_KEY, true);
        } else if (isChecked === false) {
            Settings.setValue(SETTINGS_KEY, false);
        }
        Menu.triggerOption("Reload All Scripts");
    }
}

function removeMenuItem() {
    if (!Menu.isOptionChecked(MENU_ITEM)) {
        Menu.removeMenuItem(MENU_CATEGORY, MENU_ITEM);
    }
}

Script.scriptEnding.connect(function() {
    removeMenuItem();
});

Menu.menuItemEvent.connect(menuItemEvent);

var ANDROID_UI_AUTO_LOD_ADJUST = false;
var ANDROID_UI_LOD_ANGLE_DEG = 0.248;
LODManager.automaticLODAdjust = ANDROID_UI_AUTO_LOD_ADJUST;
LODManager.lodAngleDeg = ANDROID_UI_LOD_ANGLE_DEG;
