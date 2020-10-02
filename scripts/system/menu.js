//
//  menu.js
//  scripts/system/
//
//  Created by Dante Ruiz  on 5 Jun 2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* jslint bitwise: true */

/* global Script, HMD, Tablet, Entities */

var HOME_BUTTON_TEXTURE = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/alan/dev/tablet-with-home-button.fbx/tablet-with-home-button.fbm/button-root.png");
// var HOME_BUTTON_TEXTURE = Script.resourcesPath() + "meshes/tablet-with-home-button.fbx/tablet-with-home-button.fbm/button-root.png";

(function() {
    var lastHMDStatus = false;
    var buttonProperties = {
        icon: "icons/tablet-icons/menu-i.svg",
        activeIcon: "icons/tablet-icons/menu-a.svg",
        text: "MENU",
        sortOrder: 3
    };
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = null;

    if (HMD.active) {
        button = tablet.addButton(buttonProperties);
        button.clicked.connect(onClicked);
        lastHMDStatus = true;
    }

    var onMenuScreen = false;

    function onClicked() {
        if (onMenuScreen) {
            // for toolbar-mode: go back to home screen, this will close the window.
            tablet.gotoHomeScreen();
        } else {
            if (HMD.tabletID) {
                Entities.editEntity(HMD.tabletID, { textures: JSON.stringify({ "tex.close": HOME_BUTTON_TEXTURE }) });
            }
            tablet.gotoMenuScreen();
        }
    }

    HMD.displayModeChanged.connect(function(isHMDMode) {
        if (lastHMDStatus !== isHMDMode) {
            if (isHMDMode) {
                button = tablet.addButton(buttonProperties);
                button.clicked.connect(onClicked);
            } else if (button) {
                button.clicked.disconnect(onClicked);
                tablet.removeButton(button);
                button = null;
            }
            lastHMDStatus = isHMDMode;
        }
    });

    function onScreenChanged(type, url) {
        onMenuScreen = type === "Menu";
        if (button) {
            button.editProperties({isActive: onMenuScreen});
        }
    }

    tablet.screenChanged.connect(onScreenChanged);

    Script.scriptEnding.connect(function () {
        if (onMenuScreen) {
            tablet.gotoHomeScreen();
        }

        if (button) {
            button.clicked.disconnect(onClicked);
        }
        tablet.removeButton(button);
        tablet.screenChanged.disconnect(onScreenChanged);
    });
}());
