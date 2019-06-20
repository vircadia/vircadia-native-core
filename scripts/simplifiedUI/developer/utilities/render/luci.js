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
    var AppUi = Script.require('appUi');
    
    var MaterialInspector = Script.require('./materialInspector.js');
    var Page = Script.require('./luci/Page.js');

    var moveDebugCursor = false;
    var onMousePressEvent = function (e) {
        if (e.isMiddleButton) {
            moveDebugCursor = true;
            setDebugCursor(e.x, e.y);
        }
    };
    Controller.mousePressEvent.connect(onMousePressEvent);

    var onMouseReleaseEvent = function () {
        moveDebugCursor = false;
    };
    Controller.mouseReleaseEvent.connect(onMouseReleaseEvent);

    var onMouseMoveEvent = function (e) {
        if (moveDebugCursor) {
            setDebugCursor(e.x, e.y);
        }
    };
    Controller.mouseMoveEvent.connect(onMouseMoveEvent);

    function setDebugCursor(x, y) {
        var nx = 2.0 * (x / Window.innerWidth) - 1.0;
        var ny = 1.0 - 2.0 * ((y) / (Window.innerHeight));

        Render.getConfig("RenderMainView").getConfig("DebugDeferredBuffer").size = { x: nx, y: ny, z: 1.0, w: 1.0 };
    }


    var pages = new Pages();

    pages.addPage('openEngineLODView', 'Render LOD', '../lod.qml', 300, 400);
    pages.addPage('openCullInspectorView', 'Cull Inspector', '../luci/Culling.qml', 300, 400);
    pages.addPage('openMaterialInspectorView', 'Material Inspector', '../materialInspector.qml', 300, 400, MaterialInspector.setWindow);

    function fromQml(message) {
        if (pages.open(message.method)) {
            return;
        }    
    }

    var ui;
    function startup() {
        ui = new AppUi({
            buttonName: "LUCI",
            home: Script.resolvePath("deferredLighting.qml"),
            additionalAppScreens : Script.resolvePath("engineInspector.qml"),
            onMessage: fromQml,
            normalButton: Script.resolvePath("../../../system/assets/images/luci-i.svg"),
            activeButton: Script.resolvePath("../../../system/assets/images/luci-a.svg")
        });
    }
    startup();
    Script.scriptEnding.connect(function () {
        Controller.mousePressEvent.disconnect(onMousePressEvent);
        Controller.mouseReleaseEvent.disconnect(onMouseReleaseEvent);
        Controller.mouseMoveEvent.disconnect(onMouseMoveEvent);
        pages.clear();
    });
}()); 
