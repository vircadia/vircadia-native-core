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
/* globals HMD, Script, Menu, Tablet, Camera */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

(function() { // BEGIN LOCAL_SCOPE

var headset; // The preferred headset. Default to the first one found in the following list.
var displayMenuName = "Display";
var desktopMenuItemName = "Desktop";
['OpenVR (Vive)', 'Oculus Rift'].forEach(function (name) {
    if (!headset && Menu.menuItemExists(displayMenuName, name)) {
        headset = name;
    }
});

var controllerDisplay = false;
function updateControllerDisplay() {
    if (HMD.active && Menu.isOptionChecked("Third Person")) {
        if (!controllerDisplay) {
            HMD.requestShowHandControllers();
            controllerDisplay = true;
        }
    } else if (controllerDisplay) {
        HMD.requestHideHandControllers();
        controllerDisplay = false;
    }
}

var button;
var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

// Independent and Entity mode make people sick; disable them in hmd.
var desktopOnlyViews = ['Independent Mode', 'Entity Mode'];

var switchToVR = "ENTER VR";
var switchToDesktop = "EXIT VR";

function onHmdChanged(isHmd) {
    HMD.closeTablet();
    if (isHmd) {
        button.editProperties({
            icon: "icons/tablet-icons/switch-desk-i.svg",
            text: switchToDesktop
        });
    } else {
        button.editProperties({
            icon: "icons/tablet-icons/switch-vr-i.svg",
            text: switchToVR
        });
    }
    desktopOnlyViews.forEach(function (view) {
        Menu.setMenuEnabled("View>" + view, !isHmd);
    });
    updateControllerDisplay();
}

function onClicked() {
    var isDesktop = Menu.isOptionChecked(desktopMenuItemName);
    Menu.setIsOptionChecked(isDesktop ? headset : desktopMenuItemName, true);
    if (!isDesktop) {
        UserActivityLogger.logAction("exit_vr");
    }
}

if (headset) {
    button = tablet.addButton({
        icon: HMD.active ? "icons/tablet-icons/switch-desk-i.svg" : "icons/tablet-icons/switch-vr-i.svg",
        text: HMD.active ? switchToDesktop : switchToVR,
        sortOrder: 2
    });
    onHmdChanged(HMD.active);

    button.clicked.connect(onClicked);
    HMD.displayModeChanged.connect(onHmdChanged);
    Camera.modeUpdated.connect(updateControllerDisplay);

    Script.scriptEnding.connect(function () {
        button.clicked.disconnect(onClicked);
        if (tablet) {
            tablet.removeButton(button);
        }
        HMD.displayModeChanged.disconnect(onHmdChanged);
        Camera.modeUpdated.disconnect(updateControllerDisplay);
    });
}

}()); // END LOCAL_SCOPE
