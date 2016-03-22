  var orientation = Camera.getOrientation();
  orientation = Quat.safeEulerAngles(orientation);
  orientation.x = 0;
  orientation = Quat.fromVec3Degrees(orientation);
  var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(orientation)));

  // Math.random ensures no caching of script
  var SCRIPT_URL = Script.resolvePath("batonSoundTestEntityScript.js")

  var soundEntity = Entities.addEntity({
      type: "Box",
      color: {
          red: 200,
          green: 10,
          blue: 10
      },
      position: center,
      dimensions: {
          x: 0.1,
          y: 0.1,
          z: 0.1
      },
      script: SCRIPT_URL
  });


  function cleanup() {
      // Entities.deleteEntity(soundEntity);
  }

  Script.scriptEnding.connect(cleanup);