"use strict";

//
//  hmd.js
//  scripts/system/
//
//  Created by Howard Stearns on 2 Jun 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE

var headset; // The preferred headset. Default to the first one found in the following list.
var displayMenuName = "Display";
var desktopMenuItemName = "Desktop";
['OpenVR (Vive)', 'Oculus Rift'].forEach(function (name) {
    if (!headset && Menu.menuItemExists(displayMenuName, name)) {
        headset = name;
    }
});

var toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");
var button;
// Independent and Entity mode make people sick. Third Person and Mirror have traps that we need to work through.
// Disable them in hmd.
var desktopOnlyViews = ['Third Person', 'Mirror', 'Independent Mode', 'Entity Mode'];
function onHmdChanged(isHmd) {
    button.writeProperty('buttonState', isHmd ? 0 : 1);
    button.writeProperty('defaultState', isHmd ? 0 : 1);
    button.writeProperty('hoverState', isHmd ? 2 : 3);
    desktopOnlyViews.forEach(function (view) {
        Menu.setMenuEnabled("View>" + view, !isHmd);
    });
}
function onClicked(){
    var isDesktop = Menu.isOptionChecked(desktopMenuItemName);
    Menu.setIsOptionChecked(isDesktop ? headset : desktopMenuItemName, true);
}
if (headset) {
    button = toolBar.addButton({
        objectName: "hmdToggle",
        imageURL: Script.resolvePath("assets/images/tools/switch.svg"),
        visible: true,
        hoverState: 2,
        defaultState: 0,
        alpha: 0.9
    });
    onHmdChanged(HMD.active);

    button.clicked.connect(onClicked);
    HMD.displayModeChanged.connect(onHmdChanged);

    Script.scriptEnding.connect(function () {
        toolBar.removeButton("hmdToggle");
        button.clicked.disconnect(onClicked);
        HMD.displayModeChanged.disconnect(onHmdChanged);
    });
}

}()); // END LOCAL_SCOPE
