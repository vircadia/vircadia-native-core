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
        text: "HELP"
    });

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
        tablet.gotoWebScreen(HELP_URL + "?" + queryParameters);
    }

    button.clicked.connect(onClicked);

    Script.scriptEnding.connect(function () {
        tablet.removeButton(button);
    });

}()); // END LOCAL_SCOPE
