"use strict";

//
//  audio.js
//
//  Created by Howard Stearns on 2 Jun 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

(function() { // BEGIN LOCAL_SCOPE

var TABLET_BUTTON_NAME = "AUDIO";
var HOME_BUTTON_TEXTURE = "http://hifi-content.s3.amazonaws.com/alan/dev/tablet-with-home-button.fbx/tablet-with-home-button.fbm/button-root.png";

var MUTE_ICONS = {
    icon: "icons/tablet-icons/mic-mute-i.svg",
    activeIcon: "icons/tablet-icons/mic-mute-a.svg"
};

var UNMUTE_ICONS = {
    icon: "icons/tablet-icons/mic-unmute-i.svg",
    activeIcon: "icons/tablet-icons/mic-unmute-a.svg"
};

function onMuteToggled() {
    if (AudioDevice.getMuted()) {
        button.editProperties(MUTE_ICONS);
    } else {
        button.editProperties(UNMUTE_ICONS);
    }
}

var shouldActivateButton = false;
var onAudioScreen = false;

function onClicked() {
    if (onAudioScreen) {
        // for toolbar-mode: go back to home screen, this will close the window.
        tablet.gotoHomeScreen();
    } else {
        var entity = HMD.tabletID;
        Entities.editEntity(entity, { textures: JSON.stringify({ "tex.close": HOME_BUTTON_TEXTURE }) });
        shouldActivateButton = true;
        tablet.gotoMenuScreen("Audio");
        onAudioScreen = true;
    }
}

function onScreenChanged(type, url) {
    // for toolbar mode: change button to active when window is first openend, false otherwise.
    button.editProperties({isActive: shouldActivateButton});
    shouldActivateButton = false;
    onAudioScreen = false;
}

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var button = tablet.addButton({
    icon: AudioDevice.getMuted() ? MUTE_ICONS.icon : UNMUTE_ICONS.icon,
    activeIcon: AudioDevice.getMuted() ? MUTE_ICONS.activeIcon : UNMUTE_ICONS.activeIcon,
    text: TABLET_BUTTON_NAME,
    sortOrder: 1
});

onMuteToggled();

button.clicked.connect(onClicked);
tablet.screenChanged.connect(onScreenChanged);
AudioDevice.muteToggled.connect(onMuteToggled);

Script.scriptEnding.connect(function () {
    if (onAudioScreen) {
        tablet.gotoHomeScreen();
    }
    button.clicked.disconnect(onClicked);
    tablet.screenChanged.disconnect(onScreenChanged);
    AudioDevice.muteToggled.disconnect(onMuteToggled);
    tablet.removeButton(button);
});

}()); // END LOCAL_SCOPE
