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
    Script.include('../../../../libraries/utils.js');

    var _this;

    function GrowingPlant() {
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
        _this.SHADER_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/shaders/flower.fs";
        // _this.SHADER_URL = "file:///C:/Users/Eric/hifi/unpublishedScripts/DomainContent/Home/plant/flower.fs";

        _this.flowerHSLColors = [{
            hue: 19 / 360,
            saturation: 0.92,
            light: 0.31
        }, {
            hue: 161 / 360,
            saturation: 0.28,
            light: 0.62
        }];

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


        },

        createFlower: function(position, surfaceNormal) {
            if (_this.previousFlowerPosition && Vec3.distance(position, _this.previousFlowerPosition) < _this.MIN_FLOWER_TO_FLOWER_DISTANCE) {
                // Reduces flower overlap
                return;
            }
            var xzGrowthRate = randFloat(0.00006, 0.00016);
            var growthRate = {
                x: xzGrowthRate,
                y: randFloat(0.001, 0.003),
                z: xzGrowthRate
            };

            var flower = {
                dimensions: {
                    x: _this.STARTING_FLOWER_DIMENSIONS.x,
                    y: _this.STARTING_FLOWER_DIMENSIONS.y,
                    z: _this.STARTING_FLOWER_DIMENSIONS.z
                },
                startingPosition: position,
                rotation: Quat.rotationBetween(Vec3.UNIT_Y, surfaceNormal),
                maxYDimension: randFloat(0.4, 1.1),
                // startingHSLColor: {
                //     hue: 80 / 360,
                //     saturation: 0.47,
                //     light: 0.48
                // },
                // endingHSLColor: {
                //     hue: 19 / 260,
                //     saturation: 0.92,
                //     light: 0.41
                // },
                hslColor: Math.random() < 0.5 ? _this.flowerHSLColors[0] : _this.flowerHSLColors[1],
                growthRate: growthRate
            };
            flower.userData = {
                ProceduralEntity: {
                    shaderUrl: _this.SHADER_URL,
                    uniforms: {
                        iBloomPct: randFloat(0.4, 0.8),
                        iHSLColor: [flower.hslColor.hue, flower.hslColor.saturation, flower.hslColor.light]
                    }
                }
            };
            flower.id = Entities.addEntity({
                type: "Sphere",
                name: "flower",
                lifetime: 3600,
                position: position,
                collisionless: true,
                rotation: flower.rotation,
                dimensions: _this.STARTING_FLOWER_DIMENSIONS,
                userData: JSON.stringify(flower.userData)
            });
            flower.grow = function() {
                // grow flower a bit
                if (flower.dimensions.y > flower.maxYDimension) {
                    return;
                }
                flower.dimensions = Vec3.sum(flower.dimensions, flower.growthRate);
                flower.position = Vec3.sum(flower.startingPosition, Vec3.multiply(Quat.getUp(flower.rotation), flower.dimensions.y / 2));
                //As we grow we must also move ourselves in direction we grow!
                //TODO: Add this color changing back once we fix bug https://app.asana.com/0/inbox/31759584831096/96943843100173/98022172055918
                // var newHue = map(flower.dimensions.y, _this.STARTING_FLOWER_DIMENSIONS.y, flower.maxYDimension, flower.startingHSLColor.hue, flower.endingHSLColor.hue);
                // var newSaturation = map(flower.dimensions.y, _this.STARTING_FLOWER_DIMENSIONS.y, flower.maxYDimension, flower.startingHSLColor.saturation, flower.endingHSLColor.saturation);
                // var newLight = map(flower.dimensions.y, _this.STARTING_FLOWER_DIMENSIONS.y, flower.maxYDimension, flower.startingHSLColor.light, flower.endingHSLColor.light);
                // flower.userData.PrsoceduralEntity.uniforms.iHSLColor = [newHue, newSaturation, newLight];
                Entities.editEntity(flower.id, {
                    dimensions: flower.dimensions,
                    position: flower.position,
                });
            }
            _this.flowers.push(flower);
            _this.previousFlowerPosition = position;
        },

        preload: function(entityID) {
            _this.entityID = entityID;
        },

    };

    // entity scripts always need to return a newly constructed object of our type
    return new GrowingPlant();
});