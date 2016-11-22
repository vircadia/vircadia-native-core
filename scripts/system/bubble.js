"use strict";

//
//  bubble.js
//  scripts/system/
//
//  Created by Brad Hefta-Gaub on 11/18/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* global Toolbars, Script, Users, Overlays, AvatarList, Controller, Camera, getControllerWorldLocation */


(function() { // BEGIN LOCAL_SCOPE

// grab the toolbar
var toolbar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");

var ASSETS_PATH = Script.resolvePath("assets");
var TOOLS_PATH = Script.resolvePath("assets/images/tools/");

function buttonImageURL() {
    return TOOLS_PATH + 'bubble.svg';
}

var bubbleActive = Users.getIgnoreRadiusEnabled();

// setup the mod button and add it to the toolbar
var button = toolbar.addButton({
    objectName: 'bubble',
    imageURL: buttonImageURL(),
    visible: true,
    buttonState: bubbleActive ? 0 : 1,
    defaultState: bubbleActive ? 0 : 1,
    hoverState: bubbleActive ? 2 : 3,
    alpha: 0.9
});


// handle clicks on the toolbar button
function buttonClicked(){
    Users.toggleIgnoreRadius();
    bubbleActive = Users.getIgnoreRadiusEnabled();
    button.writeProperty('buttonState', bubbleActive ? 0 : 1);
    button.writeProperty('defaultState', bubbleActive ? 0 : 1);
    button.writeProperty('hoverState', bubbleActive ? 2 : 3);
}

button.clicked.connect(buttonClicked);

// cleanup the toolbar button and overlays when script is stopped
Script.scriptEnding.connect(function() {
    toolbar.removeButton('bubble');
});

}()); // END LOCAL_SCOPE
