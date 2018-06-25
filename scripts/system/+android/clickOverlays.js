"use strict";
//
//  clickOverlays.js
//  scripts/system/+android
//
//  Created by Gabriel Calero & Cristian Duarte on Jun 22, 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE

var logEnabled = false;
var touchOverlayID;

function printd(str) {
    if (logEnabled)
        print("[clickOverlays.js] " + str);
}

function touchBegin(event) {
    var rayIntersection = Overlays.findRayIntersection(Camera.computePickRay(event.x, event.y));
    if (rayIntersection && rayIntersection.intersects && rayIntersection.overlayID &&
        Overlays.getOverlayType(rayIntersection.overlayID) == "web3d") {
        touchOverlayID = rayIntersection.overlayID;
    } else {
        touchOverlayID = null;        
    }
}

function touchEnd(event) {
    var rayIntersection = Overlays.findRayIntersection(Camera.computePickRay(event.x, event.y));
    if (rayIntersection && rayIntersection.intersects && rayIntersection.overlayID &&
        touchOverlayID == rayIntersection.overlayID) {
        var propertiesToGet = {};
        propertiesToGet[rayIntersection.overlayID] = ['url'];
        var properties = Overlays.getOverlaysProperties(propertiesToGet);
        if (properties[rayIntersection.overlayID].url) {
            Window.openUrl(properties[rayIntersection.overlayID].url);
        }
        var overlayObj = Overlays.getOverlayObject(rayIntersection.overlayID);
        Overlays.sendMousePressOnOverlay(rayIntersection.overlayID, {
          type: "press",
          id: 0,
          pos2D: event
        });
    }
    touchOverlayID = null;
}

function ending() {
    Controller.touchBeginEvent.disconnect(touchBegin);
    Controller.touchEndEvent.disconnect(touchEnd);
}

function init() {
    Controller.touchBeginEvent.connect(touchBegin);
    Controller.touchEndEvent.connect(touchEnd);

    Script.scriptEnding.connect(function () {
        ending();
    });

}

module.exports = {
    init: init,
    ending: ending
}

}()); // END LOCAL_SCOPE
