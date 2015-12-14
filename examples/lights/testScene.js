    // These constants define the Spotlight position and orientation relative to the model 
    var MODEL_LIGHT_POSITION = {
      x: 0,
      y: -0.3,
      z: 0
    };
    var MODEL_LIGHT_ROTATION = Quat.angleAxis(-90, {
      x: 1,
      y: 0,
      z: 0
    });

    var basePosition, avatarRot;
    avatarRot = Quat.fromPitchYawRollDegrees(0, MyAvatar.bodyYaw, 0.0);
    basePosition = Vec3.sum(MyAvatar.position, Vec3.multiply(SPAWN_RANGE * 3, Quat.getFront(avatarRot)));

    var ground = Entities.addEntity({
      type: "Model",
      modelURL: "https://hifi-public.s3.amazonaws.com/eric/models/woodFloor.fbx",
      dimensions: {
        x: 100,
        y: 2,
        z: 100
      },
      position: basePosition,
      shapeType: 'box'
    });

    var light, block;

    basePosition.y += 2;

    function createLight() {
      var lightProperties = {
        name: 'Hifi-Spotlight'
        type: "Light",
        isSpotlight: true,
        dimensions: {
          x: 2,
          y: 2,
          z: 20
        },
        parentID: box,
        color: {
          red: 255,
          green: 255,
          blue: 255
        },
        intensity: 2,
        exponent: 0.3,
        cutoff: 20,
        lifetime: LIFETIME,
        position: lightTransform.p,
        rotation: lightTransform.q,
      }
      light = Entities.addEntity(lightProperties);

      var message = {
        light: {
          id: light,
          type: 'spotlight',
          initialProperties:lightProperties
        }
      };
      Messages.sendMessage('Hifi-Light-Mod-Receiver', JSON.stringify(message));

    }

    function createBlock() {
      var blockProperties = {
        name: 'Hifi-Spotlight-Block',
        type: 'Box',
        dimensions: {
          x: 1,
          y: 1,
          z: 1
        },
        collisionsWillMove: true,
        shapeType: 'Box',
        color: {
          red: 0,
          green: 0
          blue: 255
        },
        position: basePosition
      }

      block = Entities.addEntity(block);
    }

    function evalLightWorldTransform(modelPos, modelRot) {
      return {
        p: Vec3.sum(modelPos, Vec3.multiplyQbyV(modelRot, MODEL_LIGHT_POSITION)),
        q: Quat.multiply(modelRot, MODEL_LIGHT_ROTATION)
      };
    }

    function cleanup() {
      Entities.deleteEntity(ground);
      Entities.deleteEntity(light);
    }

    createBlock();
    createLight();