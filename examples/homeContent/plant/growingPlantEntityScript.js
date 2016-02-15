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
    Script.include("../../libraries/tween.js");
    Script.include("../../libraries/utils.js");

    var _this;
    var TWEEN = loadTween();
    GrowingPlant = function() {
        _this = this;
        _this.flowers = [];
        _this.delay = 1000;
        _this.MAX_CACTUS_Y_DIMENSION = 0.7;
        _this.GROW_RATE = 0.001;
    };

    GrowingPlant.prototype = {


        water: function() {
            print("EBL IM BEING WATERED!");
            _this.cactusDimensions = Vec3.sum(_this.cactusDimensions, {x: 0, y: _this.GROW_RATE, z: 0});

            if (_this.cactusDimensions.y > _this.MAX_CACTUS_Y_DIMENSION) {
                // We don't want to grow our cactus any more than this or it will look bad
                return;
            }
            // Need to raise up cactus as it stretches so it doesnt burst out the bottom of the plant
            _this.cactusPosition = Vec3.sum(_this.cactusPosition, {x: 0, y: _this.cactusHeightMovement * 0.1, z: 0});
            Entities.editEntity(_this.cactus, {dimensions: _this.cactusDimensions, position: _this.cactusPosition});

            _this.flowers.forEach(_this.waterFlower);
        },

        waterFlower: function(flower) {
            var props = Entities.getEntityProperties(flower, ["position, dimensions"]);
            var newDimensions = Vec3.sum(props.dimensions, {x: randFloat(0, 0.0001), y: 0.001, z: randFloat(0, 0.0001)});
            var newPosition = Vec3.sum(props.position, {x: 0, y: _this.flowerHeightMovement * 0.55, z: 0});
            Entities.editEntity(flower, {dimensions: newDimensions, position: newPosition});
        },

        createFlowers: function() {
            var size = 0.1;
            var NUM_FLOWERS = 20
              _this.startingFlowerDimensions = {
                x: size,
                y: 0.001,
                z: size
            };
            _this.flowerHeightMovement = _this.startingFlowerDimensions.y;
            for (var i = 0; i < NUM_FLOWERS; i++) {
                var segment = i / NUM_FLOWERS * Math.PI * 2;
                var radius = randFloat(0.13, 0.25);
                var position = Vec3.sum(_this.position, {
                    x: radius * Math.cos(segment),
                    y: 0.16,
                    z: radius * Math.sin(segment)
                });
                _this.createFlower(position);
            }
        },

        createFlower: function(position) {
          
            var flowerUserData = {
                ProceduralEntity: {
                    shaderUrl: "file:///C:/Users/Eric/hifi/examples/homeContent/plant/flower.fs",
                    uniforms: {
                        iBloomPct: randFloat(0.4, 0.8),
                        hueTwerking: randFloat(10, 30)
                    }
                }
            };
            var flower = Entities.addEntity({
                type: "Sphere",
                position: position,
                dimensions: _this.startingFlowerDimensions,
                userData: JSON.stringify(flowerUserData)
            });
            _this.flowers.push(flower);
     
        },

        createCactus: function() {
            var MODEL_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/cactus.fbx"
            _this.cactusDimensions = {
                x: 0.09,
                y: 0.01,
                z: 0.09
            };
            _this.cactusHeightMovement = _this.cactusDimensions.y/2;
            _this.cactusPosition = _this.position;
            _this.cactus = Entities.addEntity({
                type: "Model",
                modelURL: MODEL_URL,
                position: _this.cactusPosition,
                dimensions: _this.cactusDimensions
            });     
        },

        preload: function(entityID) {
            print("EBL PRELOAD");
            this.entityID = entityID;
            this.props = Entities.getEntityProperties(this.entityID, ["position", "dimensions"]);
            this.position = this.props.position;
            this.dimensions = this.props.dimensions;
            this.createCactus();
            this.createFlowers();
        },

    

        unload: function() {
            Entities.deleteEntity(_this.cactus);
            _this.flowers.forEach(function(flower) {
                Entities.deleteEntity(flower);
            });
            print("EBL UNLOAD DONE")
        }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new GrowingPlant();
});