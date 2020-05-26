"use strict";

//  controllerScripts.js
//
//  Created by David Rowe on 15 Mar 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Script, Menu */

var CONTOLLER_SCRIPTS = [
    "squeezeHands.js",
    "controllerDisplayManager.js",
    "grab.js",
    //"toggleAdvancedMovementForHandControllers.js",
    "handTouch.js",
    "controllerDispatcher.js",
    "controllerModules/nearParentGrabOverlay.js",
    "controllerModules/stylusInput.js",
    "controllerModules/equipEntity.js",
    "controllerModules/nearTrigger.js",
    "controllerModules/webSurfaceLaserInput.js",
    "controllerModules/inEditMode.js",
    "controllerModules/inVREditMode.js",
    "controllerModules/disableOtherModule.js",
    "controllerModules/farTrigger.js",
    "controllerModules/teleport.js",
    "controllerModules/hudOverlayPointer.js",
    "controllerModules/mouseHMD.js",
    "controllerModules/nearGrabHyperLinkEntity.js",
    "controllerModules/nearTabletHighlight.js",
    "controllerModules/nearGrabEntity.js",
    "controllerModules/farGrabEntity.js",
    "controllerModules/pushToTalk.js",
    "controllerModules/trackedHandWalk.js",
    "controllerModules/trackedHandTablet.js"
];

var DEBUG_MENU_ITEM = "Debug defaultScripts.js";

function runDefaultsTogether() {
    for (var j in CONTOLLER_SCRIPTS) {
        if (CONTOLLER_SCRIPTS.hasOwnProperty(j)) {
            Script.include(CONTOLLER_SCRIPTS[j]);
        }
    }
}

function runDefaultsSeparately() {
    for (var i in CONTOLLER_SCRIPTS) {
        if (CONTOLLER_SCRIPTS.hasOwnProperty(i)) {
            Script.load(CONTOLLER_SCRIPTS[i]);
        }
    }
}

if (Menu.isOptionChecked(DEBUG_MENU_ITEM)) {
    runDefaultsSeparately();
} else {
    runDefaultsTogether();
}
