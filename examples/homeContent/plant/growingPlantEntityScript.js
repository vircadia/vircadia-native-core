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

        _this.debounceRange = {min: 500, max: 1000};
        _this.canCreateFlower = true;

    };

    GrowingPlant.prototype = {


        continueWatering: function(entityID, data) {
            // we're being watered- every now and then spawn a new flower to add to our growing list
            // If we don't have any flowers yet, immediately grow one
            var data = JSON.parse(data[0]);

            if(_this.canCreateFlower) {
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
            var flowerRotation = Quat.rotationBetween(Vec3.UNIT_Y, surfaceNormal);
            var flowerEntityID = Entities.addEntity({
                type: "Sphere",
                name: "flower",
                position: position,
                collisionless: true,
                rotation: flowerRotation,
                dimensions: _this.STARTING_FLOWER_DIMENSIONS,
                userData: JSON.stringify(_this.flowerUserData)
            });

            var flower = {
                id: flowerEntityID,
                dimensions: {x: _this.STARTING_FLOWER_DIMENSIONS.x, y: _this.STARTING_FLOWER_DIMENSIONS.y, z: _this.STARTING_FLOWER_DIMENSIONS.z},
                startingPosition: position,
                rotation: flowerRotation,
                maxYDimension: randFloat(0.2, 1.5),
                growthRate: {x: 0.0002, y: 0.001, z: 0.0002}
            };
            print(_this.STARTING_FLOWER_DIMENSIONS.y)
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
        },

        preload: function(entityID) {
            _this.entityID = entityID;
            _this.flowerUserData = {
                ProceduralEntity: {
                    shaderUrl: "https://s3-us-west-1.amazonaws.com/hifi-content/eric/shaders/flower.fs",
                    uniforms: {
                        iBloomPct: randFloat(0.4, 0.8),
                        hueTwerking: randFloat(10, 30)
                    }
                }
            };
        },

        unload: function() {
            _this.flowers.forEach(function(flower) {
                Entities.deleteEntity(flower.id);
            })


        }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new GrowingPlant();
});