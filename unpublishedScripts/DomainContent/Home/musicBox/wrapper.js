var HomeMusicBox = function() {

    var SHOULD_CLEANUP = true;

    var WHITE = {
        red: 255,
        green: 255,
        blue: 255
    };

    var RED = {
        red: 255,
        green: 0,
        blue: 0
    };

    var GREEN = {
        red: 0,
        green: 255,
        blue: 0
    };

    var BLUE = {
        red: 0,
        green: 0,
        blue: 255
    };

    var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
        x: 0,
        y: 0.5,
        z: 0
    }), Vec3.multiply(1, Quat.getFront(Camera.getOrientation())));

    var BASE_DIMENSIONS = {
        x: 0.5,
        y: 0.5,
        z: 0.5
    };

    var BASE_POSITION = center;

    var LID_DIMENSIONS = {
        x: BASE_DIMENSIONS.x,
        y: BASE_DIMENSIONS.y / 8,
        z: BASE_DIMENSIONS.z
    };

    var LID_OFFSET = {
        x: 0,
        y: BASE_DIMENSIONS.y / 2 + (LID_DIMENSIONS.y / 2),
        z: 0.25
    };

    var LID_REGISTRATION_POINT = {
        x: 0.5,
        y: 0.5,
        z: 1
    }

    var LID_SCRIPT_URL = Script.resolvePath('lid.js?' + Math.random());

    var base, lid;


    function createBase() {
        var baseProperties = {
            name: 'hifi-home-music-box-base',
            type: 'Box',
            color: WHITE,
            position: BASE_POSITION,
            dimensions: BASE_DIMENSIONS
        }

        base = Entities.addEntity(baseProperties);
    };


    function createLid() {

        var lidPosition = Vec3.sum(BASE_POSITION, LID_OFFSET);
        print('LID DIMENSIONS:' + JSON.stringify(lidPosition))
        var lidProperties = {
            name: 'hifi-home-music-box-lid',
            type: 'Box',
            color: BLUE,
            dimensions: LID_DIMENSIONS,
            position: lidPosition,
            registrationPoint: LID_REGISTRATION_POINT,
            dynamic: false,
            script: LID_SCRIPT_URL,
            collidesWith: 'myAvatar,otherAvatar',
            userData: JSON.stringify({
                grabConstraintsKey: {
                    callback: 'rotateLid',
                    positionLocked: true,
                    rotationLocked: false,
                    positionMod: false,
                    rotationMod: {
                        pitch: {
                            min: 0,
                            max: 75,
                            startingAxis: 'y',
                            startingPoint: lidPosition.y,
                            distanceToMax: lidPosition.y + 0.35,
                        },
                        yaw: false,
                        roll: false
                    }
                },
                grabbableKey: {
                    wantsTrigger: true,
                    disableReleaseVelocity: true,
                    disableMoveWithHead: true,
                    disableFarGrab: true,
                    disableEquip: true
                }
            })
        }

        lid = Entities.addEntity(lidProperties);

    };

    var theta = 0;
    var min = 0;
    var max = 60;
    var direction = "up";

    function animateLid() {
        theata += 1
    }

    // if (SHOULD_CLEANUP === true) {
    //     Script.scriptEnding.connect(cleanup);
    // }

    function cleanup() {
        Entities.deleteEntity(base);
        Entities.deleteEntity(lid);
    }

    this.cleanup = cleanup;

    createBase();
    createLid();
    
    return this;
}