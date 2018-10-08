"use strict";

//
//  Luci.js
//  tablet-engine app
//
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var TABLET_BUTTON_NAME = "LUCI";
    var QMLAPP_URL = Script.resolvePath("./deferredLighting.qml");
    var ICON_URL = Script.resolvePath("../../../system/assets/images/luci-i.svg");
    var ACTIVE_ICON_URL = Script.resolvePath("../../../system/assets/images/luci-a.svg");

   
    var onLuciScreen = false;

    function onClicked() {
        if (onLuciScreen) {
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
            onLuciScreen = true;
        } else { 
            onLuciScreen = false;
        }
        
        button.editProperties({isActive: onLuciScreen});
        wireEventBridge(onLuciScreen);
    }
        
    button.clicked.connect(onClicked);
    tablet.screenChanged.connect(onScreenChanged);

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
        nx = 2.0 * (x / Window.innerWidth) - 1.0;
        ny = 1.0 - 2.0 * ((y) / (Window.innerHeight));

         Render.getConfig("RenderMainView").getConfig("DebugDeferredBuffer").size = { x: nx, y: ny, z: 1.0, w: 1.0 };
    }


    function fromQml(message) {
        switch (message.method) {
        case "openEngineView":
            openEngineTaskView();
            break;
        }            
    }


    var engineInspectorView = null 
    function openEngineTaskView() {
        if (engineInspectorView == null) {
            var qml = Script.resolvePath('engineInspector.qml');
            var window = new OverlayWindow({
                title: 'Render Engine',
                source: qml,
                width: 300, 
                height: 400
            });
            window.setPosition(200, 50);
            engineInspectorView = window
            window.closed.connect(function() { engineInspectorView = null; });
        } else {
            engineInspectorView.setPosition(200, 50);          
        }
    }


    
    Script.scriptEnding.connect(function () {
        if (onLuciScreen) {
            tablet.gotoHomeScreen();
        }
        button.clicked.disconnect(onClicked);
        tablet.screenChanged.disconnect(onScreenChanged);
        tablet.removeButton(button);

        if (engineInspectorView !== null) {
            engineInspectorView.close()
        }
    });
}()); 