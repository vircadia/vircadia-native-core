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

var toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");


var button = toolBar.addButton({
    objectName: "mute",
    imageURL: Script.resolvePath("assets/images/tools/mic.svg"),
    visible: true,
    alpha: 0.9,
});

function onMuteToggled() {
    // We could just toggle state, but we're less likely to get out of wack if we read the AudioDevice.
    // muted => "on" button state => buttonState 1
    button.writeProperty('buttonState', AudioDevice.getMuted() ? 1 : 0);
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
    AudioDevice.muteToggled.disconnect(onMuteToggled);
});
