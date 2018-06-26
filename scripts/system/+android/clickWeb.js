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
var touchEntityID;

function printd(str) {
    if (logEnabled)
        print("[clickOverlays.js] " + str);
}


function findOverlayIDRayIntersection(pickRay) {
    // Check 3D overlays and entities. Argument is an object with origin and direction.
    var rayIntersection = Overlays.findRayIntersection(pickRay);
    if (rayIntersection && rayIntersection.intersects && rayIntersection.overlayID &&
        Overlays.getOverlayType(rayIntersection.overlayID) == "web3d") {
        return rayIntersection.overlayID;
    }
    return false;
}


function findEntityIDRayIntersection(pickRay) {
    // Check 3D overlays and entities. Argument is an object with origin and direction.
    var rayIntersection = Entities.findRayIntersection(pickRay, true);
    if (rayIntersection.entityID) {
        var properties = Entities.getEntityProperties(rayIntersection.entityID, ["type", "sourceUrl"]);
        if (properties.type && properties.type == "Web" && properties.sourceUrl) {
            return rayIntersection.entityID;
        }
    }
    return false;
}

function touchBegin(event) {
    var overlayID = findOverlayIDRayIntersection(Camera.computePickRay(event.x, event.y));
    if (overlayID) {
        touchOverlayID = overlayID;
        touchEntityID = null;
        return;
    }
    var entityID = findEntityIDRayIntersection(Camera.computePickRay(event.x, event.y));
    if (entityID) {
        touchEntityID = entityID;
        touchOverlayID = null;
        return;
    }
}

function touchEnd(event) {
    var overlayID = findOverlayIDRayIntersection(Camera.computePickRay(event.x, event.y));
    if (touchOverlayID && overlayID == touchOverlayID) {
        var propertiesToGet = {};
        propertiesToGet[overlayID] = ['url'];
        var properties = Overlays.getOverlaysProperties(propertiesToGet);
        if (properties[overlayID].url) {
            Window.openUrl(properties[overlayID].url);
        }
    }

    var entityID = findEntityIDRayIntersection(Camera.computePickRay(event.x, event.y));
    if (touchEntityID && entityID == touchEntityID) {
        var properties = Entities.getEntityProperties(entityID, ["sourceUrl"]);
        if (properties.sourceUrl) {
            Window.openUrl(properties.sourceUrl);
        }
    }

    touchOverlayID = null;
    touchEntityID = null;
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
