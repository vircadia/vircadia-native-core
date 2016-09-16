//
//  Created by Thijs Wenker on September 14, 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

const TEST_MODE = false;
const SCRIPT_URL = 'https://dl.dropboxusercontent.com/u/14997455/hifi/butaneLighter/butaneLighter.js?v=' + Date.now();
//const SCRIPT_URL = Script.resolvePath("butaneLighter.js");

function getResourceURL(file) {
    return 'http://hifi-content.s3.amazonaws.com/DomainContent/Tutorial/' + file;
};

//Creates an entity and returns a mixed object of the creation properties and the assigned entityID
var createEntity = function(entityProperties, parent) {
    if (parent.rotation !== undefined) {
        if (entityProperties.rotation !== undefined) {
            entityProperties.rotation = Quat.multiply(parent.rotation, entityProperties.rotation);
        } else {
            entityProperties.rotation = parent.rotation;
        }
    }
    if (parent.position !== undefined) {
        var localPosition = (parent.rotation !== undefined) ? Vec3.multiplyQbyV(parent.rotation, entityProperties.position) : entityProperties.position;
        entityProperties.position = Vec3.sum(localPosition, parent.position)
    }
    if (parent.id !== undefined) {
        entityProperties.parentID = parent.id;
    }
    entityProperties.id = Entities.addEntity(entityProperties);
    return entityProperties;
};

createButaneLighter = function(transform) {
    var entityProperties = {
        collisionsWillMove: true,
        dimensions: {
            x: 0.025599999353289604,
            y: 0.057399999350309372,
            z: 0.37419998645782471
        },
        dynamic: true,
        gravity: {
            x: 0,
            y: -9.8,
            z: 0
        },
        velocity: {
            x: 0,
            y: -0.01,
            z: 0
        },
        modelURL: getResourceURL('Models/lighterIceCreamSandwich.fbx'),
        name: 'BrutaneLighter',
        shapeType: 'simple-compound',
        type: 'Model',
        userData: JSON.stringify({
            tag: "equip-temporary",
            grabbableKey: {
                invertSolidWhileHeld: true
            },
            wearable: {
              joints: {
                RightHand: [{
                  x: 0.029085848480463028,
                  y: 0.09807153046131134,
                  z: 0.03062543272972107
                }, {
                  x: 0.5929139256477356,
                  y: 0.3207578659057617,
                  z: 0.7151655554771423,
                  w: -0.18468326330184937
                }],
                LeftHand: [{
                  x: -0.029085848480463028,
                  y: 0.09807153046131134,
                  z: 0.03062543272972107
                }, {
                  x: -0.5929139256477356,
                  y: 0.3207578659057617,
                  z: 0.7151655554771423,
                  w: -0.18468326330184937
                }]
              }
            }
        }),
        script: SCRIPT_URL
    };
    return createEntity(entityProperties, transform);
}

function createFireParticle(butaneLighter) {
    var entityProperties = {
        userData: JSON.stringify({ tag: "equip-temporary" }),
        accelerationSpread: {
            x: 0.1,
            y: 0,
            z: 0.1
        },
        alpha: 0.039999999105930328,
        alphaFinish: 0.039999999105930328,
        alphaStart: 0.039999999105930328,
        azimuthFinish: 0.039999999105930328,
        azimuthStart: 0,
        dimensions: {
            x: 0.49194091558456421,
            y: 0.49194091558456421,
            z: 0.49194091558456421
        },
        emitAcceleration: {
            x: 0,
            y: 0,
            z: 0
        },
        emitOrientation: {
            w: 1,
            x: -1.52587890625e-05,
            y: -1.52587890625e-05,
            z: -1.52587890625e-05
        },
        emitRate: 770,
        emitSpeed: 0.014000000432133675,
        isEmitting: false,
        lifespan: 0.37000000476837158,
        maxParticles: 820,
        name: 'lighter_particle',
        particleRadius: 0.0027000000700354576,
        position: {
            x: -0.00044769048690795898,
            y: 0.016354814171791077,
            z: 0.19217036664485931
        },
        radiusFinish: 0.0027000000700354576,
        radiusSpread: 3,
        radiusStart: 0.0027000000700354576,
        rotation: {
            w: 1,
            x: -0.0001678466796875,
            y: -1.52587890625e-05,
            z: -1.52587890625e-05
        },
        speedSpread: 0.56999999284744263,
        textures: 'atp:/textures/fire3.png',
        type: 'ParticleEffect',


            "color": {
                "red": 255,
                "green": 255,
                "blue": 255
            },
        "isEmitting": 0,
        "maxParticles": 820,
        "lifespan": 0.28,
        "emitRate": 1100,
        "emitSpeed": 0.007,
        "speedSpread": 0.5699999928474426,
        "emitOrientation": {
            "x": -0.0000152587890625,
            "y": -0.0000152587890625,
            "z": -0.0000152587890625,
            "w": 1
        },
        "emitDimensions": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "polarStart": 0,
        "polarFinish": 0,
        "azimuthStart": 0,
        "azimuthFinish": 0.03999999910593033,
        "emitAcceleration": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "accelerationSpread": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "particleRadius": 0.0037,
        "radiusSpread": 3,
        "radiusStart": 0.008,
        "radiusFinish": 0.0004,
        "colorSpread": {
            "red": 0,
            "green": 0,
            "blue": 0
        },
        "colorStart": {
            "red": 255,
            "green": 255,
            "blue": 255
        },
        "colorFinish": {
            "red": 255,
            "green": 255,
            "blue": 255
        },
        "alpha": 0.03999999910593033,
        "alphaSpread": 0,
        "alphaStart": 0.141,
        "alphaFinish": 0.02,
        "emitterShouldTrail": 0,
        "textures": "atp:/textures/fire3.png"
    };
    return createEntity(entityProperties, butaneLighter);
}

doCreateButaneLighter = function(transform) {
    var butaneLighter = createButaneLighter(transform);
    createFireParticle(butaneLighter);
    return butaneLighter;
}
