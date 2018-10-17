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


 
    var Page = function(title, qmlurl, width, height) {
       
        this.title = title;
        this.qml = Script.resolvePath(qmlurl);
        this.width = width;
        this.height = height;

        this.window = null;
    };

    Page.prototype.createView = function() {
        if (this.window == null) {
            var window = Desktop.createWindow(this.qml, {
                title: this.title,
                presentationMode: Desktop.PresentationMode.NATIVE,
                size: {x: this.width, y: this.height}
            });
            this.window = window
            this.window.closed.connect(this.killView);
        }  
    };

    Page.prototype.killView = function() {
        if (this.window !==  undefined) { 
            this.window.closed.disconnect(this.killView);
            this.window.close()
            this.window = undefined
        }
    };

    var pages = []
    pages.push_back(new Page('Render Engine', 'engineInspector.qml', 300, 400))

 

    var engineInspectorView = null 
    function openEngineInspectorView() {
        
   /*        if (engineInspectorView == null) {
            var qml = Script.resolvePath('engineInspector.qml');
            var window = Desktop.createWindow(qml, {
                title: 'Render Engine',
                presentationMode: Desktop.PresentationMode.NATIVE,
                size: {x: 300, y: 400}
            });
            engineInspectorView = window
            window.closed.connect(killEngineInspectorView);
        }
    }

    function killEngineInspectorView() {
        if (engineInspectorView !==  undefined) { 
            engineInspectorView.closed.disconnect(killEngineInspectorView);
            engineInspectorView.close()
            engineInspectorView = undefined
        }
    }
*/
    var cullInspectorView = null 
    function openCullInspectorView() {
        if (cullInspectorView == null) {
            var qml = Script.resolvePath('culling.qml');
            var window = Desktop.createWindow(qml, {
                title: 'Cull Inspector',
                presentationMode: Desktop.PresentationMode.NATIVE,
                size: {x: 400, y: 600}
            });
            cullInspectorView = window
            window.closed.connect(killCullInspectorView);
        }
    }

    function killCullInspectorView() {
        if (cullInspectorView !==  undefined) { 
            cullInspectorView.closed.disconnect(killCullInspectorView);
            cullInspectorView.close()
            cullInspectorView = undefined
        }
    }

    var engineLODView = null
    function openEngineLODView() {
        if (engineLODView == null) {
            engineLODView = Desktop.createWindow(Script.resolvePath('lod.qml'), {
                title: 'Render LOD',
                flags: Desktop.ALWAYS_ON_TOP,
                presentationMode: Desktop.PresentationMode.NATIVE,
                size: {x: 300, y: 500},
            });
            engineLODView.closed.connect(killEngineLODView);
        }
    }
     function killEngineLODView() {
        if (engineLODView !==  undefined) { 
            engineLODView.closed.disconnect(killEngineLODView);
            engineLODView.close()
            engineLODView = undefined
        }
    }
    
    

    function fromQml(message) {
        switch (message.method) {
        case "openEngineView":
            openEngineInspectorView();
            break;
        case "openCullInspectorView":
            openCullInspectorView();
            break;
        case "openEngineLODView":
            openEngineLODView();
            break;
        }               
    }

    Script.scriptEnding.connect(function () {
        if (onLuciScreen) {
            tablet.gotoHomeScreen();
        }
        button.clicked.disconnect(onClicked);
        tablet.screenChanged.disconnect(onScreenChanged);
        tablet.removeButton(button);

        killEngineInspectorView();
        killCullInspectorView();
        killEngineLODWindow();
    });
}()); 