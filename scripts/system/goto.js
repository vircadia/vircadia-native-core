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
/* globals Tablet, Toolbars, Script, HMD, DialogsManager */

(function() { // BEGIN LOCAL_SCOPE

var button;
var buttonName = "GOTO";
var toolBar = null;
var tablet = null;
var onGotoScreen = false;
function onAddressBarShown(visible) {
    button.editProperties({isActive: visible});
}

function onClicked(){
    DialogsManager.toggleAddressBar();
    onGotoScreen = !onGotoScreen;
}

if (Settings.getValue("HUDUIEnabled")) {
    toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");
    button = toolBar.addButton({
        objectName: buttonName,
        imageURL: Script.resolvePath("assets/images/tools/directory.svg"),
        visible: true,
        alpha: 0.9
    });
} else {
    tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    button = tablet.addButton({
        icon: "icons/tablet-icons/goto-i.svg",
        activeIcon: "icons/tablet-icons/goto-a.svg",
        text: buttonName,
        sortOrder: 8
    });
}

button.clicked.connect(onClicked);
DialogsManager.addressBarShown.connect(onAddressBarShown);

Script.scriptEnding.connect(function () {
    if (onGotoScreen) {
        DialogsManager.toggleAddressBar();
    }
    button.clicked.disconnect(onClicked);
    if (tablet) {
        tablet.removeButton(button);
    }
    if (toolBar) {
        toolBar.removeButton(buttonName);
    }
    DialogsManager.addressBarShown.disconnect(onAddressBarShown);
});

}()); // END LOCAL_SCOPE
