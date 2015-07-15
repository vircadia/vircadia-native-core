var controlHeld = false;
var shiftHeld = false;


function attemptVoxelChange(intersection) {
    var ids = Entities.findEntities(intersection.intersection, 10);
    var success = false;
    for (var i = 0; i < ids.length; i++) {
        var id = ids[i];
        if (controlHeld) {
            // hold control to erase a sphere
            if (Entities.setVoxelSphere(id, intersection.intersection, 1.0, 0)) {
                success = true;
            }
        } else if (shiftHeld) {
            // hold shift to set all voxels to 255
            if (Entities.setAllVoxels(id, 255)) {
                success = true;
            }
        } else {
            // no modifier key means to add a sphere
            if (Entities.setVoxelSphere(id, intersection.intersection, 1.0, 255)) {
                success = true;
            }
        }
    }
    return success;
}


function mousePressEvent(event) {
    if (!event.isLeftButton) {
        return;
    }

    var pickRay = Camera.computePickRay(event.x, event.y);
    var intersection = Entities.findRayIntersection(pickRay, true); // accurate picking

    // we've used a picking ray to decide where to add the new sphere of voxels.  If we pick nothing
    // or if we pick a non-PolyVox entity, we fall through to the next picking attempt.
    if (intersection.intersects) {
        if (attemptVoxelChange(intersection)) {
            return;
        }
    }

    // if the PolyVox entity is empty, we can't pick against its voxel.  try picking against its
    // bounding box, instead.
    intersection = Entities.findRayIntersection(pickRay, false); // bounding box picking
    if (intersection.intersects) {
        attemptVoxelChange(intersection);
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
