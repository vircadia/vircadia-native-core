var modelURL= "https://s3.amazonaws.com/hifi-public/eric/models/helicopter.fbx";
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));

var helicopter = Entities.addEntity({
    type: "Model",
    modelURL: modelURL,
    position: center
});



function cleanup() {
    Entities.deleteEntity(helicopter);
}

Script.scriptEnding.connect(cleanup);