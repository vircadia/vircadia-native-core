"use strict";

//
//  help.js
//  scripts/system/
//
//  Created by Howard Stearns on 2 Nov 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* globals Tablet */

(function() { // BEGIN LOCAL_SCOPE

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        icon: "icons/tablet-icons/help-i.svg",
        text: "HELP"
    });
    var enabled = false;
    function onClicked() {
        var HELP_URL = Script.resourcesPath() + "html/help.html";

        // Similar logic to Application::showHelp()
        var defaultTab = "kbm";
        var handControllerName = "vive";
        if (HMD.active) {
            if ("Vive" in Controller.Hardware) {
                defaultTab = "handControllers";
                handControllerName = "vive";
            } else if ("OculusTouch" in Controller.Hardware) {
                defaultTab = "handControllers";
                handControllerName = "oculus";
            }
        } else if ("SDL2" in Controller.Hardware) {
            defaultTab = "gamepad";
        }
        var queryParameters = "handControllerName=" + handControllerName + "&defaultTab=" + defaultTab;
        print("Help enabled " + Menu.isMenuEnabled("Help..."))

        if (enabled) {
            Menu.closeInfoView('InfoView_html/help.html');
            enabled = !enabled;
            button.editProperties({isActive: enabled});
        } else {
            Menu.triggerOption('Help...');
            enabled = !enabled;
            button.editProperties({isActive: enabled});
        }
    }

    button.clicked.connect(onClicked);

    var POLL_RATE = 500;
    var interval = Script.setInterval(function () {
        var visible = Menu.isInfoViewVisible('InfoView_html/help.html');
        if (visible !== enabled) {
            enabled = visible;
            button.editProperties({isActive: enabled});
        }
    }, POLL_RATE);

    Script.scriptEnding.connect(function () {
        Script.clearInterval(interval);
        tablet.removeButton(button);
    });

}()); // END LOCAL_SCOPE
