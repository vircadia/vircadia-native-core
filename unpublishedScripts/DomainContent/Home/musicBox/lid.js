//
//
//  Created by The Content Team 4/10/216
//  Copyright 2016 High Fidelity, Inc.
//
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//

(function() {

    //TODO -- At the part of the animation where the user starts to close the lid we need to rewind any frames past the one where it is aligned for going up/down before switching to the down animation
    Script.include('../utils.js');
    var GRAB_CONSTRAINTS_USER_DATA_KEY = "grabConstraintsKey"
    var MAPPING_NAME = "com.highfidelity.musicBox";

    var _this;

    function Lid() {
        _this = this;
        return this;
    }

    var MUSIC_URL = Script.resolvePath('atp:/musicBox/music.wav');
    var OPENSHUT_SOUND_URL = Script.resolvePath('atp:/openCloseSound.wav');


    Lid.prototype = {
        disabledHand: 'none',
        lidLoopConnected: false,
        myInterval: null,
        musicInjector: null,
        preload: function(entityID) {
            this.entityID = entityID;
            this.music = SoundCache.getSound(MUSIC_URL);
            this.shutSound = SoundCache.getSound(OPENSHUT_SOUND_URL);
            this.openSound = SoundCache.getSound(OPENSHUT_SOUND_URL);

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
            var mapping = Controller.newMapping(MAPPING_NAME);

            mapping.from([Controller.Standard.RT]).peek().to(_this.rightTriggerPress);

            mapping.from([Controller.Standard.LT]).peek().to(_this.leftTriggerPress);

            Controller.enableMapping(MAPPING_NAME);

        },
        setLeftHand: function() {
            _this.disabledHand = 'left';
            print('LID HAND IS LEFT');
        },

        setRightHand: function() {
            _this.disabledHand = 'right';
            print('LID HAND IS RIGHT')
        },

        clearHand: function() {
            _this.disabledHand = "none";
        },

        createMyInteval: function() {
            var handToDisable = _this.hand;
            print("disabling hand: " + handToDisable);
            Messages.sendLocalMessage('Hifi-Hand-Disabler', handToDisable);
            _this.myInterval = Script.setInterval(function() {
                _this.handleTrigger();
            }, 16);
        },
        clearMyInterval: function() {
            Messages.sendLocalMessage('Hifi-Hand-Disabler','none');
            if (_this.myInterval !== null) {
                Script.clearInterval(_this.myInterval)
            }
        },
        rightTriggerPress: function(value) {
            print('right trigger press 1')
            if (_this.disabledHand === 'none') {
                _this.generalTriggerPress(value,'right');
                return;
            }
            if (_this.disabledHand !== 'right') {
                return;
            }
            _this.hand = 'right';
            print('RIGHT TRIGGER PRESS')
            _this.triggerPress(value);
        },

        leftTriggerPress: function(value) {
            print('left trigger press1 ')
            if (_this.disabledHand === 'none') {
                _this.generalTriggerPress(value,'left');
                return;
            }
            if (_this.disabledHand !== 'left') {
                return;
            }
            _this.hand = 'left';
            _this.triggerPress(value)
            print('LEFT TRIGGER PRESS2');
        },

        generalTriggerPress: function(value,hand) {
            // print('GENERAL TRIGGER PRESS');
            _this.hand = hand;
            if (_this.disabledHand !== 'none') {
                return;
            } else {
            var handPosition = _this.getHandPosition();
            var lidProps = Entities.getEntityProperties(_this.entityID);

            var distanceHandToLid = Vec3.distance(lidProps.position, handPosition);
            print('DISTANCE TO LID' + distanceHandToLid)

            if(distanceHandToLid<=0.4){
                 print('SOME PRESS NEARBY BUT WE ARE NOT GRABBING THE BOTTOM')
                   _this.triggerPress(value);
            }
            else{
                print('TOO FAR FOR THIS TO COUNT')
            }
            
               
            }
        },

        triggerPress: function(value) {
        
            print('SOME TRIGGER PRESS ' + value);
            if (value > 0.1 && _this.lidLoopConnected === false) {
                print('SHOULD CREATE INTERVAL')
                _this.lidLoopConnected = true;
                _this.createMyInteval();
            };

            if (value <= 0.1 && _this.lidLoopConnected === true) {
                print('SHOULD CLEAR INTERVAL')
                _this.lidLoopConnected = false;
                _this.clearMyInterval();
            }

        },
        updateSoundPositionWhileBaseIsHeld: function() {

        },

        getHandPosition: function() {
            if (_this.hand === 'left') {
                return MyAvatar.getLeftPalmPosition();
            }
            if (_this.hand === 'right') {
                return MyAvatar.getRightPalmPosition();
            }

        },

        handleTrigger: function() {

            // get the base position
            // get the hand position
            // get the distance between
            // use that value to scale the angle of the lid rotation

            var baseProps = Entities.getEntityProperties(_this.base);
            var baseBoxPosition = baseProps.position;
            var handPosition = _this.getHandPosition();
            var lidProps = Entities.getEntityProperties(_this.entityID);

            var distaceHandToBaseBox = Vec3.distance(baseBoxPosition, handPosition);

            var safeEuler = Quat.safeEulerAngles(baseProps.rotation);
            var finalRotation = {
                x: 0,
                y: 0,
                z: 0
            }

            var min1 = 0;
            var max1 = 75;

            var maxDistance = 0.4;

            var finalAngle = scaleValue(distaceHandToBaseBox, 0.1, maxDistance, 0, 75)

            // print('FINAL ANGLE:: ' + finalAngle);

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

            var hatHeight = scaleValue(constraint, 0, 75, 0, 0.04);

            this.updateHatRotation();
            this.updateKeyRotation();
            Entities.editEntity(this.entityID, {
                localRotation: Quat.fromPitchYawRollDegrees(finalRotation.x, finalRotation.y, finalRotation.z)
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

        updateHatRotation: function() {
            var baseProps = Entities.getEntityProperties(_this.base);
            var localAngularVelocity = {
                x: 0,
                y: 0.785398,
                z: 0,
            };

            var worldAngularVelocity = Vec3.multiplyQbyV(baseProps.rotation, localAngularVelocity);
            Entities.editEntity(_this.hat, {
                angularVelocity: worldAngularVelocity
            })

            // print('UPDATE HAT ANGULAR VELOCITY!' + JSON.stringify(worldAngularVelocity));

        },

        updateKeyRotation: function() {
            var baseProps = Entities.getEntityProperties(_this.base);
            var localAngularVelocity = {
                x: 0,
                y: 0,
                z: 0.785398,
            };

            var worldAngularVelocity = Vec3.multiplyQbyV(baseProps.rotation, localAngularVelocity);
            Entities.editEntity(this.key, {
                angularVelocity: worldAngularVelocity
            })

            // print('UPDATE KEY ANGULAR VELOCITY!' + JSON.stringify(worldAngularVelocity));

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
                print('this open soundinjector' + JSON.stringify(this.openSoundInjector))
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
            Messages.sendLocalMessage('Hifi-Hand-Disabler','none');
            Controller.disableMapping(MAPPING_NAME);
            print('DISABLED MAPPING:: ' + MAPPING_NAME);
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