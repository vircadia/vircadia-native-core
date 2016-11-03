//
//  fireworksLaunchButtonEntityScript.js
//
//  Created by Eric Levin on 3/7/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  This is the chapter 2 entity script of the fireworks tutorial (https://docs.highfidelity.com/docs/fireworks-scripting-tutorial)
//
//  Distributed under the Apache License, Version 2.0.

  (function() {
    Script.include("../../libraries/utils.js");
    var _this;
    Fireworks = function() {
      _this = this;
      _this.launchSound = SoundCache.getSound("https://s3-us-west-1.amazonaws.com/hifi-content/eric/Sounds/missle+launch.wav");
    };

    Fireworks.prototype = {

      startNearTrigger: function() {
        _this.shootFirework(_this.position);
      },

      startFarTrigger: function() {
        _this.shootFirework(_this.position);
      },

      clickReleaseOnEntity: function() {
        _this.shootFirework(_this.position);
      },



      shootFirework: function(launchPosition) {
        Audio.playSound(_this.launchSound, {
          position: launchPosition,
          volume: 0.5
        });


        var smoke = Entities.addEntity({
          type: "ParticleEffect",
          position: _this.position,
          velocity: {x: 0, y: 3, z: 0},
          lifespan: 10,
          lifetime: 20,
          isEmitting: true,
          name: "Smoke Trail",
          maxParticles: 3000,
          emitRate: 80,
          emitSpeed: 0,
          speedSpread: 0,
          polarStart: 0,
          polarFinish: 0,
          azimuthStart: -3.14,
          azimuthFinish: 3.14,
          emitAcceleration: {
            x: 0,
            y: 0.01,
            z: 0
          },
          accelerationSpread: {
            x: 0.01,
            y: 0,
            z: 0.01
          },
          radiusSpread: 0.03,
          particleRadius: 0.3,
          radiusStart: 0.06,
          radiusFinish: 0.9,
          alpha: 0.1,
          alphaSpread: 0,
          alphaStart: 0.7,
          alphaFinish: 0,
          textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
          emitterShouldTrail: true,
        });

      },

      preload: function(entityID) {
        _this.entityID = entityID;
        _this.position = Entities.getEntityProperties(_this.entityID, "position").position;

      }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new Fireworks();
  });
