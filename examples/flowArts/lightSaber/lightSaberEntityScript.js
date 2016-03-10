//  lightSaberEntityScript.js
//  
//  Script Type: Entity
//  Created by Eric Levin on 12/17/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This entity script creates the logic for displaying the lightsaber beam.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include("../../libraries/utils.js");
    var _this;
    // this is the "constructor" for the entity as a JS object we don't do much here
    var LightSaber = function() {
        _this = this;
        this.colorPalette = [{
            red: 0,
            green: 200,
            blue: 40
        }, {
            red: 200,
            green: 10,
            blue: 40
        }];
    };

    LightSaber.prototype = {
        isGrabbed: false,

        startNearGrab: function() {
            Entities.editEntity(this.beam, {
                isEmitting: true,
                visible: true
            });
        },

        releaseGrab: function() {
            Entities.editEntity(this.beam, {
                visible: false,
                isEmitting: false
            });
        },

        preload: function(entityID) {
            this.entityID = entityID;
            this.createBeam();
        },

        unload: function() {
            Entities.deleteEntity(this.beam);
        },

        createBeam: function() {

            this.props = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);
            var forwardVec = Quat.getFront(Quat.multiply(this.props.rotation, Quat.fromPitchYawRollDegrees(-90, 0, 0)));
            var forwardQuat = Quat.rotationBetween(Vec3.UNIT_Z, forwardVec);
            var position = this.props.position;
     
            var color = this.colorPalette[randInt(0, this.colorPalette.length)];
            var props = {
                type: "ParticleEffect",
                name: "LightSaber Beam",
                position: position,
                parentID: this.entityID,
                isEmitting: false,
                colorStart: color,
                color: {
                    red: 200,
                    green: 200,
                    blue: 255
                },
                colorFinish: color,
                maxParticles: 100000,
                lifespan: 2,
                emitRate: 1000,
                emitOrientation: forwardQuat,
                emitSpeed: 0.7,
                speedSpread: 0.0,
                emitDimensions: {
                    x: 0,
                    y: 0,
                    z: 0
                },
                polarStart: 0,
                polarFinish: 0,
                azimuthStart: 0.1,
                azimuthFinish: 0.01,
                emitAcceleration: {
                    x: 0,
                    y: 0,
                    z: 0
                },
                accelerationSpread: {
                    x: .00,
                    y: .00,
                    z: .00
                },
                radiusStart: 0.03,
                adiusFinish: 0.025,
                alpha: 0.7,
                alphaSpread: 0.1,
                alphaStart: 0.5,
                alphaFinish: 0.5,
                textures: "https://s3.amazonaws.com/hifi-public/eric/textures/particleSprites/beamParticle.png",
                emitterShouldTrail: false
            }
            this.beam = Entities.addEntity(props);

        }
    };
    // entity scripts always need to return a newly constructed object of our type
    return new LightSaber();
});
