"use strict";

/*
	clapApp.js
	unpublishedScripts/marketplace/clap/clapApp.js

	Created by Matti 'Menithal' Lahtinen on 9/11/2017
	Copyright 2017 High Fidelity, Inc.

	Distributed under the Apache License, Version 2.0.
	See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
*/

// Entry Script for the clap app

// Load up engine
var APP_NAME = "CLAP";
var ClapEngine = Script.require(Script.resolvePath("scripts/ClapEngine.js?v9"));
var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

// Define Menu
var blackIcon = Script.resolvePath("icons/tablet-icons/clap-a.svg?foxv2");
var whiteIcon = Script.resolvePath("icons/tablet-icons/clap-i.svg?foxv2");

if (Settings.getValue("clapAppEnabled") === undefined) {
    Settings.setValue("clapAppEnabled", true);
}
var isActive = Settings.getValue("clapAppEnabled");

var activeButton = tablet.addButton({
    icon: whiteIcon,
    activeIcon: blackIcon,
    text: APP_NAME,
    isActive: isActive,
    sortOrder: 11
});

if (isActive) {
    ClapEngine.connectEngine();
}

function onClick(enabled) {

    isActive = !isActive;
    Settings.setValue("clapAppEnabled", isActive);
    activeButton.editProperties({
        isActive: isActive
    });
    if (isActive) {
        ClapEngine.connectEngine();
    } else {
        ClapEngine.disconnectEngine();
    }
}
activeButton.clicked.connect(onClick);

Script.scriptEnding.connect(function () {
    ClapEngine.disconnectEngine();
    activeButton.clicked.disconnect(onClick);
    tablet.removeButton(activeButton);
});
