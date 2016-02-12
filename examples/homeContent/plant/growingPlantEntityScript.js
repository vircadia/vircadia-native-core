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
    };

    GrowingPlant.prototype = {

        createFlowers: function() {
            for (var i = 0; i < 10; i++) {
                _this.createFlower();
            }
        },

        createCactus: function() {
            var MODEL_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/cactus.fbx"
            var dimensions = {
                x: 0.09,
                y: 0.01,
                z: 0.09
            };
            _this.cactus = Entities.addEntity({
                type: "Model",
                modelURL: MODEL_URL,
                position: _this.position,
                dimensions: dimensions
            });

            var curProps = {
                yDimension: 0.01,
                yPosition: _this.position.y
            };

            var endProps = {
                yDimension: 0.8,
                yPosition: _this.position.y + 0.4
            };

            var growTween = new TWEEN.Tween(curProps).
            to(endProps, 2000).
            onUpdate(function() {
                Entities.editEntity(_this.cactus, {
                    dimensions: {
                        x: dimensions.x,
                        y: curProps.yDimension,
                        z: dimensions.z
                    },
                    position: {
                        x: _this.position.x,
                        y: curProps.yPosition,
                        z: _this.position.z
                    }
                });
            }).start();
        },

        createFlower: function() {
            var startingFlowerDimensions = {
                x: 0.2,
                y: 0.001,
                z: 0.2
            };
            var flowerUserData = {
                ProceduralEntity: {
                    shaderUrl: "file:///C:/Users/Eric/hifi/examples/homeContent/plant/flower.fs",
                    uniforms: {
                        iBloomPct: 0.5
                    }
                }
            };
            do {
                var x = randFloat(-_this.dimensions.x/3, this.dimensions.x/3);
                var z = randFloat(-_this.dimensions.z/3, this.dimensions.z/3);
                print("EBL X" + x );
            } while(x < 0.1 && z < 0.1);
            var startingFlowerPosition = Vec3.sum(_this.position, {
                x: x,
                y: _this.dimensions.y / 2 - 0.04,
                z: z
            });
            var flower = Entities.addEntity({
                type: "Sphere",
                position: startingFlowerPosition,
                color: {
                    red: 100,
                    green: 10,
                    blue: 100
                },
                dimensions: startingFlowerDimensions,
                userData: JSON.stringify(flowerUserData)
            });
            _this.flowers.push(flower);

            var curProps = {
                yDimension: startingFlowerDimensions.y,
                yPosition: startingFlowerPosition.y,
                bloomPct: flowerUserData.ProceduralEntity.uniforms.iBloomPct
            };
            var newYDimension = curProps.yDimension + 1;
            var endProps = {
                yDimension: newYDimension,
                yPosition: startingFlowerPosition.y + newYDimension / 2,
                bloomPct: 1
            };

            var bloomTween = new TWEEN.Tween(curProps).
            to(endProps, 3000).
            easing(TWEEN.Easing.Cubic.InOut).
            delay(1000).
            onUpdate(function() {
                // flowerUserData.ProceduralEntity.uniforms.iBloomPct = curProps.bloomPct;
                Entities.editEntity(flower, {
                    dimensions: {
                        x: startingFlowerDimensions.x,
                        y: curProps.yDimension,
                        z: startingFlowerDimensions.z
                    },
                    position: {
                        x: startingFlowerPosition.x,
                        y: curProps.yPosition,
                        z: startingFlowerPosition.z
                    },
                    // userData: JSON.stringify(flowerUserData)
                });
            }).start();
        },

        preload: function(entityID) {
            print("EBL PRELOAD");
            this.entityID = entityID;
            this.props = Entities.getEntityProperties(this.entityID, ["position", "dimensions"]);
            this.position = this.props.position;
            this.dimensions = this.props.dimensions;
            this.createCactus();
            this.createFlowers();
            // this.createFlower();
            Script.update.connect(_this.update);
        },

        update: function() {
            TWEEN.update();
        },

        unload: function() {
            Script.update.disconnect(_this.update);
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