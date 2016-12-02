function findSurfaceBelowPosition(pos) {
    var result = Entities.findRayIntersection({
        origin: pos,
        direction: { x: 0, y: -1, z: 0 }
    });
    if (result.intersects) {
        return result.intersection;
    }
    return pos;
}
LAUNCHER_DIMENSIONS = {
    x: 0.5,
    y: 0.5,
    z: 0.5
}

createLauncherAtMyAvatar = function() {
    var launcherPos = Vec3.sum(MyAvatar.position, Vec3.multiply(10, Quat.getFront(MyAvatar.orientation)));
    launcherPos = findSurfaceBelowPosition(launcherPos);
    launcherPos.y += LAUNCHER_DIMENSIONS.y / 2;
    createLaucnher(launcherPos);
}

createLauncher = function(position) {
    Entities.addEntity({
        position: position,
        type: "Model",
        type: "Box",
        //modelURL: 'http://hifi-content.s3.amazonaws.com/alan/dev/EZ-Tube.fbx',
        //compoundShapeURL: 'http://hifi-content.s3.amazonaws.com/alan/dev/EZ-Tube3.obj',
        //shapeType: 'compound'
        dimensions: LAUNCHER_DIMENSIONS,
        script: Script.resolvePath("launch.js")
    });
}
