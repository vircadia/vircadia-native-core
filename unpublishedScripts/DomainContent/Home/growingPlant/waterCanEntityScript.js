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

    Script.include('../../../../libraries/utils.js');

    var _this;
    function WaterSpout() {
        _this = this;
        _this.waterSound = SoundCache.getSound("https://s3-us-west-1.amazonaws.com/hifi-content/eric/Sounds/shower.wav");
        _this.POUR_ANGLE_THRESHOLD = 0;
        _this.waterPouring = false;
        _this.WATER_SPOUT_NAME = "hifi-water-spout";
        _this.GROWABLE_ENTITIES_SEARCH_RANGE = 100;

    };

    WaterSpout.prototype = {

        startNearGrab: function() {
            _this.startHold();
        },

        startEquip: function() {
            _this.startHold();
        },

        startHold: function() {
            _this.findGrowableEntities();
        },

        releaseEquip: function() {
            _this.releaseHold();
        },

        releaseGrab: function() {
            _this.releaseHold();
        },

        releaseHold: function() {
            _this.stopPouring();
        },

        stopPouring: function() {
            Entities.editEntity(_this.waterEffect, {
                isEmitting: false
            });
            _this.waterPouring = false;
            //water no longer pouring...
            if (_this.waterInjector) {
              _this.waterInjector.stop();  
            }
            Entities.callEntityMethod(_this.mostRecentIntersectedGrowableEntity, 'stopWatering');
        },
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
                // Water is pouring
                var spoutProps = Entities.getEntityProperties(_this.waterSpout, ["rotation", "position"]);
                if (!_this.waterPouring) {
                    Entities.editEntity(_this.waterEffect, {
                        isEmitting: true
                    });
                    _this.waterPouring = true;
                    if (!_this.waterInjector) {
                        _this.waterInjector = Audio.playSound(_this.waterSound, {
                            position: spoutProps.position,
                            loop: true
                        });

                    } else {
                        _this.waterInjector.restart();
                    }
                }
                _this.waterSpoutRotation = spoutProps.rotation;
                var waterEmitOrientation = Quat.multiply(_this.waterSpoutRotation, Quat.fromPitchYawRollDegrees(0, 180, 0));
                Entities.editEntity(_this.waterEffect, {
                    emitOrientation: waterEmitOrientation
                });
            } else if (pitch > _this.POUR_ANGLE_THRESHOLD && _this.waterPouring) {
                _this.stopPouring();
            }
        },

        castRay: function() {
            var spoutProps = Entities.getEntityProperties(_this.waterSpout, ["position, rotation"]);
            var direction = Quat.getFront(spoutProps.rotation)
            var end = Vec3.sum(spoutProps.position, Vec3.multiply(5, direction));

            var pickRay = {
                origin: spoutProps.position,
                direction: direction
            };
            var intersection = Entities.findRayIntersection(pickRay, true, _this.growableEntities);

            if (intersection.intersects) {
                //We've intersected with a waterable object
                var data = JSON.stringify({
                    position: intersection.intersection,
                    surfaceNormal: intersection.surfaceNormal
                });
                _this.mostRecentIntersectedGrowableEntity = intersection.entityID;
                Entities.callEntityMethod(intersection.entityID, 'continueWatering', [data]);
            }

        },



        createWaterEffect: function() {
            var waterEffectPosition = Vec3.sum(_this.waterSpoutPosition, Vec3.multiply(Quat.getFront(_this.waterSpoutRotation), -0.04));
            _this.waterEffect = Entities.addEntity({
                type: "ParticleEffect",
                name: "water particle effect",
                position: waterEffectPosition,
                isEmitting: false,
                parentID: _this.waterSpout,
                colorStart: {
                    red: 90,
                    green: 90,
                    blue: 110
                },
                color: {
                    red: 70,
                    green: 70,
                    blue: 130
                },
                colorFinish: {
                    red: 23,
                    green: 195,
                    blue: 206
                },
                maxParticles: 20000,
                lifespan: 2,
                emitRate: 2000,
                emitSpeed: .3,
                speedSpread: 0.1,
                emitDimensions: {
                    x: 0.0,
                    y: 0.0,
                    z: 0.0
                },
                emitAcceleration: {
                    x: 0.0,
                    y: 0,
                    z: 0
                },
                polarStart: 0.0,
                polarFinish: 0.1,
                accelerationSpread: {
                    x: 0.01,
                    y: 0.0,
                    z: 0.01
                },
                emitOrientation: Quat.fromPitchYawRollDegrees(0, 0, 0),
                radiusSpread: 0.0001,
                radiusStart: 0.005,
                particleRadius: 0.003,
                radiusFinish: 0.001,
                alphaSpread: 0,
                alphaStart: 0.1,
                alpha: 1.0,
                alphaFinish: 1.0,
                emitterShouldTrail: true,
                textures: "https://s3-us-west-1.amazonaws.com/hifi-content/eric/images/raindrop.png",
            });

        },

        findGrowableEntities: function() {
            _this.growableEntities = [];
            var entities = Entities.findEntities(_this.position, _this.GROWABLE_ENTITIES_SEARCH_RANGE);
            entities.forEach(function(entity) {
                var name = Entities.getEntityProperties(entity, "name").name;
                if (name.length > 0 && name.indexOf("growable") !== -1) {
                    _this.growableEntities.push(entity);
                }
            });

        },

        preload: function(entityID) {
            _this.entityID = entityID;
            _this.position = Entities.getEntityProperties(_this.entityID, "position").position;
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
            Entities.deleteEntity(_this.waterEffect);
            if (_this.waterInjector) {
                _this.waterInjector.stop();
                delete _this.waterInjector;
            }
        }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new WaterSpout();
});