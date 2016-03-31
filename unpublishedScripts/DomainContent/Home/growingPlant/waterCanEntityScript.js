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

    Script.include('../utils.js');

    var _this;

    function WaterSpout() {
        _this = this;
        _this.waterSound = SoundCache.getSound("atp:/growingPlant/watering_can_pour.L.wav");
        _this.POUR_ANGLE_THRESHOLD = -0.1;
        _this.waterPouring = false;
        _this.WATER_SPOUT_NAME = "home_box_waterSpout";
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
            print("EBL START HOLD")
            var entities = Entities.findEntities(_this.position, 2);
            print("EBL SEARCHING FOR SPOUT");
            entities.forEach(function(entity) {
                var name = Entities.getEntityProperties(entity, "name").name;
                if (name === _this.WATER_SPOUT_NAME) {
                    print("EBL FOUND SPOUT");
                    _this.waterSpout = entity;
                    _this.waterSpoutPosition = Entities.getEntityProperties(_this.waterSpout, "position").position;
                    _this.waterSpoutRotation = Entities.getEntityProperties(_this.waterSpout, "rotation").rotation;
                    _this.createWaterEffect();
                }
            });

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
            var waterEffectToDelete = _this.waterEffect;
            Script.setTimeout(function() {
                Entities.deleteEntity(waterEffectToDelete);
            }, 2000);
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
            var rotation = Entities.getEntityProperties(_this.entityID, "rotation").rotation;
            var forwardVec = Quat.getFront(rotation);
            if (forwardVec.y < _this.POUR_ANGLE_THRESHOLD) {
                // Water is pouring
                _this.spoutProps= Entities.getEntityProperties(_this.waterSpout, ["rotation", "position"]);
                _this.castRay();
                if (!_this.waterPouring) {
                    _this.startPouring();
                }
                _this.waterSpoutRotation = _this.spoutProps.rotation;
                var waterEmitOrientation = Quat.multiply(_this.waterSpoutRotation, Quat.fromPitchYawRollDegrees(0, 180, 0));
                Entities.editEntity(_this.waterEffect, {
                    emitOrientation: waterEmitOrientation
                });
            } else if (forwardVec.y > _this.POUR_ANGLE_THRESHOLD && _this.waterPouring) {
                _this.stopPouring();
            }
        },


        stopPouring: function() {
            print("EBL STOP POURING")
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

        startPouring: function() {
            print("EBL START POURING")                
            Script.setTimeout(function() {
                Entities.editEntity(_this.waterEffect, {
                    isEmitting: true
                });
            }, 100);
            _this.waterPouring = true;
            if (!_this.waterInjector) {
                _this.waterInjector = Audio.playSound(_this.waterSound, {
                    position: _this.spoutProps.position,
                    loop: true
                });

            } else {
                _this.waterInjector.restart();
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
                lifetime: 5000, //Doubtful anyone will hold water can longer than this
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
                textures: "atp:/growingPlant/raindrop.png",
                userData: JSON.stringify({
                    'hifiHomeKey': {
                        'reset': true
                    }
                }),
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
            print("EBL PRELOADING WATER CAN")
            _this.entityID = entityID;
            _this.position = Entities.getEntityProperties(_this.entityID, "position").position;
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