  //
  //  fireworksLaunchEntityScript.js
  //  examples/playa/fireworks
  //
  //  Created by Eric Levina on 2/24/16.
  //  Copyright 2015 High Fidelity, Inc.
  //
  //  Run this script to spawn a big fireworks launch button that a user can press
  //
  //  Distributed under the Apache License, Version 2.0.
  //  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

  (function() {
    Script.include("../../libraries/utils.js");
    var _this;
    Fireworks = function() {
      _this = this;
      _this.launchSound = SoundCache.getSound("https://s3-us-west-1.amazonaws.com/hifi-content/eric/Sounds/missle+launch.wav");
      _this.explosionSound = SoundCache.getSound("https://s3-us-west-1.amazonaws.com/hifi-content/eric/Sounds/fireworksExplosion.wav");
      _this.timeToExplosionRange = {
        min: 2000,
        max: 4000
      };
      _this.explodeTime = randInt(500, 1000);
    };

    Fireworks.prototype = {

      startNearTrigger: function() {
        _this.shootFirework();
      },

      clickReleaseOnEntity: function() {
        _this.shootFirework();
      },

      shootFirework: function() {
        var rocketPosition = Vec3.sum(_this.position, {
          x: 0,
          y: 0.1,
          z: 0
        });
        Audio.playSound(_this.launchSound, {
          position: rocketPosition
        });

        var MODEL_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/Rocket-2.fbx";
        var missleDimensions = Vec3.multiply({
          x: 0.24,
          y: 0.7,
          z: 0.24
        }, randFloat(0.2, 1.5));
        var missleRotation = Quat.fromPitchYawRollDegrees(randInt(-30, 30), 0, randInt(-30, 30));
        var missleVelocity = Vec3.multiply(Quat.getUp(missleRotation), randFloat(1, 3));
        var missleAcceleration = Vec3.multiply(Quat.getUp(missleRotation), randFloat(0.7, 3));
        var missle = Entities.addEntity({
          type: "Model",
          modelURL: MODEL_URL,
          position: rocketPosition,
          rotation: missleRotation,
          dimensions: missleDimensions,
          damping: 0,
          dynamic: true,
          velocity: missleVelocity,
          acceleration: missleAcceleration,
          angularVelocity: {
            x: 0,
            y: randInt(-5, 5),
            z: 0
          },
          angularDamping: 0
        });

        var smokeTrailPosition = Vec3.sum(rocketPosition, {
          x: 0,
          y: -missleDimensions.y / 2 + 0.1,
          z: 0
        });
        var smokeSettings = {
          type: "ParticleEffect",
          position: smokeTrailPosition,
          lifespan: 1.6,
          lifetime: 20,
          name: "Smoke Trail",
          maxParticles: 3000,
          emitRate: 50,
          emitSpeed: 0,
          speedSpread: 0,
          dimensions: {
            x: 1000,
            y: 1000,
            z: 1000
          },
          polarStart: 0,
          polarFinish: 0,
          azimuthStart: -3.14,
          azimuthFinish: 3.14,
          emitAcceleration: {
            x: 0,
            y: 0.1,
            z: 0
          },
          accelerationSpread: {
            x: 0.1,
            y: 0,
            z: 0.1
          },
          radiusSpread: 0.001,
          particleRadius: 0.06,
          radiusStart: 0.06,
          radiusFinish: 0.2,
          alpha: 0.1,
          alphaSpread: 0,
          alphaStart: 0,
          alphaFinish: 0,
          textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
          emitterShouldTrail: true,
          parentID: missle
        };

        var smoke = Entities.addEntity(smokeSettings);

        smokeSettings.colorStart = {
          red: 75,
          green: 193,
          blue: 254
        };
        smokeSettings.color = {
          red: 202,
          green: 132,
          blue: 151
        };
        smokeSettings.colorFinish = {
          red: 250,
          green: 132,
          blue: 21
        };
        smokeSettings.lifespan = 1;
        smokeSettings.emitAcceleration.y = -1;;
        smokeSettings.alphaStart = 0.7;
        smokeSettings.alphaFinish = 0.1;
        smokeSettings.radiusFinish = 0.06;
        smokeSettings.particleRadius = 0.06;
        smokeSettings.emitRate = 200;
        smokeSettings.emitterShouldTrail = false;
        smokeSettings.name = "fire emitter";
        var fire = Entities.addEntity(smokeSettings);

        Script.setTimeout(function() {
          var explodePosition = Entities.getEntityProperties(missle, "position").position;
          _this.explodeFirework(smoke, fire, missle, explodePosition);
        }, randFloat(_this.timeToExplosionRange.min, _this.timeToExplosionRange.max));


      },

      explodeFirework: function(smoke, fire, missle, explodePosition) {
        // We just exploded firework, so stop emitting its fire and smoke
        Entities.editEntity(smoke, {
          parentID: null,
          isEmitting: false
        });
        Entities.editEntity(fire, {
          parentID: null,
          isEmitting: false
        });
        Entities.deleteEntity(missle);
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
          emitRate: randInt(500, 10000),
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
          alpha: Math.random(),
          alphaSpread: Math.random(),
          alphaStart: Math.random(),
          alphaFinish: Math.random(),
          textures: "http://ericrius1.github.io/PlatosCave/assets/star.png",
        });


        Script.setTimeout(function() {
          Entities.editEntity(firework, {
            isEmitting: false
          });
        }, _this.explodeTime);

      },

      preload: function(entityID) {
        _this.entityID = entityID;
        _this.position = Entities.getEntityProperties(_this.entityID, "position").position;
        print("EBL RELOAD ENTITY SCRIPT!!!");

      }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new Fireworks();
  });