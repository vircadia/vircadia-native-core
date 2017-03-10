"use strict";

//
//  generalSettings.js
//  scripts/system/
//
//  Created by Dante Ruiz on 9 Feb 2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* globals Tablet, Toolbars, Script, HMD, DialogsManager */

(function() { // BEGIN LOCAL_SCOPE

    var button;
    var buttonName = "Settings";
    var toolBar = null;
    var tablet = null;
    var settings = "TabletGeneralPreferences.qml"
    function onClicked(){
        if (tablet) {
            tablet.loadQMLSource(settings);
        }
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
            text: buttonName,
            sortOrder: 8
        });
    }

    button.clicked.connect(onClicked);

    Script.scriptEnding.connect(function () {
        button.clicked.disconnect(onClicked);
        if (tablet) {
            tablet.removeButton(button);
        }
        if (toolBar) {
            toolBar.removeButton(buttonName);
        }
    });

}()); // END LOCAL_SCOPE
