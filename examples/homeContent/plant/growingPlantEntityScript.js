//
//  growingPlantEntityScript.js
//  examples/homeContent/plant
//
//  Created by Eric Levin on 2/10/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This entity script handles the logic for growing a plant when it has water poured on it
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html



(function() {
    Script.include("../../libraries/utils.js");

    var _this;
    GrowingPlant = function() {
        _this = this;
        _this.flowers = [];
        // _this.STARTING_FLOWER_DIMENSIONS = {x: 0.1, y: 0.001, z: 0.1}
        _this.STARTING_FLOWER_DIMENSIONS = {
            x: 0.001,
            y: 0.001,
            z: 0.001
        }

        _this.MAX_FLOWERS = 50;
        _this.MIN_FLOWER_TO_FLOWER_DISTANCE = 0.03;

        _this.debounceRange = {
            min: 500,
            max: 1000
        };
        _this.canCreateFlower = true;
        _this.flowersBloomingSound = SoundCache.getSound("https://s3-us-west-1.amazonaws.com/hifi-content/eric/Sounds/flowersBlooming.wav");
        _this.soundPlaying = false;
        _this.BLOOM_VOLUME = 0.4;

    };

    GrowingPlant.prototype = {


        continueWatering: function(entityID, data) {
            // we're being watered- every now and then spawn a new flower to add to our growing list
            // If we don't have any flowers yet, immediately grow one
            var data = JSON.parse(data[0]);

            if (_this.canCreateFlower && _this.flowers.length < _this.MAX_FLOWERS) {
                _this.createFlower(data.position, data.surfaceNormal);
                _this.canCreateFlower = false;
                Script.setTimeout(function() {
                    _this.canCreateFlower = true;
                }, randFloat(_this.debounceRange.min, this.debounceRange.max));

            }

            _this.flowers.forEach(function(flower) {
                flower.grow();
            });

            if (!_this.soundPlaying) {
                _this.position = Entities.getEntityProperties(_this.entityID, "position").position;
                _this.soundPlaying = true;
                _this.bloomSoundInjector = Audio.playSound(_this.flowersBloomingSound, {position: _this.position, volume: _this.BLOOM_VOLUME});
            }
        },

        stopWatering: function() {
           _this.bloomSoundInjector.stop();
           _this.soundPlaying = false;
        },

        createFlower: function(position, surfaceNormal) {
            if (_this.previousFlowerPosition && Vec3.distance(position, _this.previousFlowerPosition) < _this.MIN_FLOWER_TO_FLOWER_DISTANCE) {
                // Reduces flower overlap
                return;
            }
            var flowerRotation = Quat.rotationBetween(Vec3.UNIT_Y, surfaceNormal);
            _this.flowerUserData.ProceduralEntity.uniforms.hueAngleRange = randFloat(20, 40);
            _this.flowerUserData.ProceduralEntity.uniforms.hueOffset = Math.random();
            var flowerEntityID = Entities.addEntity({
                type: "Sphere",
                name: "flower",
                position: position,
                collisionless: true,
                rotation: flowerRotation,
                dimensions: _this.STARTING_FLOWER_DIMENSIONS,
                userData: JSON.stringify(_this.flowerUserData)
            });
            var xzGrowthRate = randFloat(0.00005, 0.00015);
            var flower = {
                id: flowerEntityID,
                dimensions: {
                    x: _this.STARTING_FLOWER_DIMENSIONS.x,
                    y: _this.STARTING_FLOWER_DIMENSIONS.y,
                    z: _this.STARTING_FLOWER_DIMENSIONS.z
                },
                startingPosition: position,
                rotation: flowerRotation,
                maxYDimension: randFloat(0.4, 1.0),
                growthRate: {
                    x: xzGrowthRate,
                    y: randFloat(0.001, 0.0025),
                    z: xzGrowthRate
                }
            };
            flower.grow = function() {
                // grow flower a bit
                if (flower.dimensions.y > flower.maxYDimension) {
                    return;
                }
                flower.dimensions = Vec3.sum(flower.dimensions, flower.growthRate);
                flower.position = Vec3.sum(flower.startingPosition, Vec3.multiply(Quat.getUp(flower.rotation), flower.dimensions.y / 2));
                //As we grow we must also move ourselves in direction we grow!
                Entities.editEntity(flower.id, {
                    dimensions: flower.dimensions,
                    position: flower.position
                });
            }
            _this.flowers.push(flower);
            _this.previousFlowerPosition = position;
        },

        preload: function(entityID) {
            _this.entityID = entityID;
            var SHADER_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/shaders/flower.fs?v1";
            _this.flowerUserData = {
                ProceduralEntity: {
                    shaderUrl: SHADER_URL,
                    uniforms: {
                        iBloomPct: randFloat(0.4, 0.8),
                        hueTwerking: Math.random()
                    }
                }
            };
        },

        unload: function() {
            if (_this.bloomSoundInjector) {
                _this.bloomSoundInjector.stop();
                delete _this.bloomSoundInjector;
            }
        }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new GrowingPlant();
});