//
//  waterHoseEntityScript.js
//  examples/homeContent/plant
//
//  Created by Eric Levin on 2/15/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This entity script handles the logic for spraying water when a user holds down the trigger
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


(function() {

    Script.include("../../libraries/utils.js");

    var _this;
    var TWEEN = loadTween();
    WaterHose = function() {
        _this = this;
        this.potName = "plant pot";
        _this.potSearchRadius = 5;

    };

    WaterHose.prototype = {

        clickDownOnEntity: function() {
            // search for a pot with some seeds nearby
            _this.startWatering();

        },

        startFarTrigger: function() {
            print("TRIGGER")
            _this.startWatering();
        },

        continueFarTrigger: function() {
            print("TRIGGER")
            _this.continueWatering();
        },

        stopFarTrigger : function() {
            _this.stopWatering();
        },

         startNearTrigger: function() {
            print("TRIGGER")
            _this.startWatering();
        },

        continueNearTrigger: function() {
            print("TRIGGER")
            _this.continueWatering();
        },

        stopNearTrigger : function() {
            _this.stopWatering();
        },

        holdingClickOnEntity: function() {
            _this.continueWatering();
        },

        clickReleaseOnEntity: function() {
            _this.stopWatering();
        },

        startWatering: function() {
            Entities.editEntity(_this.waterEffect, {
                isEmitting: true
            });
            var entities = Entities.findEntities(this.position, _this.potSearchRadius);
            entities.forEach(function(entity) {
                var name = Entities.getEntityProperties(entity, "name").name;
                if (name === _this.potName) {
                    // We've found out potted plant to grow!
                    Script.setTimeout(function() {
                        // Wait a bit to assign the pot so the plants grow once water hits
                      _this.pottedPlant = entity;
                    }, 1500);
                }
            });

        },
        continueWatering: function() {
            if (!_this.pottedPlant) {
                // No plant nearby to grow, so return
                return;
            }
            Entities.callEntityMethod(_this.pottedPlant, "water");

        },

        stopWatering: function() {
            Entities.editEntity(_this.waterEffect, {
                isEmitting: false
            });
            _this.pottedPlant = null;
        },

        preload: function(entityID) {
            print("EBL PRELOAD");
            this.entityID = entityID;
            this.props = Entities.getEntityProperties(this.entityID, ["position", "dimensions"]);
            this.position = this.props.position;
            this.dimensions = this.props.dimensions;
            this.createWaterEffect();

        },

        createWaterEffect: function() {
            _this.waterEffect = Entities.addEntity({
                type: "ParticleEffect",
                isEmitting: false,
                position: _this.position,
                colorStart: {
                    red: 0,
                    green: 10,
                    blue: 20
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
                lifespan: 1.5,
                emitRate: 10000,
                emitSpeed: .1,
                speedSpread: 0.0,
                emitDimensions: {
                    x: 0.1,
                    y: 0.01,
                    z: 0.1
                },
                emitAcceleration: {
                    x: 0.0,
                    y: -1.0,
                    z: 0
                },
                polarFinish: Math.PI,
                accelerationSpread: {
                    x: 0.1,
                    y: 0.0,
                    z: 0.1
                },
                particleRadius: 0.04,
                radiusSpread: 0.01,
                radiusStart: 0.03,
                alpha: 0.9,
                alphaSpread: .1,
                alphaStart: 0.7,
                alphaFinish: 0.5,
                textures: "https://s3-us-west-1.amazonaws.com/hifi-content/eric/images/raindrop.png",
            });

        },


        unload: function() {
            print("EBL UNLOAD DONE")
            Entities.deleteEntity(_this.waterEffect);
        }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new WaterHose();
});