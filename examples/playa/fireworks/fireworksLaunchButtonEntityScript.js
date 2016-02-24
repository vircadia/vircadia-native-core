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
      _this.timeToExplosionRange = {min: 1000, max: 2000}; 
    };

    Fireworks.prototype = {

      startNearTrigger: function() {
        print("EBL NEAR TRIGGER")
        _this.shootFirework();
      },

      clickReleaseOnEntity: function() {
        _this.shootFirework();
      },

      shootFirework: function() {
        var rocketPosition = Vec3.sum(_this.position, {x: 0, y: 0.1, z: 0});
        Audio.playSound(_this.launchSound, {position: rocketPosition});

        var MODEL_URL = "file:///C:/Users/Eric/Desktop/Rocket-2.fbx"
        _this.missle = Entities.addEntity({
          type: "Model",
          modelURL: MODEL_URL,
          position: rocketPosition,
          color: {red: 200, green : 10, blue: 200},
          dimensions: {x: 0.2, y: 0.4, z: 0.2},
          damping: 0,
          dynamic: true,
          velocity: {x: 0.0, y: 0.1, z: 0},
          acceleration: {x: 0, y: 1, z: 0}
        });

        var smokeTrailPosition = Vec3.sum(rocketPosition, {x: 0, y: -0.15, z: 0});
        var smokeSettings =  {
                type: "ParticleEffect",
                position: smokeTrailPosition,
                lifespan: 2,
                name: "Smoke Trail",
                maxParticles: 3000,
                emitRate: 50,
                emitSpeed: 0,
                speedSpread: 0,
                dimensions: {x: 1000, y: 1000, z: 1000},
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
                parentID: _this.missle
            };

            _this.smoke = Entities.addEntity(smokeSettings);

            smokeSettings.colorStart = {red: 75, green: 193, blue: 254};
            smokeSettings.color = {red: 202, green: 132, blue: 151};
            smokeSettings.colorFinish = {red: 250, green: 132, blue: 21};
            smokeSettings.lifespan = 0.7;
            smokeSettings.emitAcceleration.y= -1;;
            smokeSettings.alphaStart = 0.7; 
            smokeSettings.alphaFinish = 0.2; 
            smokeSettings.radiusFinish =  0.06;
            smokeSettings.particleRadius =  0.06;
            smokeSettings.emitRate =  200;
            smokeSettings.emitterShouldTrail = false;
            _this.fire = Entities.addEntity(smokeSettings);

            // Script.setTimeout(function() {
              // var explodePosition = Entities.getEntityProperties(_this.missle, "position").position;
              // Entities.deleteEntity(_this.missle);
              // Entities.deleteEntity(_this.smoke);
              // _this.explodeFirework(explodePosition);
            // }, randFloat(_this.timeToExplosionRange.min, _this.timeToExplosionRange.max));


      },

      explodeFirework: function(explodePosition) {
        Audio.playSound(_this.explosionSound, {position: explodePosition});
        var fireworkSettings = {
          name: "fireworks emitter",
          position: explodePosition,
          type: "ParticleEffect",
          color: {
            red: 205,
            green: 84,
            blue: 84
          },
          maxParticles: 1000,
          lifetime: 20,
          lifespan: 4,
          emitRate: 1000,
          emitSpeed: 1.5,
          speedSpread: 1.0,
          emitOrientation: {
            x: -0.2,
            y: 0,
            z: 0,
            w: 0.7
          },
          polarStart: 1,
          polarFinish: 1.2,
          azimuthStart: -Math.PI,
          azimuthFinish: Math.PI,
          emitAcceleration: {
            x: 0,
            y: -0.7,
            z: 0
          },
          accelerationSpread: {
            x: 0,
            y: 0,
            z: 0
          },
          particleRadius: 0.04,
          radiusSpread: 0,
          radiusStart: 0.06,
          radiusFinish: 0.05,
          colorSpread: {
            red: 0,
            green: 0,
            blue: 0
          },
          colorStart: {
            red: 255,
            green: 255,
            blue: 255
          },
          colorFinish: {
            red: 255,
            green: 255,
            blue: 255
          },
          alpha: 1,
          alphaSpread: 0,
          alphaStart: 0,
          alphaFinish: 0,
          textures: "http://ericrius1.github.io/PlatosCave/assets/star.png",
        };

        _this.firework = Entities.addEntity(fireworkSettings);

        Script.setTimeout(function() {
          Entities.editEntity(_this.firework, {isEmitting: false});
        }, 1000)

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