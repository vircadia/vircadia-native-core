"use strict";

//
//  skyboxchanger.js
//
//  Created by Cain Kilgore on 9th August 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var TABLET_BUTTON_NAME = "SKYBOX";

    var ICONS = {
        icon: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/skyboxedit-i.svg",
        activeIcon: "http://mpassets.highfidelity.com/05904016-8f7d-4dfc-88e1-2bf9ba3fac20-v1/skyboxedit-i.svg"
    };

    var onSkyboxChangerScreen = false;

    function onClicked() {
        if (onSkyboxChangerScreen) {
            tablet.gotoHomeScreen();
        } else {
            tablet.loadQMLSource("../SkyboxChanger.qml");
        }
    }

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        icon: ICONS.icon,
        activeIcon: ICONS.activeIcon,
        text: TABLET_BUTTON_NAME,
        sortOrder: 1
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
        if (url === "../SkyboxChanger.qml") {
            onSkyboxChangerScreen = true;
        } else { 
            onSkyboxChangerScreen = false;
        }
        
        button.editProperties({isActive: onSkyboxChangerScreen});
        wireEventBridge(onSkyboxChangerScreen);
    }

    function fromQml(message) {
        switch (message.method) {
            case 'changeSkybox': // changeSkybox Code
                var standingZone;
                if (!Entities.canRez()) {
                    Window.alert("You need to have rez permissions to change the Skybox.");
                    break;
                }
                
                var nearbyEntities = Entities.findEntities(MyAvatar.position, 5);
                for (var i = 0; i < nearbyEntities.length; i++) {
                    if (Entities.getEntityProperties(nearbyEntities[i]).type === "Zone") {
                        standingZone = nearbyEntities[i];
                    }
                }
                
                if (Entities.getEntityProperties(standingZone).locked) {
                    Window.alert("This zone is currently locked; the Skybox can't be changed.");
                    break;
                }
                
                var newSkybox = {
                    skybox: {
                        url: message.url
                    },
                    keyLight: {
                        ambientURL: message.url
                    }
                };
                
                Entities.editEntity(standingZone, newSkybox);
                break;
            default:
                print('Unrecognized message from QML: ' + JSON.stringify(message));
        }
    }
        
    button.clicked.connect(onClicked);
    tablet.screenChanged.connect(onScreenChanged);

    Script.scriptEnding.connect(function () {
        if (onSkyboxChangerScreen) {
            tablet.gotoHomeScreen();
        }
        button.clicked.disconnect(onClicked);
        tablet.screenChanged.disconnect(onScreenChanged);
        tablet.removeButton(button);
    });
}());