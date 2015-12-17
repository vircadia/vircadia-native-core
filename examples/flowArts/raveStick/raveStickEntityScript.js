//  raveStickEntityScript.js
//  
//  Script Type: Entity
//  Created by Eric Levin on 12/16/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This entity script create light trails on a given object as it moves.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include("../../libraries/utils.js");
    var _this;
    // this is the "constructor" for the entity as a JS object we don't do much here
    var RaveStick = function() {
        _this = this;
    };

    RaveStick.prototype = {
        isGrabbed: false,

        startNearGrab: function() {
            // this.createBeam();
        },

        continueNearGrab: function() {

        },

        releaseGrab: function() {

        },

        preload: function(entityID) {
            this.entityID = entityID;
            this.createBeam();
        },

        unload: function() {
            Entities.deleteEntity(this.beam);
        },

        createBeam: function() {

            var props = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);
            var forwardVec = Quat.getFront(Quat.multiply(props.rotation, Quat.fromPitchYawRollDegrees(-90, 0, 0)));
            forwardVec = Vec3.normalize(forwardVec);
            var forwardQuat = orientationOf(forwardVec);
            var position = Vec3.sum(props.position, Vec3.multiply(Quat.getFront(props.rotation), 0.1));
            position.z += 0.1;
            position.x += -0.035;
            var props = {
                type: "ParticleEffect",
                position: position,
                parentID: this.entityID,
                isEmitting: true,
                "name": "ParticlesTest Emitter",
                "colorStart": {
                    red: 0,
                    green: 200,
                    blue: 40
                },
                color: {
                    red: 200,
                    green: 200,
                    blue: 255
                },
                "colorFinish": {
                    red: 25,
                    green: 200,
                    blue: 5
                },
                "maxParticles": 100000,
                "lifespan": 2,
                "emitRate": 1000,
                emitOrientation: forwardQuat,
                "emitSpeed": .4,
                "speedSpread": 0.0,
                // "emitDimensions": {
                //     "x": .1,
                //     "y": .1,
                //     "z": .1
                // },
                "polarStart": 0,
                "polarFinish": .0,
                "azimuthStart": .1,
                "azimuthFinish": .01,
                "emitAcceleration": {
                    "x": 0,
                    "y": 0,
                    "z": 0
                },
                "accelerationSpread": {
                    "x": .00,
                    "y": .00,
                    "z": .00
                },
                "radiusStart": 0.03,
                radiusFinish: 0.025,
                "alpha": 0.7,
                "alphaSpread": .1,
                "alphaStart": 0.5,
                "alphaFinish": 0.5,
                // "textures": "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
                "textures": "file:///C:/Users/Eric/Desktop/beamParticle.png?v1" + Math.random(),
                emitterShouldTrail: false
            }
            this.beam = Entities.addEntity(props);

        }
    };
    // entity scripts always need to return a newly constructed object of our type
    return new RaveStick();
});