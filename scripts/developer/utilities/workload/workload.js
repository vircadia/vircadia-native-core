"use strict";

//
//  Workload.js
//  tablet-workload-engine app
//
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var TABLET_BUTTON_NAME = "Workload";
    var QMLAPP_URL = Script.resolvePath("./workloadInspector.qml");
    var ICON_URL = Script.resolvePath("../../../system/assets/images/luci-i.svg");
    var ACTIVE_ICON_URL = Script.resolvePath("../../../system/assets/images/luci-a.svg");

   
    var onAppScreen = false;

    function onClicked() {
        if (onAppScreen) {
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
            onAppScreen = true;
        } else { 
            onAppScreen = false;
        }
        
        button.editProperties({isActive: onAppScreen});
        wireEventBridge(onAppScreen);
    }

    function fromQml(message) {
    }
        
    button.clicked.connect(onClicked);
    tablet.screenChanged.connect(onScreenChanged);

    Script.scriptEnding.connect(function () {
        if (onAppScreen) {
            tablet.gotoHomeScreen();
        }
        button.clicked.disconnect(onClicked);
        tablet.screenChanged.disconnect(onScreenChanged);
        tablet.removeButton(button);
    });


    Script.include("./test_physics_scene.js")

    function fromQml(message) {
        switch (message.method) {
        case "createScene":
            createScene();
            updateGridInQML()
            break;
        case "clearScene":
            clearScene();
            updateGridInQML()
            break;
        case "changeSize":
            changeSize(message.params.count);
            updateGridInQML()
            break;
        case "changeResolution":
            changeResolution(message.params.count);
            updateGridInQML()
            break;
        case "bumpUpFloor":
            bumpUpFloor();
            break;
        }
        
    }
    function updateGridInQML() {
        sendToQml({method: "gridSize", params: { v: getSize() }})
        sendToQml({method: "objectCount", params: { v: getNumObjects() }})
    }

    function sendToQml(message) {
        tablet.sendToQml(message);
    }

    updateGridInQML()
}()); 