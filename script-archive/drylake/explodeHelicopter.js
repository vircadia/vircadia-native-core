var explosionSound = SoundCache.getSound("https://s3.amazonaws.com/hifi-public/eric/sounds/explosion.wav");

var partsURLS = [{
    url: "https://s3.amazonaws.com/hifi-public/eric/models/blade.fbx",
    dimensions: {
        x: 2,
        y: 2,
        z: 2
    }
}, {
    url: "https://s3.amazonaws.com/hifi-public/eric/models/body.fbx",
    dimensions: {
        x: 2.2,
        y: 2.98,
        z: 7.96
    }
}, {
    url: "https://s3.amazonaws.com/hifi-public/eric/models/tail.fbx",
    dimensions: {
        x: 1,
        y: 1,
        z: 1
    }
}];

var parts = [];
var emitters = [];

var explodePosition;
var helicopter;
var entities = Entities.findEntities(MyAvatar.position, 2000);
for (i = 0; i < entities.length; i++) {
    var name = Entities.getEntityProperties(entities[i], 'name').name;
    if (name === "Helicopter") {
        var helicopter = entities[i];
        explodeHelicopter(Entities.getEntityProperties(helicopter, 'position').position);
    }
}


function explodeHelicopter(explodePosition) {
    Audio.playSound(explosionSound, {
        position: explodePosition,
        volume: 0.5
    });
    Entities.deleteEntity(helicopter);
    for (var i = 0; i < partsURLS.length; i++) {
        var position = Vec3.sum(explodePosition, {
            x: 1,
            y: 1,
            z: 1
        });
        var part = Entities.addEntity({
            type: "Model",
            modelURL: partsURLS[i].url,
            dimensions: partsURLS[i].dimensions,
            position: position,
            shapeType: "box",
            dynamic: true,
            damping: 0,
            gravity: {
                x: 0,
                y: -9.6,
                z: 0
            },
            velocity: {
                x: Math.random(),
                y: -10,
                z: Math.random()
            }
        });

        var emitter = Entities.addEntity({
            type: "ParticleEffect",
            name: "fire",
            isEmitting: true,
            textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
            position: explodePosition,
            emitRate: 100,
            colorStart: {
                red: 70,
                green: 70,
                blue: 137
            },
            color: {
                red: 200,
                green: 99,
                blue: 42
            },
            colorFinish: {
                red: 255,
                green: 99,
                blue: 32
            },
            radiusSpread: 0.2,
            radiusStart: 0.3,
            radiusEnd: 0.04,
            particleRadius: 0.09,
            radiusFinish: 0.0,
            emitSpeed: 0.1,
            speedSpread: 0.1,
            alphaStart: 0.1,
            alpha: 0.7,
            alphaFinish: 0.1,
            emitOrientation: Quat.fromPitchYawRollDegrees(-90, 0, 0),
            emitDimensions: {
                x: 1,
                y: 1,
                z: 0.1
            },
            polarFinish: Math.PI,
            polarStart: 0,
      
            accelerationSpread: {
                x: 0.1,
                y: 0.01,
                z: 0.1
            },
            lifespan: 1,
        });
        emitters.push(emitter)
        parts.push(part);
    }

    Script.setTimeout(function() {
        var pos = Entities.getEntityProperties(parts[1], "position").position;
        Entities.editEntity(emitters[0], {position: Vec3.sum(pos, {x: Math.random(), y: Math.random(), z: Math.random()})});
        Entities.editEntity(emitters[1], {position: Vec3.sum(pos, {x: Math.random(), y: Math.random(), z: Math.random()})});
        Entities.editEntity(emitters[2], {position: Vec3.sum(pos, {x: Math.random(), y: Math.random(), z: Math.random()})});
    }, 5000)


}

function cleanup() {
    parts.forEach(function(part) {
        Entities.deleteEntity(part);
    });
    emitters.forEach(function(emitter){
        Entities.deleteEntity(emitter);
    })
}

Script.scriptEnding.connect(cleanup);
