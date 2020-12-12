"use strict";

//
//  quickGoto.js
//  scripts/system/
//
//  Created by Dante Ruiz
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* globals Tablet, Toolbars, Script, HMD, DialogsManager */

(function() { // BEGIN LOCAL_SCOPE

    function addGotoButton(destination) {
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        button = tablet.addButton({
            icon: "icons/tablet-icons/goto-i.svg",
            activeIcon: "icons/tablet-icons/goto-a.svg",
            text: destination
        });
        var buttonDestination = destination;
        button.clicked.connect(function() {
            Window.location = "hifi://" + buttonDestination;
        });
        Script.scriptEnding.connect(function () {
            tablet.removeButton(button);
        });
    }

    addGotoButton("hub.daleglass.net");
    addGotoButton("lq-hub.vircadia.com");
    addGotoButton("file:///~/serverless/tutorial.json");

}()); // END LOCAL_SCOPE
