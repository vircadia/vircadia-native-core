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

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

function muteURL() {
    return "icons/tablet-icons/" + (AudioDevice.getMuted() ? "mic-a.svg" : "mic-i.svg"); 
}
var button = tablet.addButton({
    icon: "icons/tablet-icons/mic-a.svg",
    text: "MUTE",
    activeIcon: "icons/tablet-icons/mic-i.svg",
    activeText: "UNMUTE"
});

function onMuteToggled() {
    button.editProperties({icon: muteURL()});
}
onMuteToggled();
function onClicked(){
    var menuItem = "Mute Microphone";
    Menu.setIsOptionChecked(menuItem, !Menu.isOptionChecked(menuItem));
}
button.clicked.connect(onClicked);
AudioDevice.muteToggled.connect(onMuteToggled);

Script.scriptEnding.connect(function () {
    button.clicked.disconnect(onClicked);
    tablet.removeButton(button);
    AudioDevice.muteToggled.disconnect(onMuteToggled);
});

}()); // END LOCAL_SCOPE
