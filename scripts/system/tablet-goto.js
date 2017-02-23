"use strict";

//
//  goto.js
//  scripts/system/
//
//  Created by Dante Ruiz on 8 February 2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE
    var gotoQmlSource = "TabletAddressDialog.qml";
    var buttonName = "GOTO";
    var onGotoScreen = false;
    var shouldActivateButton = false;

    function onClicked() {
        if (onGotoScreen) {
            // for toolbar-mode: go back to home screen, this will close the window.
            tablet.gotoHomeScreen();
        } else {
            shouldActivateButton = true;
            tablet.loadQMLSource(gotoQmlSource);
            onGotoScreen = true;
        }
    }

    function onScreenChanged(type, url) {
        // for toolbar mode: change button to active when window is first openend, false otherwise.
        button.editProperties({isActive: shouldActivateButton});
        shouldActivateButton = false;
        onGotoScreen = false;
    }

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        icon: "icons/tablet-icons/goto-i.svg",
        activeIcon: "icons/tablet-icons/goto-a.svg",
        text: buttonName,
        sortOrder: 8
    });

    button.clicked.connect(onClicked);
    tablet.screenChanged.connect(onScreenChanged);

    Script.scriptEnding.connect(function () {
        button.clicked.disconnect(onClicked);
        tablet.removeButton(button);
        tablet.screenChanged.disconnect(onScreenChanged);
    });

}()); // END LOCAL_SCOPE
