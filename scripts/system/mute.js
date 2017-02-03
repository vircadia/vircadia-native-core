"use strict";

//
//  goto.js
//  scripts/system/
//
//  Created by Howard Stearns on 2 Jun 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE

var button;
var buttonName = "MUTE";
var toolBar = null;
var tablet = null;

function onMuteToggled() {
    button.editProperties({isActive: AudioDevice.getMuted()});
}
function onClicked(){
    var menuItem = "Mute Microphone";
    Menu.setIsOptionChecked(menuItem, !Menu.isOptionChecked(menuItem));
}

if (Settings.getValue("HUDUIEnabled")) {
    toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");
    button = toolBar.addButton({
        objectName: buttonName,
        imageURL: Script.resolvePath("assets/images/tools/mic.svg"),
        visible: true,
        alpha: 0.9
    });
} else {
    tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    button = tablet.addButton({
        icon: "icons/tablet-icons/mic-a.svg",
        text: buttonName,
        activeIcon: "icons/tablet-icons/mic-i.svg",
        activeText: "UNMUTE",
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
        toolBar.removeButton(buttonName);
    }
});

}()); // END LOCAL_SCOPE
