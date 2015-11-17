Script.include('steer.js');
var steer = loadSteer();
Script.include('tween.js');
var TWEEN = loadTween();

var RAT_NEST_LOCATION = {
    x: 0,
    y: 0,
    z: 0
};

var RAT_SPAWNER_LOCATION = {
    x: 0,
    y: 0,
    z: 0
};

var AVOIDER_BLOCK_START_LOCATION = {
    x: 0,
    y: 0,
    z: 0
};

var ratProperties = {
    name: 'Hifi-Rat-Nest',
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

var targetProperties = {
    name: 'Hifi-Rat',
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
    // script: Script.resolvePath('rat.js')
};

var target = Entities.addEntity(targetProperties)

function addRat() {
    var rat = Entities.addEntity(ratProperties);
    rats.push(rat);
}

var rats = [];
addRat();

var avoiders = [];

function addAvoiderBlock() {

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
        collisionsWillMove:false,
        ignoreForCollisions:true,
        visible: true
    }

    var avoider = Entities.addEntity(avoiderProperties);
    avoiders.push(avoider)
}

addAvoiderBlock();
tweenAvoider(avoiders[0]);

function tweenAvoider(entityID) {
    var ANIMATION_DURATION = 500;

    var begin = {
        x: 1,
        y: 1,
        z: 1
    }
    var target = {
        x: 3,
        y: 1,
        z: 3
    }
    var original = {
        x: 1,
        y: 1,
        z: 1
    }
    var tweenHead = new TWEEN.Tween(begin).to(target, ANIMATION_DURATION);

    function updateTo() {
        Entities.editEntity(entityID, {
            position: {
                x: begin.x,
                y: begin.y,
                z: begin.z
            }
        })
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
    })
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
        var i;
        var averageAvatarFlight;

        for (i = 0; i < avatarFlightVectors.length; i++) {
            print('debug2')

            if (i === 0) {
                averageAvatarFlight = avatarFlightVectors[0];
            } else {
                averageAvatarFlight = Vec3.sum(avatarFlightVectors[i - 1], avatarFlightVectors[i])
            }
        }

        averageAvatarFlight = Vec3.multiply(averageAvatarFlight, 1 / avatarFlightVectors.length);


        // var avoidBlockVectors = steer.fleeAvoiderBlocks(rat);

        // var averageAvoiderFlight;

        // for (i = 0; i < avoidBlockVectors.length; i++) {
        //     if (i === 0) {
        //         averageAvoiderFlight = avoidBlockVectors[0];
        //     } else {
        //         averageAvoiderFlight = Vec3.sum(avoidBlockVectors[i - 1], avoidBlockVectors[i])
        //     }
        // }

        // averageAvoiderFlight = Vec3.multiply(averageAvoiderFlight, 1 / avoidBlockVectors.length);



        var averageVector;
        var seek = steer.arrive(rat, target);
        averageVector = seek;
        var divisorCount = 1;
        if (avatarFlightVectors.length > 0) {
            divisorCount++;
            averageVector = Vec3.sum(averageVector, averageAvatarFlight);
        }
        // if (avoidBlockVectors > 0) {
        //     divisorCount++
        //     averageVector = Vec3.sum(averageVector, averageAvoiderFlight);
        // }

        averageVector = Vec3.multiply(averageVector, 1 / divisorCount);
        print('AVERAGE VECTOR:::' + JSON.stringify(averageVector))
            //  castRay(rat);
        Entities.editEntity(rat, {
            velocity: averageVector
        })

    })
}

Script.update.connect(moveRats)
Script.update.connect(updateTweens);

function castRay(rat) {
    var ratProps = Entities.getEntityProperties(rat, ["position", "rotation"]);
    var shotDirection = Quat.getFront(ratProps.rotation);
    var pickRay = {
        origin: ratProps.position,
        direction: shotDirection
    };

    var intersection = Entities.findRayIntersection(pickRay, true);
    if (intersection.intersects) {
        var distance = Vec3.subtract(intersection.properties.position, ratProps.position);
        distance = Vec3.length(distance);
        //   print('INTERSECTION:::'+distance);
    } else {
        print('no intersection')
    }
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


}

Script.scriptEnding.connect(cleanup)

// Script.setInterval(function() {
//     addRat();
// })