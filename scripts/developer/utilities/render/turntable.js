//
//  turntable.js
//  developer/utilities/render
//
//  Sam Gateau, created on 6/2/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Set up the qml ui
var qml = Script.resolvePath('turntable.qml');
var window = new OverlayWindow({
    title: 'Turntable',
    source: qml,
    width: 320, 
    height: 320
});
window.setPosition(500, 500);
window.closed.connect(function() { Script.stop(); });



function findClickedEntity(event) {
    var pickZones = event.isControl;

    if (pickZones) {
        Entities.setZonesArePickable(true);
    }

    var pickRay = Camera.computePickRay(event.x, event.y);

    var entityResult = Entities.findRayIntersection(pickRay, true); // want precision picking
    var lightResult = lightOverlayManager.findRayIntersection(pickRay);
    lightResult.accurate = true;

    if (pickZones) {
        Entities.setZonesArePickable(false);
    }

    var result;

    if (!entityResult.intersects && !lightResult.intersects) {
        return null;
    } else if (entityResult.intersects && !lightResult.intersects) {
        result = entityResult;
    } else if (!entityResult.intersects && lightResult.intersects) {
        result = lightResult;
    } else {
        if (entityResult.distance < lightResult.distance) {
            result = entityResult;
        } else {
            result = lightResult;
        }
    }

    if (!result.accurate) {
        return null;
    }

    var foundEntity = result.entityID;
    return {
        pickRay: pickRay,
        entityID: foundEntity,
        intersection: result.intersection
    };
}
