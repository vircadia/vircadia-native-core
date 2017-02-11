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

(function() { // BEGIN LOCAL_SCOPE

var button;
var TOOLBAR_BUTTON_NAME = "MUTE";
var TABLET_BUTTON_NAME = "AUDIO";
var toolBar = null;
var tablet = null;
var isHUDUIEnabled = Settings.getValue("HUDUIEnabled");
var HOME_BUTTON_TEXTURE = "http://hifi-content.s3.amazonaws.com/alan/dev/tablet-with-home-button.fbx/tablet-with-home-button.fbm/button-root.png";

function onMuteToggled() {
    if (isHUDUIEnabled) {
        button.editProperties({ isActive: AudioDevice.getMuted() });
    }
}
function onClicked(){
    if (isHUDUIEnabled) {
        var menuItem = "Mute Microphone";
        Menu.setIsOptionChecked(menuItem, !Menu.isOptionChecked(menuItem));
    } else {
        var entity = HMD.tabletID;
        Entities.editEntity(entity, { textures: JSON.stringify({ "tex.close": HOME_BUTTON_TEXTURE }) });
        tablet.gotoMenuScreen("Audio");
    }
}

if (Settings.getValue("HUDUIEnabled")) {
    toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");
    button = toolBar.addButton({
        objectName: TOOLBAR_BUTTON_NAME,
        imageURL: Script.resolvePath("assets/images/tools/mic.svg"),
        visible: true,
        alpha: 0.9
    });
} else {
    tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    button = tablet.addButton({
        icon: "icons/tablet-icons/mic-i.svg",
        text: TABLET_BUTTON_NAME,
        sortOrder: 1
    });
}
onMuteToggled();

button.clicked.connect(onClicked);
AudioDevice.muteToggled.connect(onMuteToggled);

Script.scriptEnding.connect(function () {
    button.clicked.disconnect(onClicked);
    AudioDevice.muteToggled.disconnect(onMuteToggled);
    if (tablet) {
        tablet.removeButton(button);
    }
    if (toolBar) {
        toolBar.removeButton(TOOLBAR_BUTTON_NAME);
    }
});

}()); // END LOCAL_SCOPE
