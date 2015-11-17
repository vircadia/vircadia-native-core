Script.include('steer.js')
var steer = loadSteer();
Script.include('tween.js');
var TWEEN = loadTween();

var USE_CONSTANT_SPAWNER = false;

var RAT_SPAWNER_LOCATION = {
    x: 0,
    y: 0,
    z: 0
};

var RAT_NEST_LOCATION = {
    x: 0,
    y: 0,
    z: 0
};

var RAT_MODEL_URL = '';
var RAT_RUNNING_ANIMATION_URL = '';
var RAT_DEATH_ANIMATION_URL = '';

var RAT_IN_NEST_DISTANCE = 0.25;

//how many milliseconds between rats
var RAT_SPAWN_RATE = 1000;


function playRatRunningAnimation() {
    var animationSettings = JSON.stringify({
        running: true
    });
    Entities.editEntity(rat, {
        animationURL: RAT_RUNNING_ANIMATION_URL,
        animationSettings: animationSettings,
        // animation: {
        //     url: RAT_RUNNING_ANIMATION_URL,
        //     running: true,
        //     fps: 180
        // },
    });
}

function playRatDeathAnimation() {
    var animationSettings = JSON.stringify({
        running: true
    });

    Entities.editEntity(rat, {
        animationURL: RAT_DEATH_ANIMATION_URL,
        animationSettings: animationSettings,
        // animation: {
        //     url: RAT_DEATH_ANIMATION_URL,
        //     running: true,
        //     fps: 180
        // },
    });
}

var ratProperties = {
    name: 'Hifi-Rat',
    type: 'Box',
    color: {
        red: 0,
        green: 0,
        blue: 255
    },
    dimensions: {
        x: 1,
        y: 1,
        z: 1
    },
    position: RAT_SPAWNER_LOCATION
};

var modelRatProperties = {
    name: 'Hifi-Rat',
    type: 'Model',
    dimensions: RAT_DIMENSIONS,
    position: RAT_SPAWNER_LOCATION,
    shapeType: 'Box',
    collisionsWillMove: true,
    gravity: {
        x: 0,
        y: -9.8
        z: 0
    },
    //enable this if for some reason we want grabbable rats
    // userData:JSON.stringify({
    //     grabbableKey:{
    //         grabbable:false
    //     }
    // })

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
    position: RAT_NEST_LOCATION
        //  script: Script.resolvePath('rat.js')
};

var target = Entities.addEntity(targetProperties)

function addRat() {
    var rat = Entities.addEntity(ratProperties);
    rats.push(rat);
}

var rats = [];
addRat();


var FIRST_AVOIDER_START_POSITION = {
    x: 0,
    y: 0,
    z: 0
};
var FIRST_AVOIDER_FINISH_POSITION = {
    x: 0,
    y: 0,
    z: 0
};
var SECOND_AVOIDER_START_POSITION = {
    x: 0,
    y: 0,
    z: 0
};
var SECOND_AVOIDER_FINISH_POSITION = {
    x: 0,
    y: 0,
    z: 0
};
var THIRD_AVOIDER_START_POSITION = {
    x: 0,
    y: 0,
    z: 0
};
var THIRD_AVOIDER_FINISH_POSITION = {
    x: 0,
    y: 0,
    z: 0
};

var avoiders = [
    addAvoiderBlock(FIRST_AVOIDER_START_POSITION),
    addAvoiderBlock(SECOND_AVOIDER_START_POSITION),
    addAvoiderBlock(THIRD_AVOIDER_START_POSITION)
];

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
        position: {
            x: 1,
            y: 1,
            z: 1
        },
        collisionsWillMove: false,
        ignoreForCollisions: true
    }

    var avoider = Entities.addEntity(avoiderProperties);
    avoiders.push(avoider);
};

addAvoiderBlock();
tweenAvoider(avoiders[0]);

function tweenAvoider(entityID, startPosition, endPosition) {
    var ANIMATION_DURATION = 500;

    var begin = {
        x: startPosition.x,
        y: startPosition.y,
        z: startPosition.z
    };

    var target = endPosition;

    var original = startPosition;

    var tweenHead = new TWEEN.Tween(begin).to(target, ANIMATION_DURATION);

    function updateTo() {
        Entities.editEntity(entityID, {
            position: {
                x: begin.x,
                y: begin.y,
                z: begin.z
            }
        })
    };

    function updateBack() {
        Entities.editEntity(entityID, {
            position: {
                x: begin.x,
                y: begin.y,
                z: begin.z
            }
        })
    };

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
        //   print('debug1')

        var avatarFlightVectors = steer.fleeAllAvatars(rat);
        print('avatarFlightVectors' + avatarFlightVectors)
        var i, j;
        var averageAvatarFlight;

        for (i = 0; i < avatarFlightVectors.length; i++) {
            if (i === 0) {
                averageAvatarFlight = avatarFlightVectors[0];
            } else {
                averageAvatarFlight = Vec3.sum(avatarFlightVectors[i - 1], avatarFlightVectors[i])
            }
        }

        averageAvatarFlight = Vec3.normalize(averageAvatarFlight);

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

        avarageAvoiderFlight = Vec3.normalize(averageAvoiderFlight);

        averageAvoiderFlight = Vec3.multiply(averageAvoiderFlight, 1 / avoidBlockVectors.length);

        var averageVector;
        var seek = steer.arrive(rat, target);
        averageVector = seek;
        var divisorCount = 1;
        if (avatarFlightVectors.length > 0) {
            divisorCount++;
            averageVector = Vec3.sum(averageVector, averageAvatarFlight);
        }
        if (avoidBlockVectors > 0) {
            divisorCount++;
            averageVector = Vec3.sum(averageVector, averageAvoiderFlight);
        }

        averageVector = Vec3.multiply(averageVector, 1 / divisorCount);

        Entities.editEntity(rat, {
            velocity: averageVector
        })

        //  castRay(rat);

    })
}

Script.update.connect(moveRats)
Script.update.connect(updateTweens);

function checkDistanceFromNest(rat) {
    var ratProps = Entitis.getEntityProperties(rat, "position");
    var distance = Vec3.distance(ratProps.position, RAT_NEST_LOCATION);
    if (distance < RAT_IN_NEST_DISTANCE) {
        removeRatFromScene();
    }
}

function removeRatFromScene(rat) {
    var index = rats.indexOf(rat);
    if (index > -1) {
        rats.splice(index, 1);
    }
    Entities.deleteEntity(rat);
}



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
    Script.clearInterval(ratSpawnerInterval);
}

Script.scriptEnding.connect(cleanup)

var ratSpawnerInterval;

if (USE_CONSTANT_SPAWNER === true) {
    ratSpawnerInterval = Script.setInterval(function() {
        addRat();
    }, RAT_SPAWN_RATE)
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