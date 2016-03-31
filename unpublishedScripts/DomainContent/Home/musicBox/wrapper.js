HomeMusicBox = function(spawnPosition, spawnRotation) {

    var SHOULD_CLEANUP = false;

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
        x: 0.1661,
        y: 0.1010,
        z: 0.2256
    };

    var BASE_POSITION = center;

    var LID_DIMENSIONS = {
        x: 0.1435,
        y: 0.0246,
        z: 0.1772
    };

    var LID_OFFSET = {
        x: 0,
        y: BASE_DIMENSIONS.y / 2,
        z: 0
    };

    var LID_REGISTRATION_POINT = {
        x: 0,
        y: 0.5,
        z: 0.5
    }

    var LID_SCRIPT_URL = Script.resolvePath('lid.js?' + Math.random());

    var LID_MODEL_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/musicBox/MusicBoxLid.fbx';
    var BASE_MODEL_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/musicBox/MusicBoxNoLidNoanimation.fbx';


    var base, lid;

    function createBase() {
        var baseProperties = {
            name: 'hifi-home-music-box-base',
            type: 'Model',
            modelURL: BASE_MODEL_URL,
            position: BASE_POSITION,
            dimensions: BASE_DIMENSIONS,
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                }
            }),
        }

        base = Entities.addEntity(baseProperties);
        createLid(base);
    };


    function createLid(baseID) {

        var baseProps = Entities.getEntityProperties(baseID);
        var frontVector = Quat.getFront(baseProps.rotation);
        var rightVector = Quat.getRight(baseProps.rotation);
        var backVector = Vec3.multiply(-1, frontVector);
        var backOffset = 0.0125;
        var backPosition = baseProps.position;
        var backPosition = Vec3.sum(baseProps.position, Vec3.multiply(backOffset, backVector));
        backPosition.y = backPosition.y +=(BASE_DIMENSIONS.y / 2)
        // backPosition = Vec3.sum(backPosition,LID_OFFSET)
        print('backPosition' + JSON.stringify(backPosition));
        var lidProperties = {
            name: 'hifi-home-music-box-lid',
            type: 'Model',
            modelURL: LID_MODEL_URL,
            dimensions: LID_DIMENSIONS,
            position: baseProps.position,
            registrationPoint: LID_REGISTRATION_POINT,
            dynamic: false,
            script: LID_SCRIPT_URL,
            collidesWith: 'myAvatar,otherAvatar',
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true,
                    'musicBoxBase': baseID
                },
                grabConstraintsKey: {
                    callback: 'rotateLid',
                    positionLocked: true,
                    rotationLocked: false,
                    positionMod: false,
                    rotationMod: {
                        roll: {
                            min: 0,
                            max: 75,
                            startingAxis: 'y',
                            startingPoint: backPosition.y,
                            distanceToMax: backPosition.y + 0.35,
                        },
                        yaw: false,
                        pitch: false
                    }
                },
                grabbableKey: {
                    wantsTrigger: true,
                    disableReleaseVelocity: true
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


    return this;
}