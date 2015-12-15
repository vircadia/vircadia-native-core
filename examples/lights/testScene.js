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
    basePosition = Vec3.sum(MyAvatar.position, Vec3.multiply(-3, Quat.getUp(avatarRot)));

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

    // basePosition.y += 2;

    function createLight() {
      print('making light' + block)
      var blockProperties = Entities.getEntityProperties(block, ["position", "rotation"]);
      var lightTransform = evalLightWorldTransform(blockProperties.position, blockProperties.rotation);
      var lightProperties = {
        name: 'Hifi-Spotlight',
        type: "Light",
        isSpotlight: true,
        dimensions: {
          x: 2,
          y: 2,
          z: 20
        },
        parentID: block,
        color: {
          red: 255,
          green: 0,
          blue: 255
        },
        intensity: 2,
        exponent: 0.3,
        cutoff: 20,
        lifetime: -1,
        position: lightTransform.p,
        rotation: lightTransform.q
      };

      light = Entities.addEntity(lightProperties);

      var message = {
        light: {
          id: light,
          type: 'spotlight',
          initialProperties: lightProperties
        }
      };

      Messages.sendMessage('Hifi-Light-Mod-Receiver', JSON.stringify(message));
      print('SENT MESSAGE')

    }

    function createBlock() {
      print('making block');

      var position = basePosition;
      position.y += 5;
      var blockProperties = {
        name: 'Hifi-Spotlight-Block',
        type: 'Box',
        dimensions: {
          x: 1,
          y: 1,
          z: 1
        },
        collisionsWillMove: true,
        color: {
          red: 0,
          green: 0,
          blue: 255
        },
        position: position
      }

      block = Entities.addEntity(blockProperties);
    }

    function evalLightWorldTransform(modelPos, modelRot) {
      return {
        p: Vec3.sum(modelPos, Vec3.multiplyQbyV(modelRot, MODEL_LIGHT_POSITION)),
        q: Quat.multiply(modelRot, MODEL_LIGHT_ROTATION)
      };
    }

    function cleanup() {
      Entities.deleteEntity(block);
      Entities.deleteEntity(ground);
      Entities.deleteEntity(light);
    }

    Script.scriptEnding.connect(cleanup);

    createBlock();
    createLight();