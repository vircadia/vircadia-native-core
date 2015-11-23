    var ARROW_HIT_SOUND_URL = 'http://hifi-content.s3.amazonaws.com/james/bow_and_arrow/sounds/Arrow_impact1.L.wav'

    var ARROW_DIMENSIONS = {
        x: 0.02,
        y: 0.02,
        z: 0.64
    };

    var ARROW_OFFSET = -0.36;
    var ARROW_TIP_OFFSET = 0.32;
    var ARROW_GRAVITY = {
        x: 0,
        y: -4.8,
        z: 0
    };

    var ARROW_MODEL_URL = "http://hifi-content.s3.amazonaws.com/james/bow_and_arrow/models/newarrow_textured.fbx";
    var ARROW_COLLISION_HULL_URL = "http://hifi-content.s3.amazonaws.com/james/bow_and_arrow/models/newarrow_collision_hull.obj";

    var ARROW_DIMENSIONS = {
        x: 0.02,
        y: 0.02,
        z: 0.64
    };

    var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
        x: 0,
        y: 0.5,
        z: 0
    }), Vec3.multiply(1.5, Quat.getFront(Camera.getOrientation())));

    var arrow;
    createArrow = function() {

        arrow = Entities.addEntity({
            name: 'Hifi-Arrow',
            type: 'Model',
            modelURL: ARROW_MODEL_URL,
            shapeType: 'compound',
            compoundShapeURL: ARROW_COLLISION_HULL_URL,
            dimensions: ARROW_DIMENSIONS,
            position: center,
            collisionsWillMove: false,
            ignoreForCollisions: true,

        });

        return arrow
    }


    createArrow();
    var yRotation = 0;
    var rotateInterval = Script.setInterval(function() {
        Entities.editEntity(arrow, {
            rotation: Quat.fromPitchYawRollDegrees(0, yRotation, 0)
        })
        yRotation += 1
        print('changing rotation')
    }, 17)

    Script.setTimeout(function() {
         Script.clearInterval(rotateInterval)
        print('interval cleared')
        Entities.editEntity(arrow, {
            velocity: {
                x: 0.5,
                y: 0.25,
                z: 1
            },
            collisionsWillMove: true,
        })
        print('entity edited')
    }, 2000)