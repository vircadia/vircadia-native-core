//
//  fireworksLaunchButtonEntityScript.js
//
//  Created by Eric Levin on 3/7/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  This is the chapter 3 entity script of the fireworks tutorial (https://docs.highfidelity.com/docs/fireworks-scripting-tutorial)
//
//  Distributed under the Apache License, Version 2.0.

  (function() {
    Script.include("../../libraries/utils.js");
    var _this;
    Fireworks = function() {
      _this = this;
      _this.launchSound = SoundCache.getSound("https://s3-us-west-1.amazonaws.com/hifi-content/eric/Sounds/missle+launch.wav");
      _this.explosionSound = SoundCache.getSound("https://s3-us-west-1.amazonaws.com/hifi-content/eric/Sounds/fireworksExplosion.wav");
      _this.TIME_TO_EXPLODE = 3000;
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
          linearDamping: 0,
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

        Script.setTimeout(function() {
          var explodePosition = Entities.getEntityProperties(smoke, "position").position;
          _this.explodeFirework(explodePosition);
        }, _this.TIME_TO_EXPLODE);

      },

    explodeFirework: function(explodePosition) {
        Audio.playSound(_this.explosionSound, {
          position: explodePosition
        });
        var firework = Entities.addEntity({
          name: "fireworks emitter",
          position: explodePosition,
          type: "ParticleEffect",
          colorStart: hslToRgb({
            h: Math.random(),
            s: 0.5,
            l: 0.7
          }),
          color: hslToRgb({
            h: Math.random(),
            s: 0.5,
            l: 0.5
          }),
          colorFinish: hslToRgb({
            h: Math.random(),
            s: 0.5,
            l: 0.7
          }),
          maxParticles: 10000,
          lifetime: 20,
          lifespan: randFloat(1.5, 3),
          emitRate: randInt(500, 5000),
          emitSpeed: randFloat(0.5, 2),
          speedSpread: 0.2,
          emitOrientation: Quat.fromPitchYawRollDegrees(randInt(0, 360), randInt(0, 360), randInt(0, 360)),
          polarStart: 1,
          polarFinish: randFloat(1.2, 3),
          azimuthStart: -Math.PI,
          azimuthFinish: Math.PI,
          emitAcceleration: {
            x: 0,
            y: randFloat(-1, -0.2),
            z: 0
          },
          accelerationSpread: {
            x: Math.random(),
            y: 0,
            z: Math.random()
          },
          particleRadius: randFloat(0.001, 0.1),
          radiusSpread: Math.random() * 0.1,
          radiusStart: randFloat(0.001, 0.1),
          radiusFinish: randFloat(0.001, 0.1),
          alpha: randFloat(0.8, 1.0),
          alphaSpread: randFloat(0.1, 0.2),
          alphaStart: randFloat(0.7, 1.0),
          alphaFinish: randFloat(0.7, 1.0),
          textures: "http://ericrius1.github.io/PlatosCave/assets/star.png",
        });


        Script.setTimeout(function() {
          Entities.editEntity(firework, {
            isEmitting: false
          });
        }, randInt(500, 1000));

      },

      preload: function(entityID) {
        _this.entityID = entityID;
        _this.position = Entities.getEntityProperties(_this.entityID, "position").position;

      }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new Fireworks();
  });
