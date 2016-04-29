  //
  //  fireworksLaunchEntityScript.js
  //  examples/playa/fireworks
  //
  //  Created by Eric Levin on 2/24/16.
  //  Copyright 2016 High Fidelity, Inc.
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
        min: 2500,
        max: 4500
      };
    };

    Fireworks.prototype = {

      startNearTrigger: function() {
        _this.shootFireworks();
      },

      startFarTrigger: function() {
        _this.shootFireworks();
      },

      clickReleaseOnEntity: function() {
        _this.shootFireworks();
      },

      shootFireworks: function() {
        // Get launch position
        var launchPosition = getEntityUserData(_this.entityID).launchPosition || _this.position;

        var numMissles = randInt(1, 3);
        for(var i = 0; i < numMissles; i++) {
        _this.shootMissle(launchPosition);
        }
      },

      shootMissle: function(launchPosition) {
        Audio.playSound(_this.launchSound, {
          position: launchPosition,
          volume: 0.5
        });

        var MODEL_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/Rocket-2.fbx";
        var missleDimensions = Vec3.multiply({
          x: 0.24,
          y: 0.7,
          z: 0.24
        }, randFloat(0.2, 1.5));
        var missleRotation = Quat.fromPitchYawRollDegrees(randInt(-60, 60), 0, randInt(-60, 60));
        var missleVelocity = Vec3.multiply(Quat.getUp(missleRotation), randFloat(2, 4));
        var missleAcceleration = Vec3.multiply(Quat.getUp(missleRotation), randFloat(1, 3));
        var missle = Entities.addEntity({
          type: "Model",
          modelURL: MODEL_URL,
          position: launchPosition,
          rotation: missleRotation,
          dimensions: missleDimensions,
          damping: 0,
          dynamic: true,
          lifetime: 20, // Just in case
          velocity: missleVelocity,
          acceleration: missleAcceleration,
          angularVelocity: {
            x: 0,
            y: randInt(-1, 1),
            z: 0
          },
          angularDamping: 0,
          visible: false
        });

        var smokeTrailPosition = Vec3.sum(launchPosition, Vec3.multiply(-missleDimensions.y / 2 + 0.1, Quat.getUp(missleRotation)));
        var smoke = Entities.addEntity({
          type: "ParticleEffect",
          position: smokeTrailPosition,
          lifespan: 10,
          lifetime: 20,
          name: "Smoke Trail",
          maxParticles: 3000,
          emitRate: 80,
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
          parentID: missle,
        });


        Script.setTimeout(function() {
          Entities.editEntity(smoke, {
            parentID: null,
            isEmitting: false
          });

          var explodeBasePosition = Entities.getEntityProperties(missle, "position").position;
          
          Entities.deleteEntity(missle);
          // Explode 1 firework immediately
            _this.explodeFirework(explodeBasePosition);
          var numAdditionalFireworks = randInt(1, 5);
          for (var i = 0; i < numAdditionalFireworks; i++) {
            Script.setTimeout(function() {
              var explodePosition = Vec3.sum(explodeBasePosition, {x: randFloat(-3, 3), y: randFloat(-3, 3), z: randFloat(-3, 3)});
              _this.explodeFirework(explodePosition);
            }, randInt(0, 1000))
          }
        }, randFloat(_this.timeToExplosionRange.min, _this.timeToExplosionRange.max));


      },

      explodeFirework: function(explodePosition) {
        // We just exploded firework, so stop emitting its fire and smoke

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
        print("EBL RELOAD ENTITY SCRIPT!!!");

      }

    };

    // entity scripts always need to return a newly constructed object of our type
    return new Fireworks();
  });
