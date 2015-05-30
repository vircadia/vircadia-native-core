
var controlHeld = false;


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
            if (controlHeld) {
                Entities.setVoxelSphere(id, intersection.intersection, 0.5, 0);
            } else {
                Entities.setVoxelSphere(id, intersection.intersection, 0.5, 255);
            }
        }
    }
}


function keyPressEvent(event) {
    if (event.text == "CONTROL") {
        controlHeld = true;
    }
}


function keyReleaseEvent(event) {
    if (event.text == "CONTROL") {
        controlHeld = false;
    }
}


Controller.mousePressEvent.connect(mousePressEvent);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
