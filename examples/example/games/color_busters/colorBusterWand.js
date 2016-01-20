//
//  colorBusterWand.js
//
//  Created by James B. Pollack @imgntn on 11/2/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This is the entity script that attaches to a wand for the Color Busters game
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//



(function() {
    Script.include("../../../libraries/utils.js");

    var COMBINED_COLOR_DURATION = 5;

    var INDICATOR_OFFSET_UP = 0.40;

    var REMOVE_CUBE_SOUND_URL = 'http://hifi-public.s3.amazonaws.com/sounds/color_busters/boop.wav';
    var COMBINE_COLORS_SOUND_URL = 'http://hifi-public.s3.amazonaws.com/sounds/color_busters/powerup.wav';

    var COLOR_INDICATOR_DIMENSIONS = {
        x: 0.10,
        y: 0.10,
        z: 0.10
    };

    var _this;

    function ColorBusterWand() {
        _this = this;
    }

    ColorBusterWand.prototype = {
        combinedColorsTimer: null,
        soundIsPlaying: false,
        preload: function(entityID) {
            print("preload");
            this.entityID = entityID;
            this.REMOVE_CUBE_SOUND = SoundCache.getSound(REMOVE_CUBE_SOUND_URL);
            this.COMBINE_COLORS_SOUND = SoundCache.getSound(COMBINE_COLORS_SOUND_URL);
        },

        collisionWithEntity: function(me, otherEntity, collision) {
            var otherProperties = Entities.getEntityProperties(otherEntity, ["name", "userData"]);
            var myProperties = Entities.getEntityProperties(me, ["userData"]);
            var myUserData = JSON.parse(myProperties.userData);
            var otherUserData = JSON.parse(otherProperties.userData);

            if (otherProperties.name === 'Hifi-ColorBusterWand') {
                print('HIT ANOTHER COLOR WAND!!');
                if (otherUserData.hifiColorBusterWandKey.colorLocked !== true && myUserData.hifiColorBusterWandKey.colorLocked !== true) {
                    if (otherUserData.hifiColorBusterWandKey.originalColorName === myUserData.hifiColorBusterWandKey.originalColorName) {
                        print('BUT ITS THE SAME COLOR!')
                        return;
                    } else {
                        print('COMBINE COLORS!' + this.entityID);
                        this.combineColorsWithOtherWand(otherUserData.hifiColorBusterWandKey.originalColorName, myUserData.hifiColorBusterWandKey.originalColorName);
                    }
                }
            }

            if (otherProperties.name === 'Hifi-ColorBusterCube') {
                if (otherUserData.hifiColorBusterCubeKey.originalColorName === myUserData.hifiColorBusterWandKey.currentColor) {
                    print('HIT THE SAME COLOR CUBE');
                    this.removeCubeOfSameColor(otherEntity);
                } else {
                    print('HIT A CUBE OF A DIFFERENT COLOR');
                }
            }
        },

        combineColorsWithOtherWand: function(otherColor, myColor) {
            print('combining my :' + myColor + " with their: " + otherColor);

            if ((myColor === 'violet') || (myColor === 'orange') || (myColor === 'green')) {
                print('MY WAND ALREADY COMBINED');
                return;
            }

            var newColor;
            if ((otherColor === 'red' && myColor == 'yellow') || (myColor === 'red' && otherColor === 'yellow')) {
                //orange
                newColor = 'orange';
            }

            if ((otherColor === 'red' && myColor == 'blue') || (myColor === 'red' && otherColor === 'blue')) {
                //violet
                newColor = 'violet';
            }

            if ((otherColor === 'blue' && myColor == 'yellow') || (myColor === 'blue' && otherColor === 'yellow')) {
                //green.
                newColor = 'green';
            }

            _this.combinedColorsTimer = Script.setTimeout(function() {
                _this.resetToOriginalColor(myColor);
                _this.combinedColorsTimer = null;
            }, COMBINED_COLOR_DURATION * 1000);

            setEntityCustomData('hifiColorBusterWandKey', this.entityID, {
                owner: MyAvatar.sessionUUID,
                currentColor: newColor,
                originalColorName: myColor,
                colorLocked: false
            });


            this.playSoundAtCurrentPosition(false);
        },

        setCurrentColor: function(newColor) {
            var color;

            if (newColor === 'orange') {
                color = {
                    red: 255,
                    green: 165,
                    blue: 0
                };
            }

            if (newColor === 'violet') {
                color = {
                    red: 128,
                    green: 0,
                    blue: 128
                };
            }

            if (newColor === 'green') {
                color = {
                    red: 0,
                    green: 255,
                    blue: 0
                };
            }

            if (newColor === 'red') {
                color = {
                    red: 255,
                    green: 0,
                    blue: 0
                };
            }

            if (newColor === 'yellow') {
                color = {
                    red: 255,
                    green: 255,
                    blue: 0
                };
            }

            if (newColor === 'blue') {
                color = {
                    red: 0,
                    green: 0,
                    blue: 255
                };
            }

            Entities.editEntity(this.colorIndicator, {
                color: color
            });

            // print('SET THIS COLOR INDICATOR TO:' + newColor);
        },

        resetToOriginalColor: function(myColor) {
            setEntityCustomData('hifiColorBusterWandKey', this.entityID, {
                owner: MyAvatar.sessionUUID,
                currentColor: myColor,
                originalColorName: myColor,
                colorLocked: false
            });

            this.setCurrentColor(myColor);
        },

        removeCubeOfSameColor: function(cube) {
            this.playSoundAtCurrentPosition(true);
            Entities.callEntityMethod(cube, 'cubeEnding');
            Entities.deleteEntity(cube);

        },

        startNearGrab: function() {
            this.currentProperties = Entities.getEntityProperties(this.entityID);
            this.createColorIndicator();
        },

        continueNearGrab: function() {
            this.currentProperties = Entities.getEntityProperties(this.entityID);

            var color = JSON.parse(this.currentProperties.userData).hifiColorBusterWandKey.currentColor;

            this.setCurrentColor(color);
            this.updateColorIndicatorLocation();
        },

        releaseGrab: function() {
            Entities.deleteEntity(this.colorIndicator);
            if (this.combinedColorsTimer !== null) {
                Script.clearTimeout(this.combinedColorsTimer);
            }

        },

        createColorIndicator: function(color) {


            var properties = {
                name: 'Hifi-ColorBusterIndicator',
                type: 'Box',
                dimensions: COLOR_INDICATOR_DIMENSIONS,
                position: this.currentProperties.position,
                dynamic: false,
                collisionless: true
            }

            this.colorIndicator = Entities.addEntity(properties);
        },

        updateColorIndicatorLocation: function() {

            var position;

            var upVector = Quat.getUp(this.currentProperties.rotation);
            var indicatorVector = Vec3.multiply(upVector, INDICATOR_OFFSET_UP);
            position = Vec3.sum(this.currentProperties.position, indicatorVector);

            var properties = {
                position: position,
                rotation: this.currentProperties.rotation
            }

            Entities.editEntity(this.colorIndicator, properties);
        },


        playSoundAtCurrentPosition: function(isRemoveCubeSound) {

            var position = Entities.getEntityProperties(this.entityID, "position").position;
            var audioProperties = {
                volume: 0.25,
                position: position
            };

            if (isRemoveCubeSound === true) {
                Audio.playSound(this.REMOVE_CUBE_SOUND, audioProperties);
            } else {
                Audio.playSound(this.COMBINE_COLORS_SOUND, audioProperties);
            }
        },


    };

    return new ColorBusterWand();
});
