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
    "system/request-service.js",
    "system/progress.js",
    "system/away.js",
    "system/audio.js",
    "system/hmd.js",
    "system/menu.js",
    "system/bubble.js",
    "system/snapshot.js",
    "system/pal.js", // "system/mod.js", // older UX, if you prefer
    "system/avatarapp.js",
    "system/makeUserConnection.js",
    "system/tablet-goto.js",
    "system/marketplaces/marketplaces.js",
    "system/notifications.js",
    "system/commerce/wallet.js",
    "system/create/edit.js",
    "system/dialTone.js",
    "system/firstPersonHMD.js",
    "system/tablet-ui/tabletUI.js",
    "system/emote.js",
    "system/miniTablet.js",
    "system/audioMuteOverlay.js",
    "system/keyboardShortcuts/keyboardShortcuts.js"
];
var DEFAULT_SCRIPTS_SEPARATE = [
    "system/controllers/controllerScripts.js",
    "communityModules/notificationCore/notificationCore.js",
    "communityModules/chat/FloofChat.js"
    //"system/chat.js"
];

if (Window.interstitialModeEnabled) {
    // Insert interstitial scripts at front so that they're started first.
    DEFAULT_SCRIPTS_COMBINED.splice(0, 0, "system/interstitialPage.js", "system/redirectOverlays.js");
}

// add a menu item for debugging
var MENU_CATEGORY = "Developer > Scripting";
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
    });
}

function loadSeparateDefaults() {
    var currentlyRunningScripts = ScriptDiscoveryService.getRunning();

    for (var i in DEFAULT_SCRIPTS_SEPARATE) {
        var shouldLoadCurrentDefaultScript = true;

        for (var j = 0; j < currentlyRunningScripts.length; j++) {
            var currentRunningScriptObject = currentlyRunningScripts[j];
            var currentDefaultScriptName = DEFAULT_SCRIPTS_SEPARATE[i].substr((DEFAULT_SCRIPTS_SEPARATE[i].lastIndexOf("/") + 1), DEFAULT_SCRIPTS_SEPARATE[i].length);
            if (currentDefaultScriptName === currentRunningScriptObject.name) {
                shouldLoadCurrentDefaultScript = false;
            }
        }

        if (shouldLoadCurrentDefaultScript) {
            Script.load(DEFAULT_SCRIPTS_SEPARATE[i]);
        }
    }
}

function runDefaultsTogether() {
    var currentlyRunningScripts = ScriptDiscoveryService.getRunning();

    for (var i = 0; i < DEFAULT_SCRIPTS_COMBINED.length; i++) {
        var shouldIncludeCurrentDefaultScript = true;

        for (var j = 0; j < currentlyRunningScripts.length; j++) {
            var currentRunningScriptObject = currentlyRunningScripts[j];
            var currentDefaultScriptName = DEFAULT_SCRIPTS_COMBINED[i].substr((DEFAULT_SCRIPTS_COMBINED[i].lastIndexOf("/") + 1), DEFAULT_SCRIPTS_COMBINED[i].length);
            if (currentDefaultScriptName === currentRunningScriptObject.name) {
                shouldIncludeCurrentDefaultScript = false;
            }
        }

        if (shouldIncludeCurrentDefaultScript) {
            Script.include(DEFAULT_SCRIPTS_COMBINED[i]);
        }
    }
}

function runDefaultsSeparately() {
    var currentlyRunningScripts = ScriptDiscoveryService.getRunning();

    for (var i in DEFAULT_SCRIPTS_COMBINED) {
        var shouldLoadCurrentDefaultScript = true;

        for (var j = 0; j < currentlyRunningScripts.length; j++) {
            var currentRunningScriptObject = currentlyRunningScripts[j];
            var currentDefaultScriptName = DEFAULT_SCRIPTS_COMBINED[i].substr((DEFAULT_SCRIPTS_COMBINED[i].lastIndexOf("/") + 1), DEFAULT_SCRIPTS_COMBINED[i].length);
            if (currentDefaultScriptName === currentRunningScriptObject.name) {
                shouldLoadCurrentDefaultScript = false;
            }
        }

        if (shouldLoadCurrentDefaultScript) {
            Script.load(DEFAULT_SCRIPTS_COMBINED[i]);
        }
    }
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
loadSeparateDefaults();

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
