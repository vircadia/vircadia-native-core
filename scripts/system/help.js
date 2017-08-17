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
    
    var HOME_BUTTON_TEXTURE = Script.resourcesPath() + "meshes/tablet-with-home-button.fbx/tablet-with-home-button.fbm/button-root.png";
    var buttonName = "HELP";
    var onHelpScreen = false;
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        icon: "icons/tablet-icons/help-i.svg",
        activeIcon: "icons/tablet-icons/help-a.svg",
        text: buttonName,
        sortOrder: 6
    });

    var enabled = false;
    function onClicked() {
        if (onHelpScreen) {
            tablet.gotoHomeScreen();
        } else {
            var tabletEntity = HMD.tabletID;
            if (tabletEntity) {
                Entities.editEntity(tabletEntity, {textures: JSON.stringify({"tex.close" : HOME_BUTTON_TEXTURE})});
            }
            Menu.triggerOption('Help...');
            onHelpScreen = true;
        }
    }

    function onScreenChanged(type, url) {
        onHelpScreen = type === "Web" && url.startsWith("../../../html/tabletHelp.html");
        button.editProperties({ isActive: onHelpScreen });
    }

    button.clicked.connect(onClicked);
    tablet.screenChanged.connect(onScreenChanged);

    var POLL_RATE = 500;
    var interval = Script.setInterval(function () {
        var visible = Menu.isInfoViewVisible('InfoView_html/help.html');
        if (visible !== enabled) {
            enabled = visible;
            button.editProperties({isActive: enabled});
        }
    }, POLL_RATE);

    Script.scriptEnding.connect(function () {
        if (onHelpScreen) {
            tablet.gotoHomeScreen();
        }
        button.clicked.disconnect(onClicked);
        tablet.screenChanged.disconnect(onScreenChanged);
        Script.clearInterval(interval);
        if (tablet) {
            tablet.removeButton(button);
        }
    });

}()); // END LOCAL_SCOPE
