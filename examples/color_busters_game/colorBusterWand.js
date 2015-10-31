(function() {

    //seconds that the combination lasts
    var COMBINED_COLOR_DURATION = 10;

    var INDICATOR_OFFSET_UP = 0.5;

    var REMOVE_CUBE_SOUND_URL = '';
    var COMBINE_COLORS_SOUND_URL = '';

    var _this;

    function ColorWand() {
        _this = this;
    };

    ColorBusterWand.prototype = {
        combinedColorsTimer: null,

        preload: function(entityID) {
            print("preload");
            this.entityID = entityID;
            this.REMOVE_CUBE_SOUND = SoundCache.getSound(REMOVE_CUBE_SOUND_URL);
            this.COMBINE_COLORS_SOUND = SoundCache.getSound(COMBINE_COLORS_SOUND_URL);

        },

        entityCollisionWithEntity: function(me, otherEntity, collision) {

            var otherProperties = Entities.getEntityProperties(otherEntity, ["name", "userData"]);
            var myProperties = Entities.getEntityProperties(otherEntity, ["userData"]);
            var myUserData = JSON.parse(myProperties.userData);
            var otherUserData = JSON.parse(otherProperties.userData);

            if (otherProperties.name === 'Hifi-ColorBusterWand') {
                if (otherUserData.hifiColorBusterWandKey.colorLocked !== true && myUserData.hifiColorBusterWandKey.colorLocked !== true) {
                    if (otherUserData.hifiColorBusterWandKey.originalColorName === myUserData.hifiColorBusterWandKey.originalColorName) {
                        return;
                    } else {
                        this.combineColorsWithOtherWand(otherUserData.hifiColorBusterWandKey.originalColorName, myUserData.hifiColorBusterWandKey.originalColorName);
                    }
                }
            }

            if (otherProperties.name === 'Hifi-ColorBusterCube') {
                if (otherUserData.hifiColorBusterCubeKey.originalColorName === myUserData.hifiColorBusterWandKey.currentColor) {
                    removeCubeOfSameColor(otherEntity);
                }

            }
        },

        combineColorsWithOtherWand: function(otherColor, myColor) {
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

            setEntityCustomData(hifiColorWandKey, this.entityID, {
                owner: MyAvatar.sessionUUID,
                currentColor: newColor
                originalColorName: myColor,
                colorLocked: false
            });

            _this.setCurrentColor(newColor);

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
            })
        },

        resetToOriginalColor: function(myColor) {
            setEntityCustomData(hifiColorWandKey, this.entityID, {
                owner: MyAvatar.sessionUUID,
                currentColor: myColor
                originalColorName: myColor,
                colorLocked: true
            });

            this.setCurrentColor(myColor);
        },

        removeCubeOfSameColor: function(cube) {
            Entities.callEntityMethod(cube, 'cubeEnding');
            Entities.deleteEntity(cube);
        },

        startNearGrab: function() {
            this.currentProperties = Entities.getEntityProperties(this.entityID);
            this.createColorIndicator();
        },

        continueNearGrab: function() {
            this.currentProperties = Entities.getEntityProperties(this.entityID);
            this.updateColorIndicatorLocation();
        },

        releaseGrab: function() {
            this.deleteEntity(this.colorIndicator);
            if (this.combinedColorsTimer !== null) {
                Script.clearTimeout(this.combinedColorsTimer);
            }

        },

        createColorIndicator: function() {
            var properties = {
                name: 'Hifi-ColorBusterIndicator',
                type: 'Box',
                dimensions: COLOR_INDICATOR_DIMENSIONS,
                color: this.currentProperties.position,
                position: this.currentProperties.position
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


        playSoundAtCurrentPosition: function(removeCubeSound) {
            var position = Entities.getEntityProperties(this.entityID, "position").position;

            var audioProperties = {
                volume: 0.25,
                position: position
            };

            if (removeCubeSound) {
                Audio.playSound(this.REMOVE_CUBE_SOUND, audioProperties);
            } else {
                Audio.playSound(this.COMBINE_COLORS_SOUND, audioProperties);
            }
        },


    };

    return new ColorBusterWand();
});