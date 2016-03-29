(function() {

    var _this;

    function Lid() {
        _this = this;
        return this;
    }

    var MUSIC_URL = 'https://hifi-content.s3.amazonaws.com/DomainContent/Home/Sounds/aquarium_small.L.wav';
    var SHUT_SOUND_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/Sounds/book_fall.L.wav';
    var OPEN_SOUND_URL = 'http://public.highfidelity.io/sounds/Switches%20and%20sliders/lamp_switch_2.wav'

    Lid.prototype = {
        preload: function(entityID) {
            print('PRELOAD LID')
            _this.entityID = entityID;
            _this.music = SoundCache.getSound(MUSIC_URL);
            _this.shutSound = SoundCache.getSound(SHUT_SOUND_URL);
            _this.openSound = SoundCache.getSound(OPEN_SOUND_URL);
            _this.musicIsPlaying = false;
            _this.shut = true;
            _this.shutSoundInjector = {
                isPlaying: false
            };
            _this.musicInjector = null;
            _this.openSoundInjector = {
                isPlaying: false
            }
        },
        continueNearTrigger: function() {
            var properties = Entities.getEntityProperties(this.entityID);

        },
        playMusic: function() {
            if (this.musicIsPlaying !== true) {

                var properties = Entities.getEntityProperties(this.entityID);

                var audioOptions = {
                    position: properties.position,
                    volume: 0.75,
                    loop: true
                }
                this.musicInjector = Audio.playSound(this.music, audioOptions);
                this.musicIsPlaying = true;
            }
        },

        stopMusic: function() {
            this.musicInjector.stop();
            this.musicIsPlaying = false;
        },

        playOpenSound: function() {
            if (this.openSoundInjector.isPlaying !== true) {

                var properties = Entities.getEntityProperties(this.entityID);

                var audioOptions = {
                    position: properties.position,
                    volume: 1,
                    loop: false
                }
                this.openSoundInjector = Audio.playSound(this.openSound, audioOptions);
            }
        },
        playShutSound: function() {
            if (this.shutSoundInjector.isPlaying !== true) {

                var properties = Entities.getEntityProperties(this.entityID);

                var audioOptions = {
                    position: properties.position,
                    volume: 1,
                    loop: false
                }
                this.shutSoundInjector = Audio.playSound(this.shutSound, audioOptions);
            }
        },
        rotateLid: function(myID, paramsArray) {

            var finalRotation;
            paramsArray.forEach(function(param) {
                var p;
                // print('param is:' + param)
                try {
                    p = JSON.parse(param);
                    finalRotation = p;

                } catch (err) {
                    // print('not a json param')
                    return;
                    p = param;
                }

            });

            if (finalRotation.x > 20 && this.musicIsPlaying === false) {
                this.playMusic();
                print('play music!!')
            }
            if (finalRotation.x <= 20 && this.musicIsPlaying === true) {
                print('stop music!!')
                this.stopMusic();
            }
            if (finalRotation.x > 0 && this.shut === true) {
                print('play open sound!!')
                this.shut = false;
                this.playOpenSound();
            } else if (finalRotation.x <= 0 && this.shut === false) {
                print('play shut sound!!')
                this.shut = true;
                this.playShutSound();
            }
            Entities.editEntity(this.entityID, {
                rotation: Quat.fromPitchYawRollDegrees(finalRotation.x, finalRotation.y, finalRotation.z)
            })

        },

        unload: function() {
            this.musicInjector.stop();

        },
    }


    return new Lid();
})