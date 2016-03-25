  var orientation = MyAvatar.orientation;
  orientation = Quat.safeEulerAngles(orientation);
  orientation.x = 0;
  orientation = Quat.fromVec3Degrees(orientation);
  var center = Vec3.sum(MyAvatar.getHeadPosition(), Vec3.multiply(2, Quat.getFront(orientation)));


  var SCRIPT_URL = Script.resolvePath("cowEntityScript.js?v1" + Math.random());
  var cow = Entities.addEntity({
      type: "Model",
      modelURL: "http://hifi-content.s3.amazonaws.com/DomainContent/production/cow/newMooCow.fbx",
      name: "playa_model_throwinCow",
      position: center,
      animation: {
          currentFrame: 278,
          running: true,
          url: "http://hifi-content.s3.amazonaws.com/DomainContent/Junkyard/Playa/newMooCow.fbx"
      },
      dimensions: {
          x: 0.739,
          y: 1.613,
          z: 2.529
      },
      dynamic: true,
      gravity: {
          x: 0,
          y: -5,
          z: 0
      },
      shapeType: "box",
      script: SCRIPT_URL,
      userData: "{\"grabbableKey\":{\"grabbable\":true}}"
  });


  function cleanup() {
      Entities.deleteEntity(cow);
  }

  Script.scriptEnding.connect(cleanup);