"use strict";

//
//  debugAmbientOcclusionPass.js
//  tablet-sample-app
//
//  Created by Olivier Prat on April 19 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var AppUi = Script.require('appUi');

    var onMousePressEvent = function (e) {
    };
    Controller.mousePressEvent.connect(onMousePressEvent);

    var onMouseReleaseEvent = function () {
    };
    Controller.mouseReleaseEvent.connect(onMouseReleaseEvent);

    var onMouseMoveEvent = function (e) {
    };
    Controller.mouseMoveEvent.connect(onMouseMoveEvent);

    function fromQml(message) {
   
    }

    var ui;
    function startup() {
        ui = new AppUi({
            buttonName: "AO",
            home: Script.resolvePath("ambientOcclusionPass.qml"),
            onMessage: fromQml,
            //normalButton: Script.resolvePath("../../../system/assets/images/ao-i.svg"),
            //activeButton: Script.resolvePath("../../../system/assets/images/ao-a.svg")
        });
    }
    startup();
    Script.scriptEnding.connect(function () {
        Controller.mousePressEvent.disconnect(onMousePressEvent);
        Controller.mouseReleaseEvent.disconnect(onMouseReleaseEvent);
        Controller.mouseMoveEvent.disconnect(onMouseMoveEvent);
        pages.clear();
        // killEngineInspectorView();
        // killCullInspectorView();
        // killEngineLODWindow();
    });
}()); 
