//
//  waterCanEntityScript.js
//  examples/homeContent/plant
//
//  Created by Eric Levin on 2/15/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This entity script handles the logic for pouring water when a user tilts the entity the script is attached too.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


(function() {

    Script.include("../../libraries/utils.js");

    var _this;
    WaterSpout = function() {
        _this = this;
        _this.waterSound = SoundCache.getSound("https://s3-us-west-1.amazonaws.com/hifi-content/eric/Sounds/shower.wav");
        _this.POUR_ANGLE_THRESHOLD = -10;
        _this.waterPouring = false;
        _this.WATER_SPOUT_NAME = "hifi-water-spout";

    };

    WaterSpout.prototype = {
        continueEquip: function() {
            _this.continueHolding();
        },

        continueNearGrab: function() {
            _this.continueHolding();
        },

        continueHolding: function() {
            if (!_this.waterSpout) {
                return;
            }
            // Check rotation of water can along it's z axis. If it's beyond a threshold, then start spraying water
            _this.castRay();
            var rotation = Entities.getEntityProperties(_this.entityID, "rotation").rotation;
            var pitch = Quat.safeEulerAngles(rotation).x;
            if (pitch < _this.POUR_ANGLE_THRESHOLD) {
                if (!_this.waterPouring) {
                    Entities.editEntity(_this.waterEffect, {
                        isEmitting: true
                    });
                    _this.waterPouring = true;
                }
                _this.waterSpoutRotation = Entities.getEntityProperties(_this.waterSpout, "rotation").rotation;
                var waterEmitOrientation = Quat.multiply(_this.waterSpoutRotation, Quat.fromPitchYawRollDegrees(0, 180, 0));
                Entities.editEntity(_this.waterEffect, {
                    emitOrientation: waterEmitOrientation
                });
            } else if (pitch > _this.POUR_ANGLE_THRESHOLD && _this.waterPouring) {
                Entities.editEntity(_this.waterEffect, {
                    isEmitting: false
                });
                _this.waterPouring = false;
            }
        },

        castRay: function() { 
            var spoutProps = Entities.getEntityProperties(_this.waterSpout, ["position, rotation"]);
            var direction =  Quat.getFront(spoutProps.rotation)
            var end = Vec3.sum(spoutProps.position, Vec3.multiply(5, direction));
            Overlays.editOverlay(_this.rayCastLine, {
                start: spoutProps.position,
                end: end
            });

            var pickRay = {
                origin: spoutProps.position,
                direction: direction
            };
            var intersection = Entities.findRayIntersection(pickRay, true, _this.growableEntities);

            if (intersection.intersects) {
                //We've intersected with a waterable object
                var data = JSON.stringify({position: intersection.intersection, surfaceNormal: intersection.surfaceNormal});
                Entities.callEntityMethod(intersection.entityID, 'continueWatering', [data]);
            }

        },



        createWaterEffect: function() {
            _this.waterEffect = Entities.addEntity({
                type: "ParticleEffect",
                name: "water particle effect",
                position: _this.waterSpoutPosition,
                isEmitting: false,
                parentID: _this.waterSpout,
                colorStart: {
                    red: 50,
                    green: 50,
                    blue: 70
                },
                color: {
                    red: 30,
                    green: 30,
                    blue: 40
                },
                colorFinish: {
                    red: 50,
                    green: 50,
                    blue: 60
                },
                maxParticles: 20000,
                lifespan: 2,
                emitRate: 1000,
                emitSpeed: .2,
                speedSpread: 0.0,
                emitDimensions: {
                    x: 0,
                    y: 0,
                    z: 0
                },
                emitAcceleration: {
                    x: 0.0,
                    y: 0,
                    z: 0
                },
                polarStart: 0,
                polarFinish: .2,
                accelerationSpread: {
                    x: 0.01,
                    y: 0.0,
                    z: 0.01
                },
                emitOrientation: Quat.fromPitchYawRollDegrees(0, 180, 0),
                particleRadius: 0.01,
                radiusSpread: 0.001,
                radiusStart: 0.01,
                radiusFinish: 0.01,
                alpha: 0.9,
                alphaSpread: .1,
                alphaStart: 0.7,
                alphaFinish: 0.5,
                emitterShouldTrail: true,
                textures: "https://s3-us-west-1.amazonaws.com/hifi-content/eric/images/raindrop.png?v2",
            });

        },

        findGrowableEntities: function() {
            _this.growableEntities = [];
            var entities = Entities.findEntities(_this.position, 50);
            entities.forEach( function(entity) {
                var name = Entities.getEntityProperties(entity, "name").name;
                if (name.length > 0 && name.indexOf("growable") !== -1) {
                    _this.growableEntities.push(entity);
                }
            });

        },

        preload: function(entityID) {
            _this.entityID = entityID;
            _this.position = Entities.getEntityProperties(_this.entityID, "position").position;
            _this.rayCastLine = Overlays.addOverlay("line3d", {
                color: {
                    red: 200,
                    green: 10,
                    blue: 200
                },
                lineWidth: 2,
                alpha: 1,
                visible: true,
                ignoreRayIntersection: true
            });
            _this.findGrowableEntities();
            // Wait a a bit for spout to spawn for case where preload is initial spawn, then save it 
            Script.setTimeout(function() {
                var entities = Entities.findEntities(_this.position, 1);
                entities.forEach(function(entity) {
                    var name = Entities.getEntityProperties(entity, "name").name;
                    if (name === _this.WATER_SPOUT_NAME) {
                        _this.waterSpout = entity;
                    }
                });

                if (_this.waterSpout) {
                    _this.waterSpoutPosition = Entities.getEntityProperties(_this.waterSpout, "position").position;
                    _this.waterSpoutRotation = Entities.getEntityProperties(_this.waterSpout, "rotation").rotation;
                    _this.createWaterEffect();
                }

            }, 3000);

        },


        unload: function() {
            Overlays.deleteOverlay(_this.rayCastLine);
            Entities.deleteEntity(_this.waterEffect);
        }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new WaterSpout();
});