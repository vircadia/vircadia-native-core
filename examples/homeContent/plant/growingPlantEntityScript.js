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

    var _this;
    var TWEEN = loadTween();
    GrowingPlant = function() {
        _this = this;
        _this.flowers = [];
    };

    GrowingPlant.prototype = {

        createFlowers: function() {
           for(var i = 0; i < 3; i++) {
              _this.createFlower();
           }
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
                        iBloomPct: 0.0
                    }
                }
            };
            var startingFlowerPosition = Vec3.sum(_this.position, {
                x: Math.random(),
                y: _this.dimensions.y / 2,
                z: 0
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
                bloomPct:  flowerUserData.ProceduralEntity.uniforms.iBloomPct
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
                flowerUserData.ProceduralEntity.uniforms.iBloomPct = curProps.bloomPct;
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
                    userData: JSON.stringify(flowerUserData)
                });
            }).start();
        },

        preload: function(entityID) {
            print("EBL PRELOAD");
            this.entityID = entityID;
            this.props = Entities.getEntityProperties(this.entityID, ["position", "dimensions"]);
            this.position = this.props.position;
            this.dimensions = this.props.dimensions;
            this.createFlowers();
            Script.update.connect(_this.update);
        },

        update: function() {
            TWEEN.update();
        },

        unload: function() {
            Script.update.disconnect(_this.update);
            _this.flowers.forEach(function(flower) {
                Entities.deleteEntity(flower);
            });
            print("EBL UNLOAD DONE")
        }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new GrowingPlant();
});