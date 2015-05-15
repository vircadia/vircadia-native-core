
var lineEntityID = null;
var lineIsRezzed = false;


function nearLinePoint(targetPosition) {
    var handPosition = MyAvatar.getRightPalmPosition();
    var along = Vec3.subtract(targetPosition, handPosition);
    along = Vec3.normalize(along);
    along = Vec3.multiply(along, 0.4);
    return Vec3.sum(handPosition, along);
}


function removeLine() {
    if (lineIsRezzed) {
        Entities.deleteEntity(lineEntityID);
        lineEntityID = null;
        lineIsRezzed = false;
    }
}


function createOrUpdateLine(event) {
    var pickRay = Camera.computePickRay(event.x, event.y);
    var intersection = Entities.findRayIntersection(pickRay, true); // accurate picking

    if (lineIsRezzed) {
        Entities.editEntity(lineEntityID, {
            position: nearLinePoint(intersection.intersection),
            dimensions: Vec3.subtract(intersection.intersection, nearLinePoint(intersection.intersection)),
            lifetime: 30 // renew lifetime
    });

    } else {
        lineIsRezzed = true;
        lineEntityID = Entities.addEntity({
            type: "Line",
            position: nearLinePoint(intersection.intersection),
            dimensions: Vec3.subtract(intersection.intersection, nearLinePoint(intersection.intersection)),
            color: { red: 255, green: 255, blue: 255 },
            lifetime: 30 // if someone crashes while pointing, don't leave the line there forever.
        });
    }
}


function mousePressEvent(event) {
    if (!event.isLeftButton) {
        return;
    }
    if (lineIsRezzed) {
        return;
    }
    createOrUpdateLine(event);
    if (lineIsRezzed) {
        Controller.mouseMoveEvent.connect(mouseMoveEvent);
    }
 }


function mouseMoveEvent(event) {
    createOrUpdateLine(event);
}


function mouseReleaseEvent() {
    if (lineIsRezzed) {
        Controller.mouseMoveEvent.disconnect(mouseMoveEvent);
    }
    removeLine();
}


Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
