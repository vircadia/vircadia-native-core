
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
    var props = Entities.getEntityProperties(intersection.entityID);

    if (intersection.intersects) {
        var dim = Vec3.subtract(intersection.intersection, nearLinePoint(intersection.intersection));
        if (lineIsRezzed) {
            Entities.editEntity(lineEntityID, {
                position: nearLinePoint(intersection.intersection),
                dimensions: dim,
                lifetime: 60 + props.lifespan // renew lifetime
            });
        } else {
            lineIsRezzed = true;
            lineEntityID = Entities.addEntity({
                type: "Line",
                position: nearLinePoint(intersection.intersection),
                dimensions: dim,
                color: { red: 255, green: 255, blue: 255 },
                lifetime: 60 // if someone crashes while pointing, don't leave the line there forever.
            });
        }
    } else {
        removeLine();
    }
}


function mousePressEvent(event) {
    if (!event.isLeftButton) {
        return;
    }
    Controller.mouseMoveEvent.connect(mouseMoveEvent);
    createOrUpdateLine(event);
 }


function mouseMoveEvent(event) {
    createOrUpdateLine(event);
}


function mouseReleaseEvent(event) {
    if (!event.isLeftButton) {
        return;
    }
    Controller.mouseMoveEvent.disconnect(mouseMoveEvent);
    removeLine();
}


Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
