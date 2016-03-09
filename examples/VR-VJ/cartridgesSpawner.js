  var orientation = MyAvatar.orientation;
  orientation = Quat.safeEulerAngles(orientation);
  orientation.x = 0;
  orientation = Quat.fromVec3Degrees(orientation);
  var center = Vec3.sum(MyAvatar.getHeadPosition(), Vec3.multiply(2, Quat.getFront(orientation)));


Script.include("../libraries/utils.js");

var SOUND_SCRIPT_URL = Script.resolvePath("VRVJSoundCartridgeEntityScript.js");
var SOUND_CARTRIDGE_NAME = "VRVJ-Sound-Cartridge";
var soundEntity = Entities.addEntity({
      type: "Box",
      name: SOUND_CARTRIDGE_NAME,
      dimensions: {x: 0.1, y: 0.1, z: 0.1},
      color: {red: 200, green: 10, blue: 200},
      position: center,
      damping: 1,
      angularDamping: 1,
      dynamic: true,
      script: SOUND_SCRIPT_URL,
      userData: JSON.stringify({
        soundURL: "https://s3-us-west-1.amazonaws.com/hifi-content/eric/Sounds/VRVJ/Synth-MarchToWar.wav",
      })
});

var VISUAL_SCRIPT_URL = Script.resolvePath("VRVJVisualCartridgeEntityScript.js?v1" + Math.random());
var visualEntity = Entities.addEntity({
    type: "Sphere",
    name: "VRVJ-Visual-Cartridge",
    dimensions: {x: 0.1, y: 0.1, z: 0.1},
    damping: 1,
    angularDamping: 1,
    color: {red: 0, green: 200, blue: 10},
    dynamic: true,
    position: Vec3.subtract(center, {x: 0, y: 0.2, z: 0}),
    script: VISUAL_SCRIPT_URL
});
Script.setTimeout(function() {
    // Wait for sounds to load
    Entities.callEntityMethod(soundEntity, "playSound");
}, 1000)

function cleanup() {
    Entities.deleteEntity(soundEntity);
    Entities.deleteEntity(visualEntity);
}

Script.scriptEnding.connect(cleanup);