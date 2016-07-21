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
    buttonState: 1,
    defaultState: 1,
    hoverState: 3,
    alpha: 0.9
});

function onMuteToggled() {
    // We could just toggle state, but we're less likely to get out of wack if we read the AudioDevice.
    // muted => button "on" state => 1. go figure.
    var state = AudioDevice.getMuted() ? 0 : 1;
    var hoverState = AudioDevice.getMuted() ? 2 : 3;
    button.writeProperty('buttonState', state);
    button.writeProperty('defaultState', state);
    button.writeProperty('hoverState', hoverState);
}
onMuteToggled();
function onClicked(){
    var menuItem = "Mute Microphone"; 
    Menu.setIsOptionChecked(menuItem, !Menu.isOptionChecked(menuItem));
}
button.clicked.connect(onClicked);
AudioDevice.muteToggled.connect(onMuteToggled);

Script.scriptEnding.connect(function () {
    toolBar.removeButton("mute");
    button.clicked.disconnect(onClicked);
    AudioDevice.muteToggled.disconnect(onMuteToggled);
});
