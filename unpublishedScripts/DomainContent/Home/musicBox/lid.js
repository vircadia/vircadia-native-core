(function() {

    var _this;

    function Lid() {
        _this = this;
        return this;
    }

    var MUSIC_URL = Script.resolvePath('http://hifi-content.s3.amazonaws.com/DomainContent/Home/musicBox/music_converted.wav');
    var SHUT_SOUND_URL = Script.resolvePath('http://hifi-content.s3.amazonaws.com/DomainContent/Home/Sounds/book_fall.L.wav');
    var OPEN_SOUND_URL = Script.resolvePath('http://public.highfidelity.io/sounds/Switches%20and%20sliders/lamp_switch_2.wav');
    var BASE_ANIMATIONS = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/musicBox/MusicBoxAnimated2.fbx';

    Lid.prototype = {
        musicInjector: null,
        preload: function(entityID) {
            print('PRELOAD LID');
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


            //this might be z now that its roll
            var constraint = finalRotation.x

            var MIN_LID_ROTATION = 0;
            var MAX_LID_ROTAITON = 75;

            //handle sound on open, close, and actually play the song
            if (constraint > 20 && this.musicIsPlaying === false) {
                this.playMusic();
                print('play music!!')
            }
            if (constraint <= 20 && this.musicIsPlaying === true) {
                print('stop music!!')
                this.stopMusic();
            }
            if (constraint > 0 && this.shut === true) {
                print('play open sound!!')
                this.shut = false;
                this.playOpenSound();
            } else if (constraint <= 0 && this.shut === false) {
                print('play shut sound!!')
                this.shut = true;
                this.playShutSound();
            }

            //handle scaling the lid angle to the animation frame
            //0 to 30 hat going up
            //30-90 hat spinning

            //scale for going up down, and then spin when fully open ;)

            var currentFrame = scaleValue(constraint, MIN_LID_ROTATION, MAX_LID_ROTAITON, 0, 30)

            var animation;

            if (finalRotation.x === 75) {
                animation = {
                    loop: true,
                    hold: true,
                    firstFrame: 30,
                    lastFrame: 90,
                    animationIsPlaying: true,
                    animationFPS: 30,
                }
            } else {
                animation = {
                    url: BASE_ANIMATION_URL,
                    running: false,
                    currentFrame: currentFrame,
                    animationIsPlaying: false,
                    firstFrame: 0,
                    lastFrame: 30
                },
            }

            Entities.editEntity(this.entityID, {
                rotation: Quat.fromPitchYawRollDegrees(finalRotation.x, finalRotation.y, finalRotation.z)
            })

            Entities.editEntity(this.base, {
                animation: animation
            })

        },


        clickReleaseOnEntity: function() {

        },

        getBase: function() {
            var props = Entities.getEntityProperties(this.entityID);
            var data = JSON.parse(props.userData);
            var base = data["hifiHomeKey"].musicBoxBase;
            print('base is: ' + base);
            this.base = base;
        },

        unload: function() {
            if (this.musicInjector !== null) {
                this.musicInjector.stop();
            }
        },
    }


    var scaleValue = function(value, min1, max1, min2, max2) {
        return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
    }


    return new Lid();
})