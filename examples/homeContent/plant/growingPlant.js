var orientation = Camera.getOrientation();
orientation = Quat.safeEulerAngles(orientation);
orientation.x = 0;
orientation = Quat.fromVec3Degrees(orientation);
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(orientation)));

var pot;
initializePlant();

function initializePlant() {
  var MODEL_URL = "file:///C:/Users/Eric/Desktop/pot.fbx";
  pot = Entities.addEntity({
    type: "Model",
    modelURL: MODEL_URL,
    position: center
  });

  Script.setTimeout(function() {
    var naturalDimensions = Entities.getEntityProperties(pot, "naturalDimensions").naturalDimensions;
    Entities.editEntity(pot, {dimensions: naturalDimensions});
  }, 2000);

}

function cleanup() {
  Entities.deleteEntity(pot);
}


Script.scriptEnding.connect(cleanup);