//  arcBallEntityScript.js
//  
//  Script Type: Entity
//  Created by Eric Levin on 12/17/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This entity script handles the logic for the arcBall rave toy
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include("../../libraries/utils.js");
    var _this;
    var ArcBall = function() {
        _this = this;
        this.colorPalette = [{
            red: 25,
            green: 20,
            blue: 162
        }, {
            red: 200,
            green: 10,
            blue: 10
        }];

        this.searchRadius = 10;
    };

    ArcBall.prototype = {
        isGrabbed: false,
        startDistantGrab: function() {
            this.searchForNearbyArcBalls();
        },

        startNearGrab: function() {
            this.searchForNearbyArcBalls();
        },

        searchForNearbyArcBalls: function() {
            //Search for nearby balls and create an arc to it if one is found
            var position = Entities.getEntityProperties(this.entityID, "position").position
            var entities = Entities.findEntities(position, this.searchRadius);
            entities.forEach(function(entity) {
                var props = Entities.getEntityProperties(entity, ["position", "name"]);
                if (props.name === "Arc Ball" && JSON.stringify(_this.entityID) !== JSON.stringify(entity)) {
                    _this.target = entity;
                    _this.createBeam(position, props.position);

                }
            });
        },

        createBeam: function(startPosition, endPosition) {

            // Creates particle arc from start position to end position
            var rotation = Entities.getEntityProperties(this.entityID, "rotation").rotation;
            var sourceToTargetVec = Vec3.subtract(endPosition, startPosition);
            var emitOrientation = Quat.rotationBetween(Vec3.UNIT_Z, sourceToTargetVec);
            emitOrientation = Quat.multiply(Quat.inverse(rotation), emitOrientation);

            var color = this.colorPalette[randInt(0, this.colorPalette.length)];
            var props = {
                type: "ParticleEffect",
                name: "Particle Arc",
                parentID: this.entityID,
                parentJointIndex: -1,
                // position: startPosition,
                isEmitting: true,
                colorStart: color,
                color: {
                    red: 200,
                    green: 200,
                    blue: 255
                },
                colorFinish: color,
                maxParticles: 100000,
                lifespan: 1,
                emitRate: 1000,
                emitOrientation: emitOrientation,
                emitSpeed: 1,
                speedSpread: 0.02,
                emitDimensions: {
                    x: .01,
                    y: .01,
                    z: .01
                },
                polarStart: 0,
                polarFinish: 0,
                azimuthStart: 0.02,
                azimuthFinish: .01,
                emitAcceleration: {
                    x: 0,
                    y: 0,
                    z: 0
                },
                accelerationSpread: {
                    x: 0,
                    y: 0,
                    z: 0
                },
                radiusStart: 0.01,
                radiusFinish: 0.005,
                radiusSpread: 0.005,
                alpha: 0.5,
                alphaSpread: 0.1,
                alphaStart: 0.5,
                alphaFinish: 0.5,
                textures: "https://s3.amazonaws.com/hifi-public/eric/textures/particleSprites/beamParticle.png",
                emitterShouldTrail: true
            }
            this.particleArc = Entities.addEntity(props);
        },

        updateBeam: function() {
            if(!this.target) {
                return;
            }
            var startPosition = Entities.getEntityProperties(this.entityID, "position").position;
            var targetPosition = Entities.getEntityProperties(this.target, "position").position;
            var rotation = Entities.getEntityProperties(this.entityID, "rotation").rotation;
            var sourceToTargetVec = Vec3.subtract(targetPosition, startPosition);
            var emitOrientation = Quat.rotationBetween(Vec3.UNIT_Z, sourceToTargetVec);
            Entities.editEntity(this.particleArc, {
                emitOrientation: emitOrientation
            });
        },

        continueNearGrab: function() {
            this.updateBeam();
        },

        continueDistantGrab: function() {
            this.updateBeam();
        },

        releaseGrab: function() {
            Entities.editEntity(this.particleArc, {
                isEmitting: false
            });
            this.target = null;
        },

        unload: function() {
            if (this.particleArc) {
                Entities.deleteEntity(this.particleArc);
            }
        },

        preload: function(entityID) {
            this.entityID = entityID;
        },
    };
    return new ArcBall();
});