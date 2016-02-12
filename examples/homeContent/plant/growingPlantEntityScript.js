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
        _this.startingFlowerDimensions = {
            x: 0.6,
            y: 0.001,
            z: 0.6
        };
    };

    GrowingPlant.prototype = {

        createFlower: function() {
            var userData = JSON.stringify({
                ProceduralEntity: {
                    shaderUrl: "file:///C:/Users/Eric/hifi/examples/homeContent/plant/flower.fs",
                    // shaderUrl: "file:///C:/Users/Eric/hifi/examples/shaders/shaderToyWrapper.fs",
                }
            });
            _this.startingFlowerPosition = Vec3.sum(this.position, {
                x: 0,
                y: this.dimensions.y/2,
                z: 0
            });
            _this.flower = Entities.addEntity({
                type: "Sphere",
                position: _this.startingFlowerPosition,
                color: {
                    red: 100,
                    green: 10,
                    blue: 100
                },
                dimensions: _this.startingFlowerDimensions,
                userData: userData
            });

            var curProps = {
                yDimension: _this.startingFlowerDimensions.y,
                yPosition: _this.startingFlowerPosition.y
            };
            var newYDimension = curProps.yDimension + 2;
            var endProps = {
                yDimension: newYDimension,
                yPosition: _this.startingFlowerPosition.y + newYDimension/2
            };

            var bloomTween = new TWEEN.Tween(curProps).
              to(endProps, 3000).
              easing(TWEEN.Easing.Cubic.InOut).
              delay(1000).
              onUpdate(function() {
                Entities.editEntity(_this.flower, {
                    dimensions: {x: _this.startingFlowerDimensions.x, y: curProps.yDimension, z: _this.startingFlowerDimensions.z},
                    position: {x: _this.startingFlowerPosition.x, y: curProps.yPosition, z: _this.startingFlowerPosition.z}
                });
              }).start();
        },

        preload: function(entityID) {
            print("EBL PRELOAD");
            this.entityID = entityID;
            this.props = Entities.getEntityProperties(this.entityID, ["position", "dimensions"]);
            this.position = this.props.position;
            this.dimensions = this.props.dimensions;
            this.createFlower();
            Script.update.connect(_this.update);
        },
 
        update: function() {
            TWEEN.update();

        },

        unload: function() {
            Script.update.disconnect(_this.update);
            Entities.deleteEntity(_this.flower);
            print("EBL UNLOAD DONE")
        }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new GrowingPlant();
});