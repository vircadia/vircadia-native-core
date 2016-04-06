(function() {

    //TODO -- At the part of the animation where the user starts to close the lid we need to rewind any frames past the one where it is aligned for going up/down before switching to the down animation
    Script.include('../utils.js');
    var GRAB_CONSTRAINTS_USER_DATA_KEY = "grabConstraintsKey"

    var _this;

    function Lid() {
        _this = this;
        return this;
    }

    var MUSIC_URL = Script.resolvePath('http://hifi-content.s3.amazonaws.com/DomainContent/Home/musicBox/music_converted.wav');
    // var SHUT_SOUND_URL = Script.resolvePath('http://hifi-content.s3.amazonaws.com/DomainContent/Home/Sounds/book_fall.L.wav');
    // var OPEN_SOUND_URL = Script.resolvePath('http://public.highfidelity.io/sounds/Switches%20and%20sliders/lamp_switch_2.wav');
    var SHUT_SOUND_URL = Script.resolvePath('atp:/openSound.wav');
    var OPEN_SOUND_URL = Script.resolvePath('atp:/closeSound.wav');

    Lid.prototype = {
        musicInjector: null,
        preload: function(entityID) {
            this.entityID = entityID;
            this.music = SoundCache.getSound(MUSIC_URL);
            this.shutSound = SoundCache.getSound(SHUT_SOUND_URL);
            this.openSound = SoundCache.getSound(OPEN_SOUND_URL);

            print('OPEN SOUND?? ' + this.openSound)
            this.musicIsPlaying = false;
            this.shut = true;
            this.shutSoundInjector = {
                isPlaying: false
            };
            this.musicInjector = null;
            this.openSoundInjector = {
                isPlaying: false
            }
            this.getParts();
            this.props = Entities.getEntityProperties(this.entityID);
        },

        updateSoundPositionWhileBaseIsHeld: function() {

        },

        startNearTrigger: function() {
            this.getParts();
        },
        getGrabConstraints: function() {
            var defaultConstraints = {
                positionLocked: false,
                rotationLocked: false,
                positionMod: false,
                rotationMod: {
                    pitch: false,
                    yaw: false,
                    roll: false
                }
            }
            var constraints = getEntityCustomData(GRAB_CONSTRAINTS_USER_DATA_KEY, this.grabbedEntity, defaultConstraints);
            return constraints;
        },
        getHandPosition: function() {
            if (this.hand === 'left') {
                return MyAvatar.getLeftPalmPosition();
            }
            if (this.hand === 'right') {
                return MyAvatar.getRightPalmPosition();
            }

        },
        continueNearTrigger: function(myID, paramsArray) {
            paramsArray.forEach(function(param, index) {
                print('PARAM::' + param)
                if (index === 0) {
                    _this.hand = param;
                }
            });

            print('HAND:: ' + _this.hand);

            var constraints = this.getGrabConstraints();
            var handPosition = this.getHandPosition();


            var modTypes = [];
            if (constraints.rotationMod !== false) {
                if (constraints.rotationMod.pitch !== false) {
                    modTypes.push('pitch')
                }
                if (constraints.rotationMod.yaw !== false) {
                    modTypes.push('yaw')
                }
                if (constraints.rotationMod.roll !== false) {
                    modTypes.push('roll')
                }
            }


  
            // var properties = Entities.getEntityProperties(this.entityID);

            // var constraints = this.getGrabConstraints();
            // if (constraints.rotationMod !== false) {
            //     //implement the rotation modifier
            //     var grabbedProps = Entities.getEntityProperties(this.grabbedEntity);

            var handPosition = this.getHandPosition();

            var modTypes = [];

            if (constraints.rotationMod.pitch !== false) {
                modTypes.push('pitch')
            }
            if (constraints.rotationMod.yaw !== false) {
                modTypes.push('yaw')
            }
            if (constraints.rotationMod.roll !== false) {
                modTypes.push('roll')
            }

            // get the base position
            // get the hand position
            // 

            var baseProps = Entities.getEntityProperties(_this.base);
            var baseBoxPosition = baseProps.position;
            var handPosition = _this.getHandPosition();
            var lidProps = Entities.getEntityProperties(_this.entityID);
            
            var distaceHandToBaseBox = Vec3.distance(baseBoxPosition, handPosition);

            var safeEuler = Quat.safeEulerAngles()
            var finalRotation = {
                x: 0,
                y: 0,
                z: 0
            }

            var min1 = 0;
            var max1 = 75;

            var maxDistance = 0.4;

            var finalAngle = scale(distance, 0, maxDistance, 0, 75)

            print('FINAL ANGLE:: ' + finalAngle);

            if (finalAngle < 0) {
                finalAngle = 0;
            }
            if (finalAngle > 75) {
                finalAngle = 75;
            }
            finalRotation.z = finalAngle;
            _this.handleLidActivities(finalRotation)



        },

        handleLidActivities: function(finalRotation) {
            var constraint = finalRotation.z;

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
                this.startHat();
                this.startKey();
            } else if (constraint <= 0 && this.shut === false) {
                print('play shut sound!!')
                this.shut = true;
                this.playShutSound();
                this.stopKey();
                this.stopHat();
            }

            var hatHeight = scaleValue(constraint, MIN_LID_ROTATION, MAX_LID_ROTATION, 0, 0.04);

            Entities.editEntity(this.entityID, {
                rotation: Quat.fromPitchYawRollDegrees(finalRotation.x, finalRotation.y, finalRotation.z)
            })

            var VERTICAL_OFFSET = 0.025;
            var FORWARD_OFFSET = 0.0;
            var LATERAL_OFFSET = 0.0;

            var hatOffset = getOffsetFromCenter(VERTICAL_OFFSET, FORWARD_OFFSET, LATERAL_OFFSET)

            var baseProps = Entities.getEntityProperties(this.base);

            var upOffset = Vec3.multiply(hatHeight, Quat.getUp(baseProps.rotation));

            var hatPosition = Vec3.sum(hatOffset, upOffset)
            Entities.editEntity(this.hat, {
                position: hatPosition
            })

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
                print('music should be playing now')
            }

        },

        stopMusic: function() {
            this.musicIsPlaying = false;
            if (this.musicInjector !== null) {
                this.musicInjector.stop();
            }

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
                print('this oppen soundinejctopr' + JSON.stringify(this.openSoundInjector))
            }
        },

        playShutSound: function() {
            print('shut injector' + JSON.stringify(this.shutSoundInjector));
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

        getParts: function() {
            var properties = Entities.getEntityProperties(this.entityID);
            var results = Entities.findEntities(properties.position, 2);
            results.forEach(function(result) {

                var props = Entities.getEntityProperties(result);

                if (props.name === 'home_music_box_base') {
                    print('FOUND BASE');
                    _this.base = result;
                    _this.baseProps = props;
                }

                if (props.name === 'home_music_box_key') {
                    print('FOUND KEY')
                    _this.key = result;
                    _this.keyProps = props;
                }

                if (props.name === 'home_music_box_hat') {
                    print('FOUND HAT')
                    _this.hat = result;
                    _this.hatProps = props;
                }

            })
        },

        startHat: function() {
            Entities.editEntity(this.hat, {
                angularDamping: 0,
                angularVelocity: {
                    x: 0,
                    y: 0.785398,
                    z: 0,
                }
            })
        },

        startKey: function() {
            Entities.editEntity(this.key, {
                angularDamping: 0,
                angularVelocity: {
                    x: 0,
                    y: 0,
                    z: 0.785398,
                }
            })
        },

        stopHat: function() {
            Entities.editEntity(this.hat, {
                angularDamping: 0.5,
            });
        },

        stopKey: function() {
            Entities.editEntity(this.key, {
                angularDamping: 0.5,
            });
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

    function getOffsetFromCenter(VERTICAL_OFFSET, FORWARD_OFFSET, LATERAL_OFFSET) {

        var properties = Entities.getEntityProperties(_this.base);

        var upVector = Quat.getUp(properties.rotation);
        var frontVector = Quat.getFront(properties.rotation);
        var rightVector = Quat.getRight(properties.rotation);

        var upOffset = Vec3.multiply(upVector, VERTICAL_OFFSET);
        var frontOffset = Vec3.multiply(frontVector, FORWARD_OFFSET);
        var rightOffset = Vec3.multiply(rightVector, LATERAL_OFFSET);

        var finalOffset = Vec3.sum(properties.position, upOffset);
        finalOffset = Vec3.sum(finalOffset, frontOffset);
        finalOffset = Vec3.sum(finalOffset, rightOffset);

        return finalOffset
    };



    return new Lid();
})