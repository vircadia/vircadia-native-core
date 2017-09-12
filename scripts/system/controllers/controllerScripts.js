"use strict";

//  controllerScripts.js
//
//  Created by David Rowe on 15 Mar 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var CONTOLLER_SCRIPTS = [
    "squeezeHands.js",
    "controllerDisplayManager.js",
    "handControllerPointer.js",
    "grab.js",
    "toggleAdvancedMovementForHandControllers.js",
    "controllerDispatcher.js",
    "controllerModules/nearParentGrabEntity.js",
    "controllerModules/nearParentGrabOverlay.js",
    "controllerModules/nearActionGrabEntity.js",
    "controllerModules/farActionGrabEntity.js",
    "controllerModules/tabletStylusInput.js",
    "controllerModules/equipEntity.js",
    "controllerModules/nearTrigger.js",
    "controllerModules/overlayLaserInput.js",
    "controllerModules/webEntityLaserInput.js",
    "controllerModules/inEditMode.js",
    "controllerModules/disableOtherModule.js",
    "controllerModules/farTrigger.js",
    "controllerModules/teleport.js",
    "controllerModules/scaleAvatar.js"
];

var DEBUG_MENU_ITEM = "Debug defaultScripts.js";


function runDefaultsTogether() {
    for (var j in CONTOLLER_SCRIPTS) {
        Script.include(CONTOLLER_SCRIPTS[j]);
    }
}

function runDefaultsSeparately() {
    for (var i in CONTOLLER_SCRIPTS) {
        Script.load(CONTOLLER_SCRIPTS[i]);
    }
}

if (Menu.isOptionChecked(DEBUG_MENU_ITEM)) {
    runDefaultsSeparately();
} else {
    runDefaultsTogether();
}
