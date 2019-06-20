  var orientation = Camera.getOrientation();
  orientation = Quat.safeEulerAngles(orientation);
  orientation.x = 0;
  orientation = Quat.fromVec3Degrees(orientation);
  var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getForward(orientation)));

  // Math.random ensures no caching of script
  var SCRIPT_URL = Script.resolvePath("myEntityScript.js")

  var myEntity = Entities.addEntity({
      type: "Sphere",
      color: {
          red: 200,
          green: 10,
          blue: 200
      },
      position: center,
      dimensions: {
          x: 1,
          y: 1,
          z: 1
      },
      script: SCRIPT_URL
  })


  function cleanup() {
      // Entities.deleteEntity(myEntity);
  }

  Script.scriptEnding.connect(cleanup);