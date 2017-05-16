function inFrontOf(distance, position, orientation) {
    return Vec3.sum(position || MyAvatar.position,
                    Vec3.multiply(distance, Quat.getForward(orientation || MyAvatar.orientation)));
}
var aroundY = Quat.fromPitchYawRollDegrees(0, 180, 0);
function flip(rotation) { return Quat.multiply(rotation, aroundY); }
// Specifying the following userData makes the camera near-grabbable in HMD.
// We really want it to also be far-grabbable and mouse-grabbable,
// but that requires dynamic:1, but alas that causes the camera to drift
// when let go. Maybe we'll restore the old "zero velocity on release" code
// to (handController)grab.js, and also make the camera collisionless?
var isDynamic = false;

var viewFinderOverlay, camera = Entities.addEntity({
    type: 'Box',
    dimensions: {x: 0.4, y: 0.2, z: 0.4},
    userData: '{"grabbableKey":{"grabbable":true}}',
    dynamic: isDynamic ? 1 : 0,
    color: {red: 255, green: 0, blue: 0},
    name: 'SpectatorCamera'
}); // FIXME: avatar entity so that you don't need rez rights.

var config = Render.getConfig("SelfieFrame");
var config2 = Render.getConfig("BeginSelfie");
function updateRenderFromCamera() {
    var cameraData = Entities.getEntityProperties(camera, ['position', 'rotation']);
    // FIXME: don't muck with config if properties haven't changed.
    config2.position = cameraData.position;
    config2.orientation = cameraData.rotation;
    // BUG: image3d overlays don't retain their locations properly when parented to a dynamic object
    if (isDynamic) {
        Overlays.editOverlay(viewFinderOverlay, { orientation: flip(cameraData.rotation) });
    }
}
Script.setTimeout(function () { // delay for a bit in case this script is running at startup
    // Set the special texture size based on the window in which it will eventually be displayed.
    var size = Controller.getViewportDimensions();
    config.resetSize(size.x, size.y);
    Script.update.connect(updateRenderFromCamera);
    config.enabled = config2.enabled = true;
    Script.setTimeout(function () { // FIXME: do we need this delay? why?
        var cameraRotation = MyAvatar.orientation, cameraPosition = inFrontOf(2);
        // Put the camera in front of me so that I can find it.
        Entities.editEntity(camera, {
            position: cameraPosition,
            rotation: cameraRotation
        });
        // Put an image3d overlay on the near face, as a viewFinder.
        viewFinderOverlay = Overlays.addOverlay("image3d", {
            url: "http://selfieFrame",
            //url: "http://1.bp.blogspot.com/-1GABEq__054/T03B00j_OII/AAAAAAAAAa8/jo55LcvEPHI/s1600/Winning.jpg",
            parentID: camera,
            alpha: 1,
            position: inFrontOf(-0.25, cameraPosition, cameraRotation),
            // FIXME: We shouldn't need the flip and the negative scale.
            // e.g., This isn't necessary using an ordinary .jpg with lettering, above.
            // Must be something about the view frustum projection matrix?
            // But don't go changing that in (c++ code) without getting all the way to a desktop display!
            orientation: flip(cameraRotation),
            scale: -0.35,
        });
    }, 500);
}, 1000);

Script.scriptEnding.connect(function () {
    config.enabled = config2.enabled = false;
    Script.update.disconnect(updateRenderFromCamera);
    Overlays.deleteOverlay(viewFinderOverlay);
    Entities.deleteEntity(camera);
})
