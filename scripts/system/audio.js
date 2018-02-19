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
var AUDIO_QML_SOURCE = "hifi/audio/Audio.qml";

var MUTE_ICONS = {
    icon: "icons/tablet-icons/mic-mute-i.svg",
    activeIcon: "icons/tablet-icons/mic-mute-a.svg"
};

var UNMUTE_ICONS = {
    icon: "icons/tablet-icons/mic-unmute-i.svg",
    activeIcon: "icons/tablet-icons/mic-unmute-a.svg"
};

function onMuteToggled() {
    if (Audio.muted) {
        button.editProperties(MUTE_ICONS);
    } else {
        button.editProperties(UNMUTE_ICONS);
    }
}

var onAudioScreen = false;

function onClicked() {
    if (onAudioScreen) {
        // for toolbar-mode: go back to home screen, this will close the window.
        tablet.gotoHomeScreen();
    } else {
        var entity = HMD.tabletID;
        Entities.editEntity(entity, { textures: JSON.stringify({ "tex.close": HOME_BUTTON_TEXTURE }) });
        tablet.loadQMLSource(AUDIO_QML_SOURCE);
    }
}

function onScreenChanged(type, url) {
    onAudioScreen = (type === "QML" && url === AUDIO_QML_SOURCE);
    // for toolbar mode: change button to active when window is first openend, false otherwise.
    button.editProperties({isActive: onAudioScreen});
}

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var button = tablet.addButton({
    icon: Audio.muted ? MUTE_ICONS.icon : UNMUTE_ICONS.icon,
    activeIcon: Audio.muted ? MUTE_ICONS.activeIcon : UNMUTE_ICONS.activeIcon,
    text: TABLET_BUTTON_NAME,
    sortOrder: 1
});

onMuteToggled();

button.clicked.connect(onClicked);
tablet.screenChanged.connect(onScreenChanged);
Audio.mutedChanged.connect(onMuteToggled);

Script.scriptEnding.connect(function () {
    if (onAudioScreen) {
        tablet.gotoHomeScreen();
    }
    button.clicked.disconnect(onClicked);
    tablet.screenChanged.disconnect(onScreenChanged);
    Audio.mutedChanged.disconnect(onMuteToggled);
    tablet.removeButton(button);
});

}()); // END LOCAL_SCOPE
