//
//  debugAmbientOcclusionPass.js
//  developer/utilities/render
//
//  Olivier Prat, created on 11/04/2018.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    "use strict";

    var TABLET_BUTTON_NAME = "SSAO";
    var QMLAPP_URL = Script.resolvePath("./ambientOcclusionPass.qml");
    var ICON_URL = Script.resolvePath("../../../system/assets/images/ssao-i.svg");
    var ACTIVE_ICON_URL = Script.resolvePath("../../../system/assets/images/ssao-a.svg");

    Script.include([
        Script.resolvePath("../../../system/libraries/stringHelpers.js"),
    ]);

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
  

    /*
    var moveDebugCursor = false;
    Controller.mousePressEvent.connect(function (e) {
        if (e.isMiddleButton) {
            moveDebugCursor = true;
            setDebugCursor(e.x, e.y);
        }
    });
    Controller.mouseReleaseEvent.connect(function() { moveDebugCursor = false; });
    Controller.mouseMoveEvent.connect(function (e) { if (moveDebugCursor) setDebugCursor(e.x, e.y); });


    function setDebugCursor(x, y) {
        nx = (x / Window.innerWidth);
        ny = 1.0 - ((y) / (Window.innerHeight - 32));

        Render.getConfig("RenderMainView").getConfig("DebugAmbientOcclusion").debugCursorTexcoord = { x: nx, y: ny };
    }
    */

    function cleanup() {
    }
    Script.scriptEnding.connect(cleanup);
}()); 

