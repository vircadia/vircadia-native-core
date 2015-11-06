var modelURL = "https://s3.amazonaws.com/hifi-public/eric/models/arrow.fbx";
var scriptURL = Script.resolvePath('avatarMover.js');
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));

var avatarMover = Entities.addEntity({
    type: "Model",
    modelURL: modelURL,
    position: center,
    userData: JSON.stringify({range: 5}),
    script: scriptURL
});


function cleanup() {
    Entities.deleteEntity(avatarMover);
}

Script.scriptEnding.connect(cleanup);