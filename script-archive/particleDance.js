(function() {
    var NUM_BURSTS = 3; 
    var NUM_EMITTERS_PER_BURST = 11;

    var RANGE = 5.0;
    var AUDIO_RANGE = 0.5 * RANGE;
    var DIST_BETWEEN_BURSTS = 1.0;

    var PI = 3.141593;
    var DEG_TO_RAD = PI / 180.0;

    var LOUDNESS_RADIUS_RATIO = 10;

    var TEXTURE_PATH = 'https://raw.githubusercontent.com/ericrius1/SantasLair/santa/assets/smokeparticle.png';
    var cameraAxis = Quat.getFront(Camera.getOrientation());
    var center = Vec3.sum(Camera.getPosition(), Vec3.multiply(RANGE, cameraAxis));
    var audioPosition = Vec3.sum(Camera.getPosition(), Vec3.multiply(AUDIO_RANGE, cameraAxis));

      var song = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/eric/sounds/songs/Made%20In%20Heights%20-%20Forgiveness.wav");
      var audioOptions = {
        volume: 0.9, position: audioPosition
      };

      var DISTANCE_FROM_CAMERA = 7.0;

      var colorPalette = [{
        red: 0,
        green: 206,
        blue: 209
      }, {
        red: 173,
        green: 216,
        blue: 230
      }, {
        red: 0,
        green: 191,
        blue: 255
      }];

      var bursts = [];
      var audioStats;

      Burst = function(point) {
        if (!audioStats) {
          audioStats = Audio.playSound(song, audioOptions);
        }

        this.point = point;
        this.emitters = [];

        this.emitRate = randInt(80, 120);
        this.emitStrength = randInt(4.0, 6.0);

        this.direction = {
          x: randFloat(-0.3, 0.3),
          y: 1.0,
          z: 0.0
        }

        this.base = Entities.addEntity({
            type: "Sphere",
            position: this.point,
            dimensions: {
              x: 0.05,
              y: 0.05,
              z: 0.05
            },
            color: {
              red: 240,
              green: 240,
              blue: 240
            }
          });
        for (var i = 0; i < NUM_EMITTERS_PER_BURST; ++i) {
          var colorIndex = randInt(0, colorPalette.length - 1);
          var color = colorPalette[colorIndex];
          this.emitters.push(Entities.addEntity({
            type: "ParticleEffect",
            isEmitting: true,
            position: this.point,
            textures: TEXTURE_PATH,
            emitRate: this.emitRate,
            polarFinish: 25 * DEG_TO_RAD,
            color: color,
            lifespan: 1.0,
            visible: true,
            locked: false
          }));
        }

      };

      var nextPosition = center;
      var posOrNeg = -1;

      for (var i = 0; i < NUM_BURSTS; ++i) {
        posOrNeg *= -1;
        bursts.push(new Burst(nextPosition));
        var offset = {
          x: RANGE/(i+2) * posOrNeg,
          y: 0,
          z: 0
        };
        var nextPosition = Vec3.sum(nextPosition, offset);
      }

      function update(deltaTime) {
        for (var i = 0; i < NUM_BURSTS; i++) {
          if (audioStats && audioStats.loudness > 0.0) {
            for (var j = 0; j < NUM_EMITTERS_PER_BURST; ++j) {
              Entities.editEntity(bursts[i].emitters[j], {
                particleRadius: audioStats.loudness / LOUDNESS_RADIUS_RATIO
              });
            }
          }
        }
      }

      function randFloat(min, max) {
        return Math.random() * (max - min) + min;
      }

      function randInt(min, max) {
        return Math.floor(Math.random() * (max - min)) + min;
      }

      this.cleanup = function() {
        for (var i = 0; i < NUM_BURSTS; ++i) {
          Entities.deleteEntity(bursts[i].base);
          for (var j = 0; j < NUM_EMITTERS_PER_BURST; ++j) {
            var emitter = bursts[i].emitters[j];
            Entities.deleteEntity(emitter);
          }

        }
         Audio.stop();
      }

      Script.update.connect(update); 

    })();

    Script.scriptEnding.connect(cleanup);
    