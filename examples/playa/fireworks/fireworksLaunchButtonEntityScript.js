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
        var fireworkSettings = {
          name: "fireworks emitter",
          position: _this.position,
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
          emitDimensions: {
            x: 0,
            y: 0,
            z: 0
          },
          emitRadiusStart: 0.5,
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
          radiusStart: 0.14,
          radiusFinish: 0.14,
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
          alphaFinish: 1,
          textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/spark_2.png",
        };

        _this.firework = Entities.addEntity(fireworkSettings);
      },

      preload: function(entityID) {
        _this.entityID = entityID;
        _this.position = Entities.getEntityProperties(_this.entityID, "position").position;
        print("EBL RELOAD ENTITY SCRIPT!!!");

      },

      unload: function() {
        if (_this.firework) {
          Entities.deleteEntity(_this.firework);
        }
      }


    };

    // entity scripts always need to return a newly constructed object of our type
    return new Fireworks();
  });