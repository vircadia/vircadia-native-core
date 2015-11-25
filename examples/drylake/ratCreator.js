Script.include('steer.js');
var steer = loadSteer();
Script.include('../libraries/tween.js');
var TWEEN = loadTween();

var USE_CONSTANT_SPAWNER = true;

var RAT_SPAWNER_LOCATION = {
    x: 1001,
    y: 98.5,
    z: 1039
};

var RAT_NEST_LOCATION = {
    x: 1003.5,
    y: 99,
    z: 964.2
};

var RAT_DIMENSIONS = {
    x: 0.22,
    y: 0.32,
    z: 1.14
};

var RAT_MODEL_URL = 'http://hifi-content.s3.amazonaws.com/james/rat/models/rat_model.fbx';
var RAT_IDLE_ANIMATION_URL = 'http://hifi-content.s3.amazonaws.com/james/rat/animations/idle.fbx';
var RAT_WALKING_ANIMATION_URL = 'http://hifi-content.s3.amazonaws.com/james/rat/animations/walk.fbx';
var RAT_RUNNING_ANIMATION_URL = 'http://hifi-content.s3.amazonaws.com/james/rat/animations/run.fbx';
var RAT_DEATH_ANIMATION_URL = 'http://hifi-content.s3.amazonaws.com/james/rat/animations/death.fbx';

var RAT_IN_NEST_DISTANCE = 3;

//how many milliseconds between rats
var RAT_SPAWN_RATE = 2500;

function playRatRunningAnimation(rat) {
    print('RUN  RAT')
    var animationSettings = JSON.stringify({
        running: true
    });
    Entities.editEntity(rat, {
        animationURL: RAT_RUNNING_ANIMATION_URL,
        // animationSettings: animationSettings,
        animation: {
            url: RAT_RUNNING_ANIMATION_URL,
            running: true,
            fps: 30
        },
    });
}

function playRatDeathAnimation(rat) {
    var animationSettings = JSON.stringify({
        running: true
    });

    Entities.editEntity(rat, {
        animationURL: RAT_DEATH_ANIMATION_URL,
        animationSettings: animationSettings
            // animation: {
            //     url: RAT_DEATH_ANIMATION_URL,
            //     running: true
            // },
    });
}

var modelRatProperties = {
    name: 'rat',
    type: 'Model',
    modelURL: RAT_MODEL_URL,
    dimensions: RAT_DIMENSIONS,
    position: RAT_SPAWNER_LOCATION,
    shapeType: 'Box',
    collisionsWillMove: true,
    ignoreForCollisions: false,
    gravity: {
        x: 0,
        y: -19.8,
        z: 0
    },
    lifetime: 30,
    rotation: Quat.fromPitchYawRollDegrees(0, 180, 0),
    //disable this if for some reason we want grabbable rats
    userData: JSON.stringify({
        grabbableKey: {
            grabbable: false
        }
    })

};

var targetProperties = {
    name: 'Hifi-Rat-Nest',
    type: 'Box',
    color: {
        red: 0,
        green: 255,
        blue: 0
    },
    dimensions: {
        x: 1,
        y: 1,
        z: 1
    },
    visible: false,
    position: RAT_NEST_LOCATION
};

var target = Entities.addEntity(targetProperties);

function addRat() {
    var rat = Entities.addEntity(modelRatProperties);
    return rat
}

var rats = [];

var AVOIDER_Y_HEIGHT = 99;
var FIRST_AVOIDER_START_POSITION = {
    x: 1004,
    y: AVOIDER_Y_HEIGHT,
    z: 1019
};
var FIRST_AVOIDER_FINISH_POSITION = {
    x: 997,
    y: AVOIDER_Y_HEIGHT,
    z: 1019
};
var SECOND_AVOIDER_START_POSITION = {
    x: 998,
    y: AVOIDER_Y_HEIGHT,
    z: 998
};
var SECOND_AVOIDER_FINISH_POSITION = {
    x: 1005,
    y: AVOIDER_Y_HEIGHT,
    z: 999
};
var THIRD_AVOIDER_START_POSITION = {
    x: 1001.5,
    y: 100,
    z: 978
};
var THIRD_AVOIDER_FINISH_POSITION = {
    x: 1005,
    y: 100,
    z: 974
};

var avoiders = [];
addAvoiderBlock(FIRST_AVOIDER_START_POSITION);
addAvoiderBlock(SECOND_AVOIDER_START_POSITION);
addAvoiderBlock(THIRD_AVOIDER_START_POSITION);

function addAvoiderBlock(position) {
    var avoiderProperties = {
        name: 'Hifi-Rat-Avoider',
        type: 'Box',
        color: {
            red: 255,
            green: 0,
            blue: 255
        },
        dimensions: {
            x: 1,
            y: 1,
            z: 1
        },
        position: position,
        collisionsWillMove: false,
        ignoreForCollisions: true,
        visible: true
    };

    var avoider = Entities.addEntity(avoiderProperties);
    avoiders.push(avoider);
}


tweenAvoider(avoiders[0], FIRST_AVOIDER_START_POSITION, FIRST_AVOIDER_FINISH_POSITION);
tweenAvoider(avoiders[1], SECOND_AVOIDER_START_POSITION, SECOND_AVOIDER_FINISH_POSITION);
tweenAvoider(avoiders[2], THIRD_AVOIDER_START_POSITION, THIRD_AVOIDER_FINISH_POSITION);

function tweenAvoider(entityID, startPosition, endPosition) {
    var ANIMATION_DURATION = 4200;

    var begin = {
        x: startPosition.x,
        y: startPosition.y,
        z: startPosition.z
    };

    var end = endPosition;

    var original = startPosition;

    var tweenHead = new TWEEN.Tween(begin).to(end, ANIMATION_DURATION);

    function updateTo() {
        Entities.editEntity(entityID, {
            position: {
                x: begin.x,
                y: begin.y,
                z: begin.z
            }
        });
    }

    function updateBack() {
        Entities.editEntity(entityID, {
            position: {
                x: begin.x,
                y: begin.y,
                z: begin.z
            }
        })
    }

    var tweenBack = new TWEEN.Tween(begin).to(original, ANIMATION_DURATION).onUpdate(updateBack);

    tweenHead.onUpdate(function() {
        updateTo()
    });

    tweenHead.chain(tweenBack);
    tweenBack.chain(tweenHead);
    tweenHead.start();

}

function updateTweens() {
    TWEEN.update();
}

function moveRats() {
    rats.forEach(function(rat) {
        checkDistanceFromNest(rat);
        //   print('debug1')
        var avatarFlightVectors = steer.fleeAllAvatars(rat);
        // print('avatarFlightVectors' + avatarFlightVectors)
        var i, j;
        var averageAvatarFlight;

        for (i = 0; i < avatarFlightVectors.length; i++) {
            if (i === 0) {
                averageAvatarFlight = avatarFlightVectors[0];
            } else {
                averageAvatarFlight = Vec3.sum(avatarFlightVectors[i - 1], avatarFlightVectors[i])
            }
        }

        // averageAvatarFlight = Vec3.normalize(averageAvatarFlight);

        averageAvatarFlight = Vec3.multiply(averageAvatarFlight, 1 / avatarFlightVectors.length);

        var avoidBlockVectors = steer.fleeAvoiderBlocks(rat);

        var averageAvoiderFlight;

        for (j = 0; j < avoidBlockVectors.length; j++) {
            if (j === 0) {
                averageAvoiderFlight = avoidBlockVectors[0];
            } else {
                averageAvoiderFlight = Vec3.sum(avoidBlockVectors[j - 1], avoidBlockVectors[j])
            }
        };

        // avarageAvoiderFlight = Vec3.normalize(averageAvoiderFlight);

        averageAvoiderFlight = Vec3.multiply(averageAvoiderFlight, 1 / avoidBlockVectors.length);

        var averageVector;
        var seek = steer.arrive(rat, target);
        averageVector = seek;
        var divisorCount = 1;
        if (avatarFlightVectors.length > 0) {
            divisorCount++;
            averageVector = Vec3.sum(averageVector, averageAvatarFlight);
        }
        if (avoidBlockVectors.length > 0) {
            divisorCount++;
            averageVector = Vec3.sum(averageVector, averageAvoiderFlight);
        }

        averageVector = Vec3.multiply(averageVector, 1 / divisorCount);
        var thisRatProps = Entities.getEntityProperties(rat, ["position", "rotation"]);

        var eulerAngle = Quat.safeEulerAngles(thisRatProps.rotation);
        eulerAngle.x = 0;
        eulerAngle.z = 0;
        var constrainedRotation = Quat.fromVec3Degrees(eulerAngle);

        //  print('CR:::'+JSON.stringify(constrainedRotation))

        var ratPosition = thisRatProps.position;
        var ratToNest = Vec3.subtract(RAT_NEST_LOCATION, ratPosition);
        var ratRotation = Quat.rotationBetween(Vec3.UNIT_Z, ratToNest);

        Entities.editEntity(rat, {
            velocity: averageVector,
            rotation: constrainedRotation,
        })

        //  castRay(rat);
    })
}

Script.update.connect(moveRats)
Script.update.connect(updateTweens);

function checkDistanceFromNest(rat) {
    var ratProps = Entities.getEntityProperties(rat, "position");
    var distance = Vec3.distance(ratProps.position, RAT_NEST_LOCATION);
    if (distance < RAT_IN_NEST_DISTANCE) {
        //print('at nest')
        removeRatFromScene(rat);
    } else {
        // print('not yet at nest:::' + distance)

    }
}

function removeRatFromScene(rat) {
    var index = rats.indexOf(rat);
    if (index > -1) {
        rats.splice(index, 1);
    }
    Entities.deleteEntity(rat);
}

function popRatFromStack(entityID) {
    var index = rats.indexOf(entityID);
    if (index > -1) {
        rats.splice(index, 1);
    }
}

Entities.deletingEntity.connect(popRatFromStack);

function cleanup() {
    while (rats.length > 0) {
        Entities.deleteEntity(rats.pop());
    }

    while (avoiders.length > 0) {
        Entities.deleteEntity(avoiders.pop());
    }

    Entities.deleteEntity(target);
    Script.update.disconnect(moveRats);
    Script.update.disconnect(updateTweens);
    Entities.deletingEntity.disconnect(popRatFromStack);
    Script.clearInterval(ratSpawnerInterval);
}

Script.scriptEnding.connect(cleanup);

var ratSpawnerInterval;

if (USE_CONSTANT_SPAWNER === true) {
    ratSpawnerInterval = Script.setInterval(function() {
        var rat = addRat();
        playRatRunningAnimation(rat);
        rats.push(rat);
    }, RAT_SPAWN_RATE);
}

//unused for now, to be used for some collision avoidance on walls and stuff?

// function castRay(rat) {
//     var ratProps = Entities.getEntityProperties(rat, ["position", "rotation"]);
//     var shotDirection = Quat.getFront(ratProps.rotation);
//     var pickRay = {
//         origin: ratProps.position,
//         direction: shotDirection
//     };

//     var intersection = Entities.findRayIntersection(pickRay, true);
//     if (intersection.intersects) {
//         var distance = Vec3.subtract(intersection.properties.position, ratProps.position);
//         distance = Vec3.length(distance);
//         //   print('INTERSECTION:::'+distance);
//     } else {
//         //print('no intersection')
//     }
// }