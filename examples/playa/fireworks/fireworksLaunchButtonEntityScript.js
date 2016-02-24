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
    var _this;
    Fireworks = function() {
      _this = this;

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
        var position = Vec3.sum(_this.position, {x: 0, y: 0.5, z: 0});
        _this.missle = Entities.addEntity({
          type: "Sphere",
          position: position,
          color: {red: 200, green : 10, blue: 200},
          dimensions: {x: 0.2, y: 0.4, z: 0.2},
          damping: 0,
          dynamic: true,
          velocity: {x: 0.0, y: 0.1, z: 0},
          gravity: {x: 0, y: 0.7, z: 0}
        });

        var smokeSettings =  {
                type: "ParticleEffect",
                position: position,
                name: "Smoke Trail",
                maxParticles: 1000,
                emitRate: 20,
                emitSpeed: 0,
                speedSpread: 0,
                dimensions: {x: 1000, y: 1000, z: 1000},
                polarStart: 0,
                polarFinish: 0,
                azimuthStart: -3.14,
                azimuthFinish: 3.14,
                emitAcceleration: {
                    x: 0,
                    y: -0.5,
                    z: 0
                },
                accelerationSpread: {
                    x: 0.2,
                    y: 0,
                    z: 0.2
                },
                radiusSpread: 0.04,
                particleRadius: 0.07,
                radiusStart: 0.07,
                radiusFinish: 0.07,
                alpha: 0.7,
                alphaSpread: 0,
                alphaStart: 0,
                alphaFinish: 0,
                additiveBlending: 0,
                textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
                emitterShouldTrail: true,
                parentID: _this.missle
            };

            _this.smoke = Entities.addEntity(smokeSettings);


            Script.setTimeout(function() {
              var explodePosition = Entities.getEntityProperties(_this.missle, "position").position;
              Entities.deleteEntity(_this.missle);
              Entities.deleteEntity(_this.smoke);
              _this.explodeFirework(explodePosition);
            }, 2500)


      },

      explodeFirework: function(explodePosition) {
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
          textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/spark_2.png",
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

      },

      unload: function() {
        Entities.deleteEntity(_this.smoke);
        Entities.deleteEntity(_this.missle);
      }


    };

    // entity scripts always need to return a newly constructed object of our type
    return new Fireworks();
  });