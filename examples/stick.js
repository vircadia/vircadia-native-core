var stickID = null;
// sometimes if this is run immediately the stick doesn't get created?  use a timer.
Script.setTimeout(function() {
    var stickID = Entities.addEntity({
        type: "Model",
        modelURL: "https://hifi-public.s3.amazonaws.com/eric/models/stick.fbx",
        compoundShapeURL: "https://hifi-public.s3.amazonaws.com/eric/models/stick.obj",
        dimensions: {x: .11, y: .11, z: .59},
        position: MyAvatar.getRightPalmPosition(),
        rotation: MyAvatar.orientation,
        damping: .1,
        collisionsWillMove: true
    });
    Entities.addAction("hold", stickID, {relativePosition: {x: 0.0, y: 0.0, z: -0.9}, timeScale: 0.15});
}, 3000);

function cleanup() { Entities.deleteEntity(stickID); }
Script.scriptEnding.connect(cleanup);
