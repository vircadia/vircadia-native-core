
  // Chapter1 : fireworksLaumchButtonSpawner.js

  var orientation = Camera.getOrientation();
  orientation = Quat.safeEulerAngles(orientation);
  orientation.x = 0;
  orientation = Quat.fromVec3Degrees(orientation);
  var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(orientation)));

  var SCRIPT_URL = Script.resolvePath("fireworksLaunchButtonEntityScript.js");
  var MODEL_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/Launch-Button.fbx";
  var launchButton = Entities.addEntity({
    type: "Model",
    name: "hifi-launch-button",
    modelURL: MODEL_URL,
    position: center,
    dimensions: {
      x: 0.98,
      y: 1.16,
      z: 0.98
    },
    script: SCRIPT_URL,
  })


  function cleanup() {
    Entities.deleteEntity(launchButton);
  }

  Script.scriptEnding.connect(cleanup);