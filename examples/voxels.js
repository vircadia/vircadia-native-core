
var altHeld = false;


function mousePressEvent(event) {
    if (!event.isLeftButton) {
        return;
    }

    var pickRay = Camera.computePickRay(event.x, event.y);
    var intersection = Entities.findRayIntersection(pickRay, true); // accurate picking
    // var props = Entities.getEntityProperties(intersection.entityID);
    if (intersection.intersects) {
        var ids = Entities.findEntities(intersection.intersection, 10);
        for (var i = 0; i < ids.length; i++) {
            var id = ids[i];
            if (altHeld) {
                Entities.setVoxelSphere(id, intersection.intersection, 2.0, 0);
            } else {
                Entities.setVoxelSphere(id, intersection.intersection, 2.0, 255);
            }
        }
    }
}


function keyPressEvent(event) {
    if (event.text == "ALT") {
        altHeld = true;
    }
}


function keyReleaseEvent(event) {
    if (event.text == "ALT") {
        altHeld = false;
    }
}


Controller.mousePressEvent.connect(mousePressEvent);
