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

function onMuteToggled() {
    button.editProperties({ isActive: AudioDevice.getMuted() });
}
function onClicked(){
    var entity = HMD.tabletID;
    Entities.editEntity(entity, { textures: JSON.stringify({ "tex.close": HOME_BUTTON_TEXTURE }) });
    tablet.gotoMenuScreen("Audio");
}

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var button = tablet.addButton({
    icon: "icons/tablet-icons/mic-unmute-i.svg",
    activeIcon: "icons/tablet-icons/mic-mute-a.svg",
    text: TABLET_BUTTON_NAME,
    sortOrder: 1
});

onMuteToggled();

button.clicked.connect(onClicked);
AudioDevice.muteToggled.connect(onMuteToggled);

Script.scriptEnding.connect(function () {
    button.clicked.disconnect(onClicked);
    AudioDevice.muteToggled.disconnect(onMuteToggled);
    tablet.removeButton(button);
});

}()); // END LOCAL_SCOPE
