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
/* globals Tablet, Script, HMD, Controller, Menu */

(function() { // BEGIN LOCAL_SCOPE

    var buttonName = "HELP";
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        icon: "icons/tablet-icons/help-i.svg",
        activeIcon: "icons/tablet-icons/help-a.svg",
        text: buttonName,
        sortOrder: 6
    });

    var enabled = false;
    function onClicked() {
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
        if (enabled) {
             Menu.closeInfoView('InfoView_html/help.html');
        }
        button.clicked.disconnect(onClicked);
        Script.clearInterval(interval);
        if (tablet) {
            tablet.removeButton(button);
        }
    });

}()); // END LOCAL_SCOPE
