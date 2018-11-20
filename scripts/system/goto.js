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

addGotoButton("hell")
addGotoButton("dev-mobile")

}()); // END LOCAL_SCOPE
