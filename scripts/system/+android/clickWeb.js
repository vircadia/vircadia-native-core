"use strict";
//
//  clickWeb.js
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
        print("[clickWeb.js] " + str);
}

function intersectsWebOverlay(intersection) {
    return intersection && intersection.intersects && intersection.overlayID && 
            Overlays.getOverlayType(intersection.overlayID) == "web3d";
}

function intersectsWebEntity(intersection) {
    if (intersection && intersection.intersects && intersection.entityID) {
        var properties = Entities.getEntityProperties(intersection.entityID, ["type", "sourceUrl"]);
        return properties.type && properties.type == "Web" && properties.sourceUrl;
    }
    return false;
}

function findRayIntersection(pickRay) {
    // Check 3D overlays and entities. Argument is an object with origin and direction.
    var overlayRayIntersection = Overlays.findRayIntersection(pickRay);
    var entityRayIntersection = Entities.findRayIntersection(pickRay, true);
    var isOverlayInters = intersectsWebOverlay(overlayRayIntersection);
    var isEntityInters = intersectsWebEntity(entityRayIntersection);
    if (isOverlayInters && 
        (!isEntityInters || 
          overlayRayIntersection.distance < entityRayIntersection.distance)) {
        return { type: 'overlay', obj: overlayRayIntersection };
    } else if (isEntityInters &&
        (!isOverlayInters ||
          entityRayIntersection.distance < overlayRayIntersection.distance)) {
        return { type: 'entity', obj: entityRayIntersection };
    }
    return false;
}

function touchBegin(event) {
    var intersection = findRayIntersection(Camera.computePickRay(event.x, event.y));
    if (intersection && intersection.type == 'overlay') {
        touchOverlayID = intersection.obj.overlayID;
        touchEntityID = null;
    } else if (intersection && intersection.type == 'entity') {
        touchEntityID = intersection.obj.entityID;
        touchOverlayID = null;
    }
}

function touchEnd(event) {
    var intersection = findRayIntersection(Camera.computePickRay(event.x, event.y));
    if (intersection && intersection.type == 'overlay' && touchOverlayID == intersection.obj.overlayID) {
        var propertiesToGet = {};
        propertiesToGet[overlayID] = ['url'];
        var properties = Overlays.getOverlaysProperties(propertiesToGet);
        if (properties[overlayID].url) {
            Window.openUrl(properties[overlayID].url);
        }
    } else if (intersection && intersection.type == 'entity' && touchEntityID == intersection.obj.entityID) {
        var properties = Entities.getEntityProperties(touchEntityID, ["sourceUrl"]);
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

init();

}()); // END LOCAL_SCOPE
