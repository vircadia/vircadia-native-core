"use strict";

//
//  debugTransition.js
//  developer/utilities/render
//
//  Olivier Prat, created on 30/04/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var TABLET_BUTTON_NAME = "Transition";
    var QMLAPP_URL = Script.resolvePath("./transition.qml");
    var ICON_URL = Script.resolvePath("../../../system/assets/images/transition-i.svg");
    var ACTIVE_ICON_URL = Script.resolvePath("../../../system/assets/images/transition-a.svg");

   
    var onScreen = false;

    function onClicked() {
        if (onScreen) {
            tablet.gotoHomeScreen();
        } else {
            tablet.loadQMLSource(QMLAPP_URL);
        }
    }

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        text: TABLET_BUTTON_NAME,
        icon: ICON_URL,
        activeIcon: ACTIVE_ICON_URL
    });

    var hasEventBridge = false;

    function wireEventBridge(on) {
        if (!tablet) {
            print("Warning in wireEventBridge(): 'tablet' undefined!");
            return;
        }
        if (on) {
            if (!hasEventBridge) {
                tablet.fromQml.connect(fromQml);
                hasEventBridge = true;
            }
        } else {
            if (hasEventBridge) {
                tablet.fromQml.disconnect(fromQml);
                hasEventBridge = false;
            }
        }
    }

    function onScreenChanged(type, url) {
        if (url === QMLAPP_URL) {
            onScreen = true;
        } else { 
            onScreen = false;
        }
        
        button.editProperties({isActive: onScreen});
        wireEventBridge(onScreen);
    }

    function fromQml(message) {
    }
        
    button.clicked.connect(onClicked);
    tablet.screenChanged.connect(onScreenChanged);

    Script.scriptEnding.connect(function () {
        if (onScreen) {
            tablet.gotoHomeScreen();
        }
        button.clicked.disconnect(onClicked);
        tablet.screenChanged.disconnect(onScreenChanged);
        tablet.removeButton(button);
    });

}()); 