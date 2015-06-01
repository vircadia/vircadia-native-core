
var controlHeld = false;
var shiftHeld = false;


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
                Entities.setVoxelSphere(id, intersection.intersection, 1.0, 0);
	    } else if (shiftHeld) {
		Entities.setAllVoxels(id, 255);
            } else {
                Entities.setVoxelSphere(id, intersection.intersection, 1.0, 255);
            }
        }
    }
}


function keyPressEvent(event) {
    if (event.text == "CONTROL") {
        controlHeld = true;
    }
    if (event.text == "SHIFT") {
        shiftHeld = true;
    }
}


function keyReleaseEvent(event) {
    if (event.text == "CONTROL") {
        controlHeld = false;
    }
    if (event.text == "SHIFT") {
        shiftHeld = false;
    }
}


Controller.mousePressEvent.connect(mousePressEvent);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
