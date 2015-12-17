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
    var LIFETIME = 6000;
    var DRAWING_DEPTH = 0.8;
    var LINE_DIMENSIONS = 100;
    var MAX_POINTS_PER_LINE = 50;
    var MIN_POINT_DISTANCE = 0.02;
    var RaveStick = function() {
        _this = this;
        this.colorPalette = [{
            red: 0,
            green: 200,
            blue: 40
        }, {
            red: 200,
            green: 10,
            blue: 40
        }];
        var texture = "https://s3.amazonaws.com/hifi-public/eric/textures/paintStrokes/trails.png";
        this.trail = Entities.addEntity({
            type: "PolyLine",
            dimensions: {x: LINE_DIMENSIONS, y: LINE_DIMENSIONS, z: LINE_DIMENSIONS},
            color: {
                red: 255,
                green: 255,
                blue: 255
            },
            textures: texture,
            lifetime: LIFETIME
        })
        this.points = [];
        this.normals = [];
        this.strokeWidths = [];
    };

    RaveStick.prototype = {
        isGrabbed: false,

        startNearGrab: function() {
            // this.createBeam();
            this.trailBasePosition = Entities.getEntityProperties(this.entityID, "position").position;
            Entities.editEntity(this.trail, {
                position: this.trailBasePosition
            });
            this.points = [];
            this.normals = [];
            this.strokeWidths = [];
        },

        continueNearGrab: function() {
            var position = Entities.getEntityProperties(this.entityID, "position").position;
            var localPoint = Vec3.subtract(position, this.trailBasePosition);
            if (this.points.length >=1 && Vec3.distance(localPoint, this.points[this.points.length - 1]) < MIN_POINT_DISTANCE) {
                //Need a minimum distance to avoid binormal NANs
                return;
            }
            this.points.push(localPoint);
            var normal = computeNormal(position, Camera.getPosition());
            this.normals.push(normal);
            this.strokeWidths.push(0.04);
            Entities.editEntity(this.trail, {
                linePoints: this.points,
                normals: this.normals,
                strokeWidths: this.strokeWidths
            })

        },

        releaseGrab: function() {

        },

        preload: function(entityID) {
            this.entityID = entityID;
            this.createBeam();
        },

        unload: function() {
            Entities.deleteEntity(this.beam);
            Entities.deleteEntity(this.trail);
            // Entities.deleteEntity(this.beamTrail);
        },

        createBeam: function() {

            var props = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);
            var forwardVec = Quat.getFront(Quat.multiply(props.rotation, Quat.fromPitchYawRollDegrees(-90, 0, 0)));
            forwardVec = Vec3.normalize(forwardVec);
            var forwardQuat = orientationOf(forwardVec);
            var position = Vec3.sum(props.position, Vec3.multiply(Quat.getFront(props.rotation), 0.1));
            position.z += 0.1;
            position.x += -0.035;
            var color = this.colorPalette[randInt(0, this.colorPalette.length)];
            var props = {
                type: "ParticleEffect",
                position: position,
                parentID: this.entityID,
                isEmitting: true,
                "name": "ParticlesTest Emitter",
                "colorStart": color,
                color: {
                    red: 200,
                    green: 200,
                    blue: 255
                },
                "colorFinish": color,
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

            // props.emitterShouldTrail = true;
            // this.beamTrail = Entities.addEntity(props);

        }
    };
    // entity scripts always need to return a newly constructed object of our type
    return new RaveStick();

    function computeNormal(p1, p2) {
        return Vec3.normalize(Vec3.subtract(p2, p1));
    }
});