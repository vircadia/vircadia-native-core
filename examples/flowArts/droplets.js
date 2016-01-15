  Script.include("../libraries/utils.js");

  var orientation = Camera.getOrientation();
  orientation = Quat.safeEulerAngles(orientation);
  orientation.x = 0;
  orientation = Quat.fromVec3Degrees(orientation);
  var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(orientation)));
 
  var emitters = [];
var spheres = []

createNodes();

function createNodes() {
    var numNodes = 10;
    for (var i = 0; i < numNodes; i++) {
        var position = Vec3.sum(center, {x: randFloat(0, 3), y: 0, z: randFloat(0, 3)});
        createNode(position)
    }
}
createNode(center);

function createNode(position) {
     var sphereDimensions = {
      x: 0.8,
      y: 0.1,
      z: 0.8
  };
  var sphere = Entities.addEntity({
      type: "Sphere",
      name: "ParticlesTest Sphere",
      position: position,
      dimensions: sphereDimensions,
      color: {
          red: 128,
          green: 128,
          blue: 128
      },
      lifetime: 3600, // 1 hour; just in case,
      collisionsWillMove: true
  });
  spheres.push(sphere);

  var props = {
      "type": "ParticleEffect",
      "position": position,
      parentID: sphere,
      "isEmitting": true,
      "maxParticles": 1000,
      "lifespan": 5,
      "emitRate": 1000,
      "emitSpeed": 0.025,
      "speedSpread": 0,
      "emitDimensions": {x: 2, y: 2, z: .1},
      "emitRadiusStart": 1,
      "polarStart": 0,
      "polarFinish": 1.570796012878418,
      "azimuthStart": -3.1415927410125732,
      "azimuthFinish": 3.1415927410125732,
      "emitAcceleration": {
          "x": 0,
          "y": 0,
          "z": 0
      },
      "particleRadius": 0.04,
      "radiusSpread": 0,
      "radiusStart": 0.04,
      radiusFinish: 0.04,
      "colorStart": {
          "red": 0,
          "green": 0,
          "blue": 255
      },
      color: {
        red: 10, green: 10, blue: 200
      },
      "colorFinish": {
          "red": 200,
          "green":200,
          "blue": 255
      },
      "alpha": 1,
      "alphaSpread": 0,
      "alphaStart": 0,
      "alphaFinish": 0,
      "emitterShouldTrail": true,
      "textures": "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png"
  }

  var oceanEmitter = Entities.addEntity(props);
  emitters.push(oceanEmitter);
}




  function cleanup() {
    emitters.forEach(function(emitter) {
      Entities.deleteEntity(emitter);
        
    })
    spheres.forEach(function(sphere) {
        Entities.deleteEntity(sphere);
    })
  }

  Script.scriptEnding.connect(cleanup);